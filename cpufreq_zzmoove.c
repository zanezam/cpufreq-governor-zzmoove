/*
 *  drivers/cpufreq/cpufreq_zzmoove.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 * - ZZMoove Governor Changelog:                                                                                                                          -
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Version 0.1 - first release
 *
 *	- codebase latest smoove governor version from midnight kernel (https://github.com/mialwe/mngb/)
 *	- modified frequency tables to match I9300 standard frequency range 200-1400 mhz
 *	- added cpu hotplug functionality with strictly cpu switching
 *	  (modifications partially taken from ktoonservative governor from
 *	  ktoonsez KT747-JB kernel https://github.com/ktoonsez/KT747-JB)
 *
 * Version 0.2 - improved
 *
 *	- added tuneables to be able to adjust values on early suspend (screen off) via sysfs instead
 *	  of using only hardcoded defaults
 *	- modified hotplug implementation to be able to tune threshold range per core indepentently
 *	  and to be able to manually turn cores offline
 *
 *	  for this functions following new tuneables were indroduced:
 *
 *	  sampling_rate_sleep_multiplier -> sampling rate multiplier on early suspend (possible values 1 or 2, default: 2)
 *	  up_threshold_sleep		 -> up threshold on early suspend (possible range from above "down_threshold_sleep" up to 100, default: 90)
 *	  down_threshold_sleep		 -> down threshold on early suspend (possible range from 11 to "under up_threshold_sleep", default: 44)
 *	  smooth_up_sleep		 -> smooth up scaling on early suspend (possible range from 1 to 100, default: 100)
 *	  up_threshold_hotplug1		 -> hotplug threshold for cpu1 (0 disable core1, possible range from "down_threshold" up to 100, default: 68)
 *	  up_threshold_hotplug2		 -> hotplug threshold for cpu2 (0 disable core2, possible range from "down_threshold" up to 100, default: 68)
 *	  up_threshold_hotplug3		 -> hotplug threshold for cpu3 (0 disable core3, possible range from "down_threshold" up to 100, default: 68)
 *	  down_threshold_hotplug1	 -> hotplug threshold for cpu1 (possible range from 11 to under "up_threshold", default: 55)
 *	  down_threshold_hotplug2	 -> hotplug threshold for cpu2 (possible range from 11 to under "up_threshold", default: 55)
 *	  down_threshold_hotplug3	 -> hotplug threshold for cpu3 (possible range from 11 to under "up_threshold", default: 55)
 *
 * Version 0.3 - more improvements
 *
 *	- added tuneable "hotplug_sleep" to be able to turn cores offline only on early suspend (screen off) via sysfs
 *	  possible values: 0 do not touch hotplug-settings on early suspend, values 1, 2 or 3 are equivalent to
 *	  cores which should be online at early suspend
 *	- modified scaling frequency table to match "overclock" freqencies to max 1600 mhz
 *	- fixed black screen of dead problem in hotplug logic due to missing mutexing on 3-core and 2-core settings
 *	- code cleaning and documentation
 *
 * Version 0.4 - limits
 *
 *	- added "soft"-freqency-limit. the term "soft" means here that this is unfortuneately not a hard limit. a hard limit is only possible with 
 *	  cpufreq driver which is the freqency "giver" the governor is only the "consultant". So now the governor will scale up to only the given up 
 *	  limit on higher system load but if the cpufreq driver "wants" to go above that limit the freqency will go up there. u can see this for 
 *	  example with touchboost or wake up freqencies (1000 and 800 mhz) where the governor obviously will be "bypassed" by the cpufreq driver.
 *	  but nevertheless this soft-limit will now reduce the use of freqencies higer than given limit and therefore it will reduce power consumption.
 *
 *	  for this function following new tuneables were indroduced:
 *
 *	  freq_limit_sleep		 -> limit freqency on early suspend (possible values 0 disable limit, 200-1600, default: 0)
 *	  freq_limit			 -> limit freqency on awake (possible values 0 disable limit, 200-1600, default: 0)
 *
 *	- added scaling frequencies to frequency tables for a faster up/down scaling. This should bring more performance but on the other hand it
 *	  can be of course a little bit more power consumptive.
 *
 *	  for this function following new tuneables were indroduced:
 *	
 *	  fast_scaling			 -> fast scaling on awake (possible values 0 disable or 1 enable, default: 0)
 *	  fast_scaling_sleep (sysfs)	 -> fast scaling on early suspend (possible values 0 disable or 1 enable, default: 0)
 *
 *	- added tuneable "freq_step_sleep" for setting the freq step at early suspend (possible values same as freq_step 0 to 100, default 5)
 *	- added DEF_FREQ_STEP and IGNORE_NICE definitions
 *	- changed downscaling cpufreq relation to the "lower way"
 *	- code/documentation cleaning
 *---------------------------------------------------------------------------------------------------------------------------------------------------------
 *-                                                                                                                                                       -
 *---------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/earlysuspend.h>

// smooth up/downscaling via lookup tables
#define MN_SMOOTH 1

// cpu load trigger
#define DEF_SMOOTH_UP (75)

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

// ZZ: midnight and zzmoove default values
#define DEF_FREQUENCY_UP_THRESHOLD		(70)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG1	(68)	// ZZ: default for hotplug up threshold for cpu1 (cpu0 stays allways on)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG2	(68)	// ZZ: default for hotplug up threshold for cpu2 (cpu0 stays allways on)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG3	(68)	// ZZ: default for hotplug up threshold for cpu3 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD		(52)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG1	(55)	// ZZ: default for hotplug down threshold for cpu1 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG2	(55)	// ZZ: default for hotplug down threshold for cpu2 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG3	(55)	// ZZ: default for hotplug down threshold for cpu3 (cpu0 stays allways on)
#define DEF_IGNORE_NICE				(0)	// ZZ: default for ignore nice load
#define DEF_FREQ_STEP				(5)	// ZZ: default for freq step on awake
#define DEF_FREQ_STEP_SLEEP			(5)	// ZZ: default for freq step on early suspend

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */

#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

// raise sampling rate to SR*multiplier and adjust sampling rate/thresholds/hotplug/scaling/freq limit/freq step on blank screen
static unsigned int sampling_rate_awake;
static unsigned int up_threshold_awake;
static unsigned int down_threshold_awake;
static unsigned int smooth_up_awake;
static unsigned int freq_limit_awake;			// ZZ: for saving freqency limit awake value
static unsigned int fast_scaling_awake;			// ZZ: for saving fast scaling awake value
static unsigned int freq_step_awake;			// ZZ: for saving frequency step awake value
static unsigned int hotplug1_awake;			// ZZ: for saving hotplug1 threshold awake value
static unsigned int hotplug2_awake;			// ZZ: for saving hotplug2 threshold awake value
static unsigned int hotplug3_awake;			// ZZ: for saving hotplug3 threshold awake value
static unsigned int sampling_rate_asleep;		// ZZ: for setting sampling rate value on early suspend
static unsigned int up_threshold_asleep;		// ZZ: for setting up threshold value on early suspend
static unsigned int down_threshold_asleep;		// ZZ: for setting down threshold value on early suspend
static unsigned int smooth_up_asleep;			// ZZ: for setting smooth scaling value on early suspend
static unsigned int freq_limit_asleep;			// ZZ: for setting frequency limit value on early suspend
static unsigned int fast_scaling_asleep;		// ZZ: for setting fast scaling value on early suspend
static unsigned int freq_step_asleep;			// ZZ: for setting freq step value on early suspend

// ZZ: midnight defaults
#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define DEF_SAMPLING_DOWN_FACTOR		(4)
#define MAX_SAMPLING_DOWN_FACTOR		(10)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

// ZZ: midnight and zzmoove defaults for suspend
#define SAMPLING_RATE_SLEEP_MULTIPLIER 		(2)	// ZZ: default for tuneable sampling_rate_sleep_multiplier
#define DEF_UP_THRESHOLD_SLEEP			(90)	// ZZ: default for tuneable up_threshold_sleep
#define DEF_DOWN_THRESHOLD_SLEEP		(44)	// ZZ: default for tuneable down_threshold_sleep
#define DEF_SMOOTH_UP_SLEEP			(100)	// ZZ: default for tuneable smooth_up_sleep

/*
* ZZ: Hotplug Sleep: 0 do not touch hotplug settings on early suspend, so all cores will be online
* values 1, 2 or 3 are equivalent to cores which should be online on early suspend
*/

#define DEF_HOTPLUG_SLEEP			(0)	// ZZ: default for tuneable hotplug_sleep

/*
* ZZ: Frequency Limit: 0 do not limit frequency and use the full range up to cpufreq->max limit
* values 200000 -> 1600000 (khz)
*/

#define DEF_FREQ_LIMIT				(0)	// ZZ: default for tuneable freq_limit
#define DEF_FREQ_LIMIT_SLEEP			(0)	// ZZ: default for tuneable freq_limit_sleep

/*
* ZZ: Fast Scaling: 0 do not activate fast scaling freqencies
* value 1 to enable fast scaling freqencies
*/

#define DEF_FAST_SCALING			(0)	// ZZ: default for tuneable fast_scaling
#define DEF_FAST_SCALING_SLEEP			(0)	// ZZ: default for tuneable fast_scaling_sleep

static void do_dbs_timer(struct work_struct *work);

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	unsigned int down_skip;
	unsigned int requested_freq;
	int cpu;
	unsigned int enable:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, cs_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int sampling_rate_sleep_multiplier;	// ZZ: added tuneable sampling_rate_sleep_multiplier
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int up_threshold_hotplug1;		// ZZ: added tuneable up_threshold_hotplug1 for core1
	unsigned int up_threshold_hotplug2;		// ZZ: added tuneable up_threshold_hotplug2 for core2
	unsigned int up_threshold_hotplug3;		// ZZ: added tuneable up_threshold_hotplug3 for core3
	unsigned int up_threshold_sleep;		// ZZ: added tuneable up_threshold_sleep for early suspend
	unsigned int down_threshold;
	unsigned int down_threshold_hotplug1;		// ZZ: added tuneable down_threshold_hotplug1 for core1
	unsigned int down_threshold_hotplug2;		// ZZ: added tuneable down_threshold_hotplug2 for core2
	unsigned int down_threshold_hotplug3;		// ZZ: added tuneable down_threshold_hotplug3 for core3
	unsigned int down_threshold_sleep;		// ZZ: added tuneable down_threshold_sleep for early suspend
	unsigned int ignore_nice;
	unsigned int freq_step;
	unsigned int freq_step_sleep;			// ZZ: added tuneable freq_step_sleep for early suspend
	unsigned int smooth_up;
	unsigned int smooth_up_sleep;			// ZZ: added tuneable smooth_up_sleep for early suspend
	unsigned int hotplug_sleep;			// ZZ: added tuneable hotplug_sleep for early suspend
	unsigned int freq_limit;			// ZZ: added tuneable freq_limit
	unsigned int freq_limit_sleep;			// ZZ: added tuneable freq_limit_sleep
	unsigned int fast_scaling;			// ZZ: added tuneable fast_scaling
	unsigned int fast_scaling_sleep;		// ZZ: added tuneable fast_scaling_sleep
	
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.up_threshold_hotplug1 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG1,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug2 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG2,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug3 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG3,		// ZZ: set default value for new tuneable
	.up_threshold_sleep = DEF_UP_THRESHOLD_SLEEP,				// ZZ: set default value for new tuneable
	.down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD,
	.down_threshold_hotplug1 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG1,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug2 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG2,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug3 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG3,	// ZZ: set default value for new tuneable
	.down_threshold_sleep = DEF_DOWN_THRESHOLD_SLEEP,			// ZZ: set default value for new tuneable
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.sampling_rate_sleep_multiplier = SAMPLING_RATE_SLEEP_MULTIPLIER,	// ZZ: set default value for new tuneable
	.ignore_nice = DEF_IGNORE_NICE,						// ZZ: set default value for tuneable
	.freq_step = DEF_FREQ_STEP,						// ZZ: set default value for new tuneable
	.freq_step_sleep = DEF_FREQ_STEP_SLEEP,					// ZZ: set default value for new tuneable
	.smooth_up = DEF_SMOOTH_UP,
	.smooth_up_sleep = DEF_SMOOTH_UP_SLEEP,					// ZZ: set default value for new tuneable
	.hotplug_sleep = DEF_HOTPLUG_SLEEP,					// ZZ: set default value for new tuneable
	.freq_limit = DEF_FREQ_LIMIT,						// ZZ: set default value for new tuneable
	.freq_limit_sleep = DEF_FREQ_LIMIT_SLEEP,				// ZZ: set default value for new tuneable
	.fast_scaling = DEF_FAST_SCALING,					// ZZ: set default value for new tuneable
	.fast_scaling_sleep = DEF_FAST_SCALING_SLEEP,				// ZZ: set default value for new tuneable
};

/**
 * Smooth scaling conservative governor (by Michael Weingaertner)
 *
 * This modification makes the governor use two lookup tables holding
 * current, next and previous frequency to directly get a correct
 * target frequency instead of calculating target frequencies with
 * up_threshold and step_up %. The two scaling lookup tables used
 * contain different scaling steps/frequencies to achieve faster upscaling
 * on higher CPU load.
 *
 * CPU load triggering faster upscaling can be adjusted via SYSFS,
 * VALUE between 1 and 100 (% CPU load):
 * echo VALUE > /sys/devices/system/cpu/cpufreq/zzmoove/smooth_up
 *
 * improved by Zane Zaminsky 2012/13
 */

#define MN_FREQ 0
#define MN_UP 1
#define MN_DOWN 2

// ZZ: up/down switches for fast scaling columns
#define ZZ_UP 3
#define ZZ_DOWN 4

/*
 * Table modified for use with Samsung I9300 by ZaneZam November 2012
 * zzmoove v0.3 - table modified to reach overclocking frequencies up to 1600mhz
 * zzmoove v0.4 - added fast scaling columns to frequency table
 */
static int mn_freqs[15][5]={
    {1600000,1600000,1500000, 1600000, 1300000},
    {1500000,1500000,1400000, 1600000, 1200000},
    {1400000,1400000,1300000, 1600000, 1100000},
    {1300000,1400000,1200000, 1400000, 1000000},
    {1200000,1300000,1100000, 1400000,  900000},
    {1100000,1200000,1000000, 1200000,  800000},
    {1000000,1100000, 900000, 1200000,  700000},
    { 900000,1000000, 800000, 1000000,  600000},
    { 800000, 900000, 700000, 1000000,  500000},
    { 700000, 800000, 600000,  800000,  400000},
    { 600000, 700000, 400000,  800000,  300000},
    { 500000, 600000, 300000,  600000,  200000},
    { 400000, 500000, 200000,  600000,  200000},
    { 300000, 400000, 200000,  400000,  200000},
    { 200000, 300000, 200000,  400000,  200000}
};

/*
 * Table modified for use with Samsung I9300 by ZaneZam November 2012
 * zzmoove v0.3 - table modified to reach overclocking frequencies up to 1600mhz
 * zzmoove v0.4 - added fast scaling columns to frequency table
 */
static int mn_freqs_power[15][5]={
    {1600000,1600000,1500000, 1600000, 1200000},
    {1500000,1600000,1400000, 1600000, 1100000},
    {1400000,1600000,1300000, 1600000, 1000000},
    {1300000,1500000,1200000, 1600000,  900000},
    {1200000,1400000,1100000, 1400000,  800000},
    {1100000,1300000,1000000, 1400000,  700000},
    {1000000,1200000, 900000, 1400000,  600000},
    { 900000,1100000, 800000, 1200000,  500000},
    { 800000,1000000, 700000, 1200000,  400000},
    { 700000, 900000, 600000, 1000000,  300000},
    { 600000, 800000, 500000, 1000000,  200000},
    { 500000, 700000, 400000,  800000,  200000},
    { 400000, 600000, 300000,  800000,  200000},
    { 300000, 500000, 200000,  600000,  200000},
    { 200000, 400000, 200000,  600000,  200000}
};

static int mn_get_next_freq(unsigned int curfreq, unsigned int updown, unsigned int load) {
    int i=0;

    if (load < dbs_tuners_ins.smooth_up)
    {
	    for(i = 0; i < 15 ; i++)
    	    {
        	if(curfreq == mn_freqs[i][MN_FREQ])
        	    return mn_freqs[i][updown]; // updown 1|2
    	    }
    }
    else
    {
	    for(i = 0; i < 15; i++)
    	    {
        	if(curfreq == mn_freqs_power[i][MN_FREQ])
            	    return mn_freqs_power[i][updown]; // updown 1|2
    	    }
    }
    return (curfreq); // not found
}

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

/* keep track of frequency transitions */
static int
dbs_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
		     void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpu_dbs_info_s *this_dbs_info = &per_cpu(cs_cpu_dbs_info,
							freq->cpu);

	struct cpufreq_policy *policy;

	if (!this_dbs_info->enable)
		return 0;

	policy = this_dbs_info->cur_policy;

	/*
	 * we only care if our internally tracked freq moves outside
	 * the 'valid' ranges of freqency available to us otherwise
	 * we do not change it
	*/
	if (this_dbs_info->requested_freq > policy->max
			|| this_dbs_info->requested_freq < policy->min)
		this_dbs_info->requested_freq = freq->new;

	return 0;
}

static struct notifier_block dbs_cpufreq_notifier_block = {
	.notifier_call = dbs_cpufreq_notifier
};

/************************** sysfs interface ************************/
static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_zzmoove Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(sampling_rate_sleep_multiplier, sampling_rate_sleep_multiplier);	// ZZ: added smapling_rate_sleep_multiplier tuneable for early suspend
show_one(sampling_down_factor, sampling_down_factor);
show_one(up_threshold, up_threshold);
show_one(up_threshold_sleep, up_threshold_sleep);				// ZZ: added up_threshold_sleep tuneable for early suspend
show_one(up_threshold_hotplug1, up_threshold_hotplug1);				// ZZ: added up_threshold_hotplug1 tuneable for cpu1
show_one(up_threshold_hotplug2, up_threshold_hotplug2);				// ZZ: added up_threshold_hotplug2 tuneable for cpu2
show_one(up_threshold_hotplug3, up_threshold_hotplug3);				// ZZ: added up_threshold_hotplug3 tuneable for cpu3
show_one(down_threshold, down_threshold);
show_one(down_threshold_sleep, down_threshold_sleep);				// ZZ: added down_threshold_sleep tuneable for early suspend
show_one(down_threshold_hotplug1, down_threshold_hotplug1);			// ZZ: added down_threshold_hotplug1 tuneable for cpu1
show_one(down_threshold_hotplug2, down_threshold_hotplug2);			// ZZ: added down_threshold_hotplug2 tuneable for cpu2
show_one(down_threshold_hotplug3, down_threshold_hotplug3);			// ZZ: added down_threshold_hotplug3 tuneable for cpu3
show_one(ignore_nice_load, ignore_nice);
show_one(freq_step, freq_step);
show_one(freq_step_sleep, freq_step_sleep);					// ZZ: added freq_step_sleep tuneable for early suspend
show_one(smooth_up, smooth_up);
show_one(smooth_up_sleep, smooth_up_sleep);					// ZZ: added smooth_up_sleep tuneable for early suspend
show_one(hotplug_sleep, hotplug_sleep);						// ZZ: added hotplug_sleep tuneable for early suspend
show_one(freq_limit, freq_limit);						// ZZ: added freq_limit tuneable
show_one(freq_limit_sleep, freq_limit_sleep);					// ZZ: added freq_limit_sleep tuneable for early suspend
show_one(fast_scaling, fast_scaling);						// ZZ: added fast_scaling tuneable
show_one(fast_scaling_sleep, fast_scaling_sleep);				// ZZ: added fast_scaling_sleep tuneable for early suspend

static ssize_t store_sampling_down_factor(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_down_factor = input;
	return count;
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);
	return count;
}

// ZZ: added tuneable -> possible values: 1 or 2, if not set default is 2
static ssize_t store_sampling_rate_sleep_multiplier(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > SAMPLING_RATE_SLEEP_MULTIPLIER || input < 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_rate_sleep_multiplier = input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 ||
			input <= dbs_tuners_ins.down_threshold)
		return -EINVAL;

	dbs_tuners_ins.up_threshold = input;
	return count;
}

// ZZ: added tuneble -> possible values: range from above down_threshold_sleep value up to 100, if not set default is 90
static ssize_t store_up_threshold_sleep(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 ||
			input <= dbs_tuners_ins.down_threshold_sleep)
		return -EINVAL;

	dbs_tuners_ins.up_threshold_sleep = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug1(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug1 = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug2(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug2 = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug3(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug3 = input;
	return count;
}

static ssize_t store_down_threshold(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold_sleep but not up_threshold_sleep, if not set default is 44
static ssize_t store_down_threshold_sleep(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold_sleep)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_sleep = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug1(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug1 = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug2(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug2 = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug3(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug3 = input;
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) /* nothing to do */
		return count;

	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
	}
	return count;
}

static ssize_t store_freq_step(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 100)
		input = 100;

	/* no need to test here if freq_step is zero as the user might actually
	 * want this, they would be crazy though :) */
	dbs_tuners_ins.freq_step = input;
	return count;
}

/*
 * ZZ: added tuneable -> possible values: range from 0 to 100, if not set default is 5 -> value 0 will stop freq scaling and hold on actual freq
 * value 100 will directly jump up/down to limits like ondemand governor
 */
static ssize_t store_freq_step_sleep(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 100)
		input = 100;

	/* no need to test here if freq_step is zero as the user might actually
	 * want this, they would be crazy though :) */
	dbs_tuners_ins.freq_step_sleep = input;
	return count;
}

static ssize_t store_smooth_up(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1)
		return -EINVAL;

	dbs_tuners_ins.smooth_up = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 1 to 100, if not set default is 100
static ssize_t store_smooth_up_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1)
		return -EINVAL;

	dbs_tuners_ins.smooth_up_sleep = input;
	return count;
}

/* 
 * ZZ: added tuneable -> possible values: 0 do not touch the hotplug values on early suspend,
 * 1-3 equals cores to run on early suspend, if not set default is 0
 */
static ssize_t store_hotplug_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 3 || (input < 1 && input != 0))
		return -EINVAL;

	dbs_tuners_ins.hotplug_sleep = input;
	return count;
}

/* 
 * ZZ: added tuneable -> possible values: 0 disable, 200000-1600000 khz -> freqency soft-limit, if not set default is 0
 * todo: check against cpufreq->max
 */
static ssize_t store_freq_limit(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;
	int valid_freq[16]={0, 1600000, 1500000, 1400000, 1300000, 1200000, 1100000, 1000000, 900000, 800000, 700000, 600000, 500000, 400000, 300000, 200000};
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 1600000 || (input < 200000 && input != 0))
		return -EINVAL;
        
        for (i=0; i<16; i++) {
	    if (input == valid_freq[i]) {
		dbs_tuners_ins.freq_limit = input;
	    return count;
	    }
	}
	return -EINVAL;
}

/* 
 * ZZ: added tuneable -> possible values: 0 disable, 200000-1600000 khz -> freqency soft-limit on early suspend, if not set default is 0
 */
static ssize_t store_freq_limit_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;
	int valid_freq[16]={0, 1600000, 1500000, 1400000, 1300000, 1200000, 1100000, 1000000, 900000, 800000, 700000, 600000, 500000, 400000, 300000, 200000};
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 1600000 || (input < 200000 && input != 0))
		return -EINVAL;
	
        for (i=0; i<16; i++) {
	    if (input == valid_freq[i]) {
		dbs_tuners_ins.freq_limit_sleep = input;
	    return count;
	    }
	}
	return -EINVAL;
}

static ssize_t store_fast_scaling(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 3 || input < 0)
		return -EINVAL;

	dbs_tuners_ins.fast_scaling = input;
	return count;
}

static ssize_t store_fast_scaling_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 3 || input < 0)
		return -EINVAL;

	dbs_tuners_ins.fast_scaling_sleep = input;
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(sampling_rate_sleep_multiplier);	// ZZ: added tuneable
define_one_global_rw(sampling_down_factor);
define_one_global_rw(up_threshold);
define_one_global_rw(up_threshold_sleep); 		// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug1);		// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug2);		// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug3);		// ZZ: added tuneable
define_one_global_rw(down_threshold);
define_one_global_rw(down_threshold_sleep);		// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug1);		// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug2);		// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug3);		// ZZ: added tuneable
define_one_global_rw(ignore_nice_load);
define_one_global_rw(freq_step);
define_one_global_rw(freq_step_sleep);			// ZZ: added tuneable
define_one_global_rw(smooth_up);
define_one_global_rw(smooth_up_sleep);			// ZZ: added tuneable
define_one_global_rw(hotplug_sleep);			// ZZ: added tuneable
define_one_global_rw(freq_limit);			// ZZ: added tuneable
define_one_global_rw(freq_limit_sleep);			// ZZ: added tuneable
define_one_global_rw(fast_scaling);			// ZZ: added tuneable
define_one_global_rw(fast_scaling_sleep);		// ZZ: added tuneable

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&sampling_rate_sleep_multiplier.attr,		// ZZ: added tuneable
	&sampling_down_factor.attr,
	&up_threshold_hotplug1.attr,			// ZZ: added tuneable
	&up_threshold_hotplug2.attr,			// ZZ: added tuneable
	&up_threshold_hotplug3.attr,			// ZZ: added tuneable
	&down_threshold.attr,
	&down_threshold_sleep.attr,			// ZZ: added tuneable
	&down_threshold_hotplug1.attr,			// ZZ: added tuneable
	&down_threshold_hotplug2.attr,			// ZZ: added tuneable
	&down_threshold_hotplug3.attr,			// ZZ: added tuneable
	&ignore_nice_load.attr,
	&freq_step.attr,
	&freq_step_sleep.attr,				// ZZ: added tuneable
	&smooth_up.attr,
	&smooth_up_sleep.attr,				// ZZ: added tuneable
	&up_threshold.attr,
	&up_threshold_sleep.attr,			// ZZ: added tuneable
	&hotplug_sleep.attr,				// ZZ: added tuneable
	&freq_limit.attr,				// ZZ: added tuneable
	&freq_limit_sleep.attr,				// ZZ: added tuneable
	&fast_scaling.attr,				// ZZ: added tuneable
	&fast_scaling_sleep.attr,			// ZZ: added tuneable
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "zzmoove",
};

/************************** sysfs end ************************/

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int load = 0;
	unsigned int max_load = 0;
	struct cpufreq_policy *policy;
	unsigned int j;

	policy = this_dbs_info->cur_policy;
	
	/*
	 * ZZ: Frequency Limit: we try here at a verly early stage to limit freqencies above limit by setting the current target_freq to freq_limit.
	 * This could be for example wakeup or touchboot freqencies which could be above the limit and are out of governors control(?) 
	 * This can not avoid the incrementation to these frequencies but should bring it down again earlier. Not sure if that is
	 * a good way to do that or if its working. Just an idea - maybe a remove candidate!
	 */
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit)
	__cpufreq_driver_target(policy, dbs_tuners_ins.freq_limit, CPUFREQ_RELATION_L);

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate*sampling_down_factor, we check, if current
	 * idle time is more than 80%, then we try to decrease frequency
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of maximum frequency
	 */

	/* Get Absolute Load */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
					 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		if (load > max_load)
			max_load = load;
	}

	/*
	 * break out if we 'cannot' reduce the speed as the user might
	 * want freq_step to be zero
	 */
	if (dbs_tuners_ins.freq_step == 0)
		return;

	/* 
	 * zzmoove v0.1 - Modification by ZaneZam November 2012
	 * Check for frequency increase is greater than hotplug up threshold value and wake up cores accordingly
	 * Following will bring up 3 cores in a row (cpu0 stays always on!)
	 * 
	 * zzmoove v0.2 - changed hotplug logic to be able to tune up threshold per core and to be able to set
	 * cores offline manually via sysfs
	 */
	
	if (num_online_cpus() < 2) {
			if (dbs_tuners_ins.up_threshold_hotplug1 != 0 || dbs_tuners_ins.up_threshold_hotplug2 != 0 || dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (dbs_tuners_ins.up_threshold_hotplug1 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug1)
			cpu_up(1);
			if (dbs_tuners_ins.up_threshold_hotplug2 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug2)
			cpu_up(2);
			if (dbs_tuners_ins.up_threshold_hotplug3 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug3)
			cpu_up(3);
			if (dbs_tuners_ins.up_threshold_hotplug1 != 0 || dbs_tuners_ins.up_threshold_hotplug2 != 0 || dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	} else if (num_online_cpus() < 3) {
			if (dbs_tuners_ins.up_threshold_hotplug2 != 0 || dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (dbs_tuners_ins.up_threshold_hotplug2 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug2)
			cpu_up(2);
			if (dbs_tuners_ins.up_threshold_hotplug3 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug3)
			cpu_up(3);
			if (dbs_tuners_ins.up_threshold_hotplug2 != 0 || dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	} else if (num_online_cpus() < 4) {
			if (dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (dbs_tuners_ins.up_threshold_hotplug3 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug3)
			cpu_up(3);
			if (dbs_tuners_ins.up_threshold_hotplug3 != 0) // ZZ: don't mutex if cores are not enabled
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	}
	
	/* Check for frequency increase */
	if (max_load > dbs_tuners_ins.up_threshold) {
		this_dbs_info->down_skip = 0;

		/* if we are already at full speed then break out early */
		if (this_dbs_info->requested_freq == policy->max)
			return;

	/* ZZ: Frequency Limit: if we are at freq_limit break out early */
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur == dbs_tuners_ins.freq_limit)
	return;

	/* ZZ: Frequency Limit: try to strictly hold down freqency at freq_limit by spoofing requested freq */
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit) {
	this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;
	
	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);
	return;

	/* ZZ: Frequency Limit: but let it scale up as normal if the freqencies are lower freq_limit */
	} else if (dbs_tuners_ins.freq_limit != 0 && policy->cur < dbs_tuners_ins.freq_limit) {
	    if (dbs_tuners_ins.fast_scaling == 1) // ZZ: use fast scaling columns
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, ZZ_UP, max_load);
	    else
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_UP, max_load);
	
	    /* ZZ: check again if we are above limit because of fast scaling */
	    if (dbs_tuners_ins.freq_limit != 0 && this_dbs_info->requested_freq > dbs_tuners_ins.freq_limit)
		this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;
	
	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);
	return;
	}
	
        if (dbs_tuners_ins.fast_scaling == 1) // ZZ: use fast scaling columns
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, ZZ_UP, max_load);
    	    else
    		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_UP, max_load);

	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			CPUFREQ_RELATION_H);
	return;
	}
    
	/* 
	 * zzmoove v0.1 - Modification by ZaneZam November 2012
	 * Check for frequency decrease is lower than hotplug value and put cores to sleep accordingly
	 * Following will disable 3 cores in a row (cpu0 is always on!)
	 * 
	 * zzmoove v0.2 - changed logic to be able to tune down threshold per core via sysfs
	 */
	
	if (num_online_cpus() > 3) {
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (max_load < dbs_tuners_ins.down_threshold_hotplug3)
			cpu_down(3);
			if (max_load < dbs_tuners_ins.down_threshold_hotplug2)
			cpu_down(2);
			if (max_load < dbs_tuners_ins.down_threshold_hotplug1)
			cpu_down(1);
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	} else if (num_online_cpus() > 2) {
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (max_load < dbs_tuners_ins.down_threshold_hotplug2)
			cpu_down(2);
			if (max_load < dbs_tuners_ins.down_threshold_hotplug1)
			cpu_down(1);
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	} else if (num_online_cpus() > 1) {
			mutex_unlock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
			if (max_load < dbs_tuners_ins.down_threshold_hotplug1)
			cpu_down(1);
			mutex_lock(&this_dbs_info->timer_mutex); // ZZ: this seems to be a very good idea, without it lockups are possible!
	}

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load < (dbs_tuners_ins.down_threshold - 10)) {

		/*
		 * if we cannot reduce the frequency anymore, break out early
		 */
		if (policy->cur == policy->min)
			return;

	/* ZZ: Frequency Limit: this should bring down freqency faster if we are coming from above limit (eg. touchboost/wakeup freqencies)*/ 
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit) {
	this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;
	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_L);
	return;

	/* ZZ: Frequency Limit: else we scale down as usual */
	} else if (dbs_tuners_ins.freq_limit != 0 && policy->cur <= dbs_tuners_ins.freq_limit) {
	    if (dbs_tuners_ins.fast_scaling == 1) // ZZ: use fast scaling columns
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, ZZ_DOWN, max_load);
	    else
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_DOWN, max_load);
	
	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_L); // ZZ: changed to relation low 
	return;
	}

        if (dbs_tuners_ins.fast_scaling == 1) // ZZ: use fast scaling columns
    		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, ZZ_DOWN, max_load);
    	    else
    		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_DOWN, max_load);

	__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_L);
	return;
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	delay -= jiffies % delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);

	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	delay -= jiffies % delay;

	dbs_info->enable = 1;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	dbs_info->enable = 0;
	cancel_delayed_work_sync(&dbs_info->work);
}

static void powersave_early_suspend(struct early_suspend *handler)
{
  mutex_lock(&dbs_mutex);
  sampling_rate_awake = dbs_tuners_ins.sampling_rate;
  up_threshold_awake = dbs_tuners_ins.up_threshold;
  down_threshold_awake = dbs_tuners_ins.down_threshold;
  smooth_up_awake = dbs_tuners_ins.smooth_up;
  freq_step_awake = dbs_tuners_ins.freq_step;				// ZZ: save freq step
  freq_limit_awake = dbs_tuners_ins.freq_limit;				// ZZ: save freq limit
  fast_scaling_awake = dbs_tuners_ins.fast_scaling;			// ZZ: save scaling setting

  if (dbs_tuners_ins.hotplug_sleep != 0) {				// ZZ: if set to 0 do not touch hotplugging values
	hotplug1_awake = dbs_tuners_ins.up_threshold_hotplug1;		// ZZ: save hotplug1 value for restore on awake
	hotplug2_awake = dbs_tuners_ins.up_threshold_hotplug2;		// ZZ: save hotplug2 value for restore on awake
	hotplug3_awake = dbs_tuners_ins.up_threshold_hotplug3;		// ZZ: save hotplug3 value for restore on awake
  }

  sampling_rate_asleep = dbs_tuners_ins.sampling_rate_sleep_multiplier; // ZZ: save sleep multiplier
  up_threshold_asleep = dbs_tuners_ins.up_threshold_sleep;		// ZZ: save up threshold
  down_threshold_asleep = dbs_tuners_ins.down_threshold_sleep;		// ZZ: save down threshold
  smooth_up_asleep = dbs_tuners_ins.smooth_up_sleep;			// ZZ: save smooth up
  freq_step_asleep = dbs_tuners_ins.freq_step_sleep;			// ZZ: save frequency step
  freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep;			// ZZ: save frequency limit
  fast_scaling_asleep = dbs_tuners_ins.fast_scaling_sleep;		// ZZ: save fast scaling
  dbs_tuners_ins.sampling_rate *= sampling_rate_asleep;			// ZZ: set sampling rate
  dbs_tuners_ins.up_threshold = up_threshold_asleep;			// ZZ: set up threshold
  dbs_tuners_ins.down_threshold = down_threshold_asleep;		// ZZ: set down threshold
  dbs_tuners_ins.smooth_up = smooth_up_asleep;				// ZZ: set smooth up
  dbs_tuners_ins.freq_step = freq_step_asleep;				// ZZ: set freqency step
  dbs_tuners_ins.freq_limit = freq_limit_asleep;			// ZZ: set freqency limit
  dbs_tuners_ins.fast_scaling = fast_scaling_asleep;			// ZZ: set fast scaling

  if (dbs_tuners_ins.hotplug_sleep != 0) {				// ZZ: if set to 0 do not touch hotplugging values
	if (dbs_tuners_ins.hotplug_sleep == 1) {			
	    dbs_tuners_ins.up_threshold_hotplug1 = 0;			// ZZ: set to one core
    	    dbs_tuners_ins.up_threshold_hotplug2 = 0;			// ZZ: set to one core
    	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to one core
	}
	if (dbs_tuners_ins.hotplug_sleep == 2) {			
	    dbs_tuners_ins.up_threshold_hotplug2 = 0;			// ZZ: set to two cores
	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to two cores
	} 
	if (dbs_tuners_ins.hotplug_sleep == 3) {			
	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to three cores
	}
  }
  mutex_unlock(&dbs_mutex);
}

static void powersave_late_resume(struct early_suspend *handler)
{
  mutex_lock(&dbs_mutex);
  dbs_tuners_ins.sampling_rate = sampling_rate_awake;		// ZZ: restore previous settings
  dbs_tuners_ins.up_threshold = up_threshold_awake;		// ZZ: restore previous settings
  dbs_tuners_ins.down_threshold = down_threshold_awake;		// ZZ: restore previous settings
  dbs_tuners_ins.smooth_up = smooth_up_awake;			// ZZ: restore previous settings
  dbs_tuners_ins.freq_step = freq_step_awake;			// ZZ: restore previous settings
  dbs_tuners_ins.freq_limit = freq_limit_awake;			// ZZ: restore previous settings
  dbs_tuners_ins.fast_scaling = fast_scaling_awake;		// ZZ: restore previous settings
   if (dbs_tuners_ins.hotplug_sleep != 0) {
	dbs_tuners_ins.up_threshold_hotplug1 = hotplug1_awake;	// ZZ: restore previous settings
	dbs_tuners_ins.up_threshold_hotplug2 = hotplug2_awake;	// ZZ: restore previous settings
	dbs_tuners_ins.up_threshold_hotplug3 = hotplug3_awake;	// ZZ: restore previous settings
  }
  mutex_unlock(&dbs_mutex);
}

static struct early_suspend _powersave_early_suspend = {
  .suspend = powersave_early_suspend,
  .resume = powersave_late_resume,
  .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(cs_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}
		}
		this_dbs_info->down_skip = 0;
		this_dbs_info->requested_freq = policy->cur;

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_enable++;
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;
			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/*
			 * conservative does not implement micro like ondemand
			 * governor, thus we are bound to jiffes/HZ
			 */
			min_sampling_rate =
				MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(3);
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			sampling_rate_awake = dbs_tuners_ins.sampling_rate;
			up_threshold_awake = dbs_tuners_ins.up_threshold;
			down_threshold_awake = dbs_tuners_ins.down_threshold;
			smooth_up_awake = dbs_tuners_ins.smooth_up;
			cpufreq_register_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
		}
		mutex_unlock(&dbs_mutex);

		dbs_timer_init(this_dbs_info);
        register_early_suspend(&_powersave_early_suspend);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		dbs_enable--;
		mutex_destroy(&this_dbs_info->timer_mutex);

		/*
		 * Stop the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 0)
			cpufreq_unregister_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);

		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

        unregister_early_suspend(&_powersave_early_suspend);
		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);

		break;
	}
	return 0;
}

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
static
#endif
struct cpufreq_governor cpufreq_gov_zzmoove = {
	.name			= "zzmoove",
	.governor		= cpufreq_governor_dbs,
	.max_transition_latency	= TRANSITION_LATENCY_LIMIT,
	.owner			= THIS_MODULE,
};

static int __init cpufreq_gov_dbs_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_zzmoove);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_zzmoove);
}

/*
 * zzmoove governor is based on the modified conservative (original author
 * Alexander Clouter <alex@digriz.org.uk>) smoove governor from Michael 
 * Weingaertner <mialwe@googlemail.com> (source: https://github.com/mialwe/mngb/)
 * Modified by Zane Zaminsky November 2012 to be hotplug-able and optimzed for use 
 * with Samsung I9300. CPU Hotplug modifications partially taken from ktoonservative 
 * governor from ktoonsez KT747-JB kernel (https://github.com/ktoonsez/KT747-JB)
 */

MODULE_AUTHOR("Zane Zaminsky <cyxman@yahoo.com>");
MODULE_DESCRIPTION("'cpufreq_zzmoove' - A dynamic cpufreq governor based "
		"on smoove governor from Michael Weingaertner which was originally based on "
		"cpufreq_conservative from Alexander Clouter. Optimized for use with Samsung I9300 "
		"using frequency lookup tables and CPU hotplug - ported/modified for I9300 "
		"by ZaneZam November 2012");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
