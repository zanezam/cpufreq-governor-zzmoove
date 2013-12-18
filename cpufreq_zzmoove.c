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
 * - ZZMoove Governor Desktop Edition v0.1 alpha by ZaneZam 2012/13 Changelog:                                                                                                                          -
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Version 0.1 - first release
 *
 *	- codebase latest zzmoove governor version 0.6 final
 *	- removed lcdfreq scaling and early suspend support
 *	- changed load calculation to be compatible with 3.7 kernel
 *	- changed back freq limit code to stock conservative
 *
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

// cpu load trigger
#define DEF_SMOOTH_UP (75)

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

// ZZ: midnight and zzmoove default values
#define DEF_FREQUENCY_UP_THRESHOLD		(70)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG	(68)	// ZZ: default for hotplug up threshold for all cpu's (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD		(52)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG	(55)	// ZZ: default for hotplug down threshold for all cpu's (cpu0 stays allways on)
#define DEF_IGNORE_NICE				(0)	// ZZ: default for ignore nice load
#define DEF_FREQ_STEP				(5)	// ZZ: default for freq step at awake

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

// ZZ: Sampling down momentum variables
static unsigned int min_sampling_rate;			// ZZ: minimal possible sampling rate
static unsigned int orig_sampling_down_factor;		// ZZ: for saving previously set sampling down factor
static unsigned int orig_sampling_down_max_mom;		// ZZ: for saving previously set smapling down max momentum

// ZZ: search limit for frequencies in scaling array, variables for scaling modes and state flags for deadlock fix/suspend detection
static unsigned int max_scaling_freq_soft = 0;		// ZZ: init value for "soft" scaling = 0 full range
static unsigned int max_scaling_freq_hard = 0;		// ZZ: init value for "hard" scaling = 0 full range
static unsigned int skip_hotplug_flag = 0;		// ZZ: initial start with hotplugging
static int scaling_mode_up;				// ZZ: fast scaling up mode holding up value during runtime
static int scaling_mode_down;				// ZZ: fast scaling down mode holding down value during runtime

// ZZ: current load for hotplugging work
static int cur_load = 0;

// ZZ: hotplug threshold array
static int hotplug_thresholds[2][8]={
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
    };

// ZZ: support for 2,4 or 8 cores
#define MAX_CORES		(8)

// raise sampling rate to SR*multiplier and adjust sampling rate/thresholds/hotplug/scaling/freq limit/freq step on blank screen

// ZZ: midnight and zzmoove momentum defaults
#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define DEF_SAMPLING_DOWN_FACTOR		(1)	// ZZ: default for sampling down factor (stratosk default = 4) here disabled by default
#define MAX_SAMPLING_DOWN_FACTOR		(100000)// ZZ: changed from 10 to 100000 for sampling down momentum implementation
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

// ZZ: Sampling down momentum
#define DEF_SAMPLING_DOWN_MOMENTUM		(0)	// ZZ: sampling down momentum disabled by default
#define DEF_SAMPLING_DOWN_MAX_MOMENTUM	 	(0)	// ZZ: default for tuneable sampling_down_max_momentum stratosk default=16, here disabled by default
#define DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY  (50)	// ZZ: default for tuneable sampling_down_momentum_sensitivity
#define MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY  (1000)	// ZZ: max value for tuneable sampling_down_momentum_sensitivity
#define DEF_GRAD_UP_THRESHOLD			(25)	// ZZ: default for grad up threshold

/*
* ZZ: Frequency Limit: 0 do not limit frequency and use the full range up to cpufreq->max limit
* values 200000 -> 1800000 khz
*/

#define DEF_FREQ_LIMIT				(0)	// ZZ: default for tuneable freq_limit

/*
* ZZ: Fast Scaling: 0 do not activate fast scaling function
* values 1-4 to enable fast scaling with normal down scaling 5-8 to enable fast scaling with fast up and down scaling
*/

#define DEF_FAST_SCALING			(0)	// ZZ: default for tuneable fast_scaling

struct work_struct hotplug_offline_work;
struct work_struct hotplug_online_work;

static void do_dbs_timer(struct work_struct *work);

struct cpu_dbs_info_s {
	u64 time_in_idle; 				// ZZ: added exit time handling
	u64 idle_exit_time;				// ZZ: added exit time handling
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	unsigned int down_skip;				// ZZ: Smapling down reactivated
	unsigned int check_cpu_skip;			// ZZ: check_cpu skip counter (to avoid potential deadlock because of double locks from hotplugging)
	unsigned int requested_freq;
	unsigned int rate_mult;				// ZZ: Sampling down momentum sampling rate multiplier
	unsigned int momentum_adder;			// ZZ: Sampling down momentum adder
	int cpu;
	unsigned int enable:1;
	unsigned int prev_load;				// ZZ: Early demand var for previous load
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
	unsigned int sampling_down_factor;		// ZZ: Sampling down factor (reactivated)
	unsigned int sampling_down_momentum;		// ZZ: Sampling down momentum tuneable
	unsigned int sampling_down_max_mom;		// ZZ: Sampling down momentum max tuneable
	unsigned int sampling_down_mom_sens;		// ZZ: Sampling down momentum sensitivity
	unsigned int up_threshold;
	unsigned int up_threshold_hotplug1;		// ZZ: added tuneable up_threshold_hotplug1 for core1
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int up_threshold_hotplug2;		// ZZ: added tuneable up_threshold_hotplug2 for core2
	unsigned int up_threshold_hotplug3;		// ZZ: added tuneable up_threshold_hotplug3 for core3
#endif
#if (MAX_CORES == 8)
	unsigned int up_threshold_hotplug4;		// ZZ: added tuneable up_threshold_hotplug4 for core4
	unsigned int up_threshold_hotplug5;		// ZZ: added tuneable up_threshold_hotplug5 for core5
	unsigned int up_threshold_hotplug6;		// ZZ: added tuneable up_threshold_hotplug6 for core6
	unsigned int up_threshold_hotplug7;		// ZZ: added tuneable up_threshold_hotplug7 for core7
#endif
	unsigned int down_threshold;
	unsigned int down_threshold_hotplug1;		// ZZ: added tuneable down_threshold_hotplug1 for core1
	unsigned int down_threshold_hotplug2;		// ZZ: added tuneable down_threshold_hotplug2 for core2
	unsigned int down_threshold_hotplug3;		// ZZ: added tuneable down_threshold_hotplug3 for core3
#if (MAX_CORES == 8)
	unsigned int down_threshold_hotplug4;		// ZZ: added tuneable down_threshold_hotplug4 for core4
	unsigned int down_threshold_hotplug5;		// ZZ: added tuneable down_threshold_hotplug5 for core5
	unsigned int down_threshold_hotplug6;		// ZZ: added tuneable down_threshold_hotplug6 for core6
	unsigned int down_threshold_hotplug7;		// ZZ: added tuneable down_threshold_hotplug7 for core7
#endif
	unsigned int ignore_nice;
	unsigned int freq_step;
	unsigned int smooth_up;
	unsigned int freq_limit;			// ZZ: added tuneable freq_limit
	unsigned int fast_scaling;			// ZZ: added tuneable fast_scaling
	unsigned int grad_up_threshold;			// ZZ: Early demand grad up threshold tuneable
	unsigned int early_demand;			// ZZ: Early demand master switch
	unsigned int disable_hotplug;			// ZZ: Hotplug switch

} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.up_threshold_hotplug1 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
	.up_threshold_hotplug2 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug3 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
#endif
#if (MAX_CORES == 8)
	.up_threshold_hotplug4 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug5 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug6 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
	.up_threshold_hotplug7 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,		// ZZ: set default value for new tuneable
#endif
	.down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD,
	.down_threshold_hotplug1 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
	.down_threshold_hotplug2 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug3 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
#endif
#if (MAX_CORES == 8)
	.down_threshold_hotplug4 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug5 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug6 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
	.down_threshold_hotplug7 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,	// ZZ: set default value for new tuneable
#endif
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,			// ZZ: sampling down reactivated but disabled by default
	.sampling_down_momentum = DEF_SAMPLING_DOWN_MOMENTUM,			// ZZ: Sampling down momentum initial disabled
	.sampling_down_max_mom = DEF_SAMPLING_DOWN_MAX_MOMENTUM,		// ZZ: Sampling down momentum default for max momentum
	.sampling_down_mom_sens = DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY,	// ZZ: Sampling down momentum default for sensitivity
	.ignore_nice = DEF_IGNORE_NICE,						// ZZ: set default value for tuneable
	.freq_step = DEF_FREQ_STEP,						// ZZ: set default value for new tuneable
	.smooth_up = DEF_SMOOTH_UP,
	.freq_limit = DEF_FREQ_LIMIT,						// ZZ: set default value for new tuneable
	.fast_scaling = DEF_FAST_SCALING,					// ZZ: set default value for new tuneable
	.grad_up_threshold = DEF_GRAD_UP_THRESHOLD,				// ZZ: Early demand default for grad up threshold
	.early_demand = 0,							// ZZ: Early demand default off
	.disable_hotplug = false,						// ZZ: Hotplug switch default off (hotplugging on)
	};

unsigned int freq_table_size = 0;						// Yank : lowest frequency index in global frequency table

/**
 * Smooth scaling conservative governor (by Michael Weingaertner)
 * ------------------------------------------------------------------------- obsolete
 * This modification makes the governor use two lookup tables holding
 * current, next and previous frequency to directly get a correct
 * target frequency instead of calculating target frequencies with
 * up_threshold and step_up %. The two scaling lookup tables used
 * contain different scaling steps/frequencies to achieve faster upscaling
 * on higher CPU load.
 * ------------------------------------------------------------------------- obsolete
 * CPU load triggering faster upscaling can be adjusted via SYSFS,
 * VALUE between 1 and 100 (% CPU load):
 * echo VALUE > /sys/devices/system/cpu/cpufreq/zzmoove/smooth_up
 *
 * improved by Zane Zaminsky and Yank555 2012/13
 */

#define SCALE_FREQ_UP 1
#define SCALE_FREQ_DOWN 2

/*
 * Table modified for use with Samsung I9300 by ZaneZam November 2012
 * zzmoove v0.3 	- table modified to reach overclocking frequencies up to 1600mhz
 * zzmoove v0.4 	- added fast scaling columns to frequency table
 * zzmoove v0.5 	- removed fast scaling colums and use line jumps instead. 4 steps and 2 modes (with/without fast downscaling) possible now
 *                	  table modified to reach overclocking frequencies up to 1800mhz
 *                	  fixed wrong frequency stepping
 *                	  added search limit for more efficent frequency searching and better hard/softlimit handling
 * zzmoove v0.5.1b 	- combination of power and normal scaling table to only one array (idea by Yank555)
 *                 	- scaling logic reworked and optimized by Yank555
 * zzmoove v0.6 	- removed lookup tables complete and use system frequency table instead (credits to Yank555)
 *
 */

static int mn_get_next_freq(unsigned int curfreq, unsigned int updown, unsigned int load) {

	int i=0;
	int smooth_up_steps=0; 			// Yank : smooth_up steps
	struct cpufreq_frequency_table *table;	// Yank : Use system frequency table

	table = cpufreq_frequency_get_table(0);	// Yank : Get system frequency table
    
	if (load < dbs_tuners_ins.smooth_up) 	// Yank : Consider smooth up
		smooth_up_steps=0;		//          load not reached, move by one step
	else
		smooth_up_steps=1;		//          load reached, move by two steps

	for(i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) {

		if(curfreq == table[i].frequency) {

			// Yank : We found where we currently are (i)

			if(updown == SCALE_FREQ_UP)

				return table[max((int) max_scaling_freq_soft, i - 1 - smooth_up_steps - scaling_mode_up  )].frequency;	// Yank : Scale up, but stay below softlimit
				
			else

				return table[min((int) freq_table_size      , i + 1                   + scaling_mode_down)].frequency;	// Yank : Scale down, but stay on valid lowest index

			return (curfreq);	// Yank : We should never get here...

		}

	}
    
	return (curfreq); // not found

}

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
    u64 idle_time;
    u64 cur_wall_time;
    u64 busy_time;
    cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
    busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

    idle_time = cur_wall_time - busy_time;
    if (wall)
    *wall = jiffies_to_usecs(cur_wall_time);
    return jiffies_to_usecs(idle_time);
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
	 * the 'valid' ranges of frequency available to us otherwise
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
show_one(sampling_down_factor, sampling_down_factor);				// ZZ: reactivated sampling down factor
show_one(sampling_down_max_momentum, sampling_down_max_mom);			// ZZ: added Sampling down momentum tuneable
show_one(sampling_down_momentum_sensitivity, sampling_down_mom_sens);		// ZZ: added Sampling down momentum tuneable
show_one(up_threshold, up_threshold);
show_one(up_threshold_hotplug1, up_threshold_hotplug1);				// ZZ: added up_threshold_hotplug1 tuneable for cpu1
#if (MAX_CORES == 4 || MAX_CORES == 8)
show_one(up_threshold_hotplug2, up_threshold_hotplug2);				// ZZ: added up_threshold_hotplug2 tuneable for cpu2
show_one(up_threshold_hotplug3, up_threshold_hotplug3);				// ZZ: added up_threshold_hotplug3 tuneable for cpu3
#endif
#if (MAX_CORES == 8)
show_one(up_threshold_hotplug4, up_threshold_hotplug4);				// ZZ: added up_threshold_hotplug4 tuneable for cpu1
show_one(up_threshold_hotplug5, up_threshold_hotplug5);				// ZZ: added up_threshold_hotplug5 tuneable for cpu2
show_one(up_threshold_hotplug6, up_threshold_hotplug6);				// ZZ: added up_threshold_hotplug6 tuneable for cpu3
show_one(up_threshold_hotplug7, up_threshold_hotplug7);				// ZZ: added up_threshold_hotplug7 tuneable for cpu3
#endif
show_one(down_threshold, down_threshold);
show_one(down_threshold_hotplug1, down_threshold_hotplug1);			// ZZ: added down_threshold_hotplug1 tuneable for cpu1
#if (MAX_CORES == 4 || MAX_CORES == 8)
show_one(down_threshold_hotplug2, down_threshold_hotplug2);			// ZZ: added down_threshold_hotplug2 tuneable for cpu2
show_one(down_threshold_hotplug3, down_threshold_hotplug3);			// ZZ: added down_threshold_hotplug3 tuneable for cpu3
#endif
#if (MAX_CORES == 8)
show_one(down_threshold_hotplug4, down_threshold_hotplug4);			// ZZ: added down_threshold_hotplug4 tuneable for cpu1
show_one(down_threshold_hotplug5, down_threshold_hotplug5);			// ZZ: added down_threshold_hotplug5 tuneable for cpu2
show_one(down_threshold_hotplug6, down_threshold_hotplug6);			// ZZ: added down_threshold_hotplug6 tuneable for cpu3
show_one(down_threshold_hotplug7, down_threshold_hotplug7);			// ZZ: added down_threshold_hotplug7 tuneable for cpu3
#endif
show_one(ignore_nice_load, ignore_nice);
show_one(freq_step, freq_step);
show_one(smooth_up, smooth_up);
show_one(freq_limit, freq_limit);						// ZZ: added freq_limit tuneable
show_one(fast_scaling, fast_scaling);						// ZZ: added fast_scaling tuneable
show_one(grad_up_threshold, grad_up_threshold);					// ZZ: added Early demand tuneable grad up threshold
show_one(early_demand, early_demand);						// ZZ: added Early demand tuneable master switch
show_one(disable_hotplug, disable_hotplug);					// ZZ: added Hotplug switch

// ZZ: added tuneable for Sampling down momentum -> possible values: 0 (disable) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 0
static ssize_t store_sampling_down_max_momentum(struct kobject *a,
		struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR -
	dbs_tuners_ins.sampling_down_factor || input < 0)
	return -EINVAL;

	dbs_tuners_ins.sampling_down_max_mom = input;
	orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;

	/* ZZ: Reset sampling down factor to default if momentum was disabled */
	if (dbs_tuners_ins.sampling_down_max_mom == 0)
	dbs_tuners_ins.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;

	/* Reset momentum_adder and reset down sampling multiplier in case momentum was disabled */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	    if (dbs_tuners_ins.sampling_down_max_mom == 0)
	    dbs_info->rate_mult = 1;
	}

return count;
}

// ZZ: added tuneable for Sampling down momentum -> possible values: 1 to MAX_SAMPLING_DOWN_SENSITIVITY, if not set default is 50
static ssize_t store_sampling_down_momentum_sensitivity(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY || input < 1)
	return -EINVAL;
	
	dbs_tuners_ins.sampling_down_mom_sens = input;

	/* Reset momentum_adder */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	}

return count;
}

// ZZ: Sampling down factor (reactivated) added reset loop for momentum functionality -> possible values: 1 (disabled) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 1
static ssize_t store_sampling_down_factor(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_down_factor = input;

	/* ZZ: Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->rate_mult = 1;
	}

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
	hotplug_thresholds[0][0] = input;
	return count;
}
#if (MAX_CORES == 4 || MAX_CORES == 8)
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
	hotplug_thresholds[0][1] = input;
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
	hotplug_thresholds[0][2] = input;
	return count;
}
#endif
#if (MAX_CORES == 8)
// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug4(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug4 = input;
	hotplug_thresholds[0][3] = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug5(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug5 = input;
	hotplug_thresholds[0][4] = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug6(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug6 = input;
	hotplug_thresholds[0][5] = input;
	return count;
}

// ZZ: added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug7(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug7 = input;
	hotplug_thresholds[0][6] = input;
	return count;
}
#endif
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
	hotplug_thresholds[1][0] = input;
	return count;
}
#if (MAX_CORES == 4 || MAX_CORES == 8)
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
	hotplug_thresholds[1][1] = input;
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
	hotplug_thresholds[1][2] = input;
	return count;
}
#endif
#if (MAX_CORES == 8)
// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug4(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug4 = input;
	hotplug_thresholds[1][3] = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug5(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug5 = input;
	hotplug_thresholds[1][4] = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug6(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug6 = input;
	hotplug_thresholds[1][5] = input;
	return count;
}

// ZZ: added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug7(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug7 = input;
	hotplug_thresholds[1][6] = input;
	return count;
}
#endif

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
		dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
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

// ZZ: added tuneable -> possible values: 0 disable, system table freq->min to freq->max in khz -> freqency soft-limit, if not set default is 0
// Yank: updated : possible values now depend on the system frequency table only
static ssize_t store_freq_limit(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	struct cpufreq_frequency_table *table;	// Yank : Use system frequency table
	int ret;
	int i=0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;
	
	if (input == 0) {
	     max_scaling_freq_soft = max_scaling_freq_hard;
	     dbs_tuners_ins.freq_limit = input;
	     return count;
	}

	table = cpufreq_frequency_get_table(0);	// Yank : Get system frequency table
	
	if (!table) {
		return -EINVAL;
	} else {
		for (i = max_scaling_freq_hard; (table[i].frequency != CPUFREQ_TABLE_END); i++) // Yank : Allow only frequencies below or equal to hard max
			if (table[i].frequency == input) {
				max_scaling_freq_soft = i;
				dbs_tuners_ins.freq_limit = input;
				return count;
			}
	}

	return -EINVAL;
}

// ZZ: added tuneable -> possible values: 0 disable, 1-4 number of scaling jumps only for upscaling, 5-8 equivalent to 1-4 for up and down scaling, if not set default is 0
static ssize_t store_fast_scaling(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 8 || input < 0)
		return -EINVAL;

	dbs_tuners_ins.fast_scaling = input;

	if (input > 4) {
	    scaling_mode_up   = input - 4;	// Yank : fast scaling up
	    scaling_mode_down = input - 4;	// Yank : fast scaling down

	} else {
	    scaling_mode_up   = input;		// Yank : fast scaling up only
	    scaling_mode_down = 0;
	}
	return count;
}

// ZZ: Early demand - added tuneable grad up threshold -> possible values: from 11 to 100, if not set default is 50
static ssize_t store_grad_up_threshold(struct kobject *a, struct attribute *b,
						const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 11)
	return -EINVAL;

	dbs_tuners_ins.grad_up_threshold = input;
	return count;
}

// ZZ: Early demand - added tuneable master switch -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_early_demand(struct kobject *a, struct attribute *b,
					    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
	return -EINVAL;
	
	dbs_tuners_ins.early_demand = !!input;
	return count;
}

// ZZ: added tuneable hotplug switch for debugging and testing purposes -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_disable_hotplug(struct kobject *a, struct attribute *b,
					    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
	return -EINVAL;

	if (input > 0) {
		dbs_tuners_ins.disable_hotplug = true;
			for (i = 1; i < num_possible_cpus(); i++) { 		// ZZ: enable all offline cores
			if (!cpu_online(i))
			cpu_up(i);
			}
	} else {
		dbs_tuners_ins.disable_hotplug = false;
	}
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(sampling_down_factor);			// ZZ: Sampling down factor (reactived)
define_one_global_rw(sampling_down_max_momentum);		// ZZ: Sampling down momentum tuneable
define_one_global_rw(sampling_down_momentum_sensitivity);	// ZZ: Sampling down momentum tuneable
define_one_global_rw(up_threshold);
define_one_global_rw(up_threshold_hotplug1);			// ZZ: added tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
define_one_global_rw(up_threshold_hotplug2);			// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug3);			// ZZ: added tuneable
#endif
#if (MAX_CORES == 8)
define_one_global_rw(up_threshold_hotplug4);			// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug5);			// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug6);			// ZZ: added tuneable
define_one_global_rw(up_threshold_hotplug7);			// ZZ: added tuneable
#endif
define_one_global_rw(down_threshold);
define_one_global_rw(down_threshold_hotplug1);			// ZZ: added tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
define_one_global_rw(down_threshold_hotplug2);			// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug3);			// ZZ: added tuneable
#endif
#if (MAX_CORES == 8)
define_one_global_rw(down_threshold_hotplug4);			// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug5);			// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug6);			// ZZ: added tuneable
define_one_global_rw(down_threshold_hotplug7);			// ZZ: added tuneable
#endif
define_one_global_rw(ignore_nice_load);
define_one_global_rw(freq_step);
define_one_global_rw(smooth_up);
define_one_global_rw(freq_limit);				// ZZ: added tuneable
define_one_global_rw(fast_scaling);				// ZZ: added tuneable
define_one_global_rw(grad_up_threshold);			// ZZ: Early demand tuneable
define_one_global_rw(early_demand);				// ZZ: Early demand tuneable
define_one_global_rw(disable_hotplug);				// ZZ: Hotplug switch
static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&sampling_down_factor.attr,
	&sampling_down_max_momentum.attr,			// ZZ: Sampling down momentum tuneable
	&sampling_down_momentum_sensitivity.attr,		// ZZ: Sampling down momentum tuneable
	&up_threshold_hotplug1.attr,				// ZZ: added tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
	&up_threshold_hotplug2.attr,				// ZZ: added tuneable
	&up_threshold_hotplug3.attr,				// ZZ: added tuneable
#endif
#if (MAX_CORES == 8)
	&up_threshold_hotplug4.attr,				// ZZ: added tuneable
	&up_threshold_hotplug5.attr,				// ZZ: added tuneable
	&up_threshold_hotplug6.attr,				// ZZ: added tuneable
	&up_threshold_hotplug7.attr,				// ZZ: added tuneable
#endif
	&down_threshold.attr,
	&down_threshold_hotplug1.attr,				// ZZ: added tuneable
#if (MAX_CORES == 4 || MAX_CORES == 8)
	&down_threshold_hotplug2.attr,				// ZZ: added tuneable
	&down_threshold_hotplug3.attr,				// ZZ: added tuneable
#endif
#if (MAX_CORES == 8)
	&down_threshold_hotplug4.attr,				// ZZ: added tuneable
	&down_threshold_hotplug5.attr,				// ZZ: added tuneable
	&down_threshold_hotplug6.attr,				// ZZ: added tuneable
	&down_threshold_hotplug7.attr,				// ZZ: added tuneable
#endif
	&ignore_nice_load.attr,
	&freq_step.attr,
	&smooth_up.attr,
	&up_threshold.attr,
	&freq_limit.attr,					// ZZ: added tuneable
	&fast_scaling.attr,					// ZZ: added tuneable
	&grad_up_threshold.attr,				// ZZ: Early demand tuneable
	&early_demand.attr,					// ZZ: Early demand tuneable
	&disable_hotplug.attr,					// ZZ: Hotplug switch
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
	int boost_freq = 0; 					// ZZ: Early demand boost freq switch
	struct cpufreq_policy *policy;
	unsigned int j;
//	int i=0;
	
	policy = this_dbs_info->cur_policy;

	/*
	 * ZZ: Frequency Limit: we try here at a verly early stage to limit freqencies above limit by setting the current target_freq to freq_limit.
	 * This could be for example wakeup or touchboot freqencies which could be above the limit and are out of governors control.
	 * This can not avoid the incrementation to these frequencies but should bring it down again earlier. Not sure if that is
	 * a good way to do that or if its realy working. Just an idea - maybe a remove-candidate!
	 */
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit)
	__cpufreq_driver_target(policy, dbs_tuners_ins.freq_limit, CPUFREQ_RELATION_L);

	/*
	 * Every sampling_rate, we check, if current idle time is less than 20%
	 * (default), then we try to increase frequency. Every sampling_rate *
	 * sampling_down_factor, we check, if current idle time is more than 80%
	 * (default), then we try to decrease frequency.
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

	wall_time = (unsigned int)
	(cur_wall_time - j_dbs_info->prev_cpu_wall);
	j_dbs_info->prev_cpu_wall = cur_wall_time;

	idle_time = (unsigned int)
	(cur_idle_time - j_dbs_info->prev_cpu_idle);
	j_dbs_info->prev_cpu_idle = cur_idle_time;

	if (dbs_tuners_ins.ignore_nice) {
	    u64 cur_nice;
	    unsigned long cur_nice_jiffies;

	    cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
	    j_dbs_info->prev_cpu_nice;
	
	    /*
	     * Assumption: nice time between sampling periods will
	     * be less than 2^32 jiffies for 32 bit sys
	     */
	    cur_nice_jiffies = (unsigned long)
	    cputime64_to_jiffies64(cur_nice);
	
	    j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
	    idle_time += jiffies_to_usecs(cur_nice_jiffies);
	}
	
	if (unlikely(!wall_time || wall_time < idle_time))
	continue;

	load = 100 * (wall_time - idle_time) / wall_time;

	if (load > max_load) {
	    max_load = load;
	    cur_load = load; // ZZ: current load for hotplugging functions
	}

	/*
	 * ZZ: Early demand by stratosk
	 * Calculate the gradient of load_freq. If it is too steep we assume
	 * that the load will go over up_threshold in next iteration(s) and
	 * we increase the frequency immediately
	 */
	if (dbs_tuners_ins.early_demand) {
               if (max_load > this_dbs_info->prev_load &&
               (max_load - this_dbs_info->prev_load >
               dbs_tuners_ins.grad_up_threshold))
                  boost_freq = 1;
           this_dbs_info->prev_load = max_load;
           }
	}

	/*
	 * ZZ: reduction of possible deadlocks - we try here to avoid deadlocks due to double locking from hotplugging and timer mutex
	 * during start/stop/limit events. to be "sure" we skip here 25 times till the locks hopefully are unlocked again. yeah that's dirty
	 * but no better way found yet! ;)
	 */

	if (this_dbs_info->check_cpu_skip != 0) {
		if (++this_dbs_info->check_cpu_skip >= 15)
		    this_dbs_info->check_cpu_skip = 0;
		return;
	}
	
	/*
	 * break out if we 'cannot' reduce the speed as the user might
	 * want freq_step to be zero
	 */
	if (dbs_tuners_ins.freq_step == 0)
		return;
	
	/*
	 * zzmoove v0.1		- Modification by ZaneZam November 2012
	 *                	  Check for frequency increase is greater than hotplug up threshold value and wake up cores accordingly
	 *                	  Following will bring up 3 cores in a row (cpu0 stays always on!)
	 *
	 * zzmoove v0.2 	- changed hotplug logic to be able to tune up threshold per core and to be able to set
	 *                	  cores offline manually via sysfs
	 *
	 * zzmoove v0.5 	- fixed non switching cores at 0+2 and 0+3 situations
	 *              	- optimized hotplug logic by removing locks and skipping hotplugging if not needed
	 *              	- try to avoid deadlocks at critical events by using a flag if we are in the middle of hotplug decision
	 *
	 * zzmoove v0.5.1b 	- optimised hotplug logic by reducing code and concentrating only on essential parts
	 *                 	- preperation for automatic core detection
	 *
	 * zzmoove v0.6 	- reduced hotplug loop to a minimum
	 *
	 */

    if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != num_possible_cpus()) {
	    if (policy->cur != policy->min)
		        schedule_work_on(0, &hotplug_online_work);
	    	}

//	if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != num_possible_cpus()) {
//	for (i = 1; i < num_possible_cpus(); i++) {
//		if (!cpu_online(i) && skip_hotplug_flag == 0 && hotplug_thresholds[0][i-1] != 0 && max_load > hotplug_thresholds[0][i-1]) {
//			cpu_up(i);
//			printk(KERN_ALERT "ZZMOOVE DEBUG: Core %d enabled %s %d \n",i,__FUNCTION__,__LINE__);
//			printk(KERN_ALERT "ZZMOOVE DEBUG: skip hotplug flag was %d \n",skip_hotplug_flag);
//			}
//	    }
//	}
	
	/* Check for frequency increase */
	if (max_load > dbs_tuners_ins.up_threshold || boost_freq) { // ZZ: Early demand - added boost switch
	
	    /* ZZ: Sampling down momentum - if momentum is inactive switch to "down_skip" method */
	    if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1)
		this_dbs_info->down_skip = 0;

	    /* if we are already at full speed then break out early */
	    if (policy->cur == policy->max) // ZZ: changed check from reqested_freq to current freq (DerTeufel1980)
		return;

	    /* ZZ: Sampling down momentum - if momentum is active and we are switching to max speed, apply sampling_down_factor */
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && policy->cur < policy->max)
		this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;

	    /* ZZ: Frequency Limit: if we are at freq_limit break out early */
	    if (dbs_tuners_ins.freq_limit != 0 && policy->cur == dbs_tuners_ins.freq_limit)
		return;

	    /* ZZ: Frequency Limit: try to strictly hold down freqency at freq_limit by spoofing requested freq */
	    if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit) {
		this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

	    /* ZZ: check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
	    if (this_dbs_info->requested_freq > policy->max)
		 this_dbs_info->requested_freq = policy->max;
	
	    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);

		    /* ZZ: Sampling down momentum - calculate momentum and update sampling down factor */
		    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
			this_dbs_info->momentum_adder++;
			dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
			dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
		    }
		    return;

	    /* ZZ: Frequency Limit: but let it scale up as normal if the freqencies are lower freq_limit */
	    } else if (dbs_tuners_ins.freq_limit != 0 && policy->cur < dbs_tuners_ins.freq_limit) {
			this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, SCALE_FREQ_UP, max_load);
	
		    /* ZZ: check again if we are above limit because of fast scaling */
		    if (dbs_tuners_ins.freq_limit != 0 && this_dbs_info->requested_freq > dbs_tuners_ins.freq_limit)
			this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

		    /* ZZ: check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
		    if (this_dbs_info->requested_freq > policy->max)
		          this_dbs_info->requested_freq = policy->max;

		    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);

		    /* ZZ: Sampling down momentum - calculate momentum and update sampling down factor */
		    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
			this_dbs_info->momentum_adder++; 
			dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
			dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
		    }
		return;
	    }
	
	    this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, SCALE_FREQ_UP, max_load);

	    /* ZZ: check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
	    if (this_dbs_info->requested_freq > policy->max)
		 this_dbs_info->requested_freq = policy->max;

	    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			CPUFREQ_RELATION_H);

	    /* ZZ: Sampling down momentum - calculate momentum and update sampling down factor */
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
		this_dbs_info->momentum_adder++; 
		dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
		dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
	    }
	return;
	}

	/*
	 * zzmoove v0.1 	- Modification by ZaneZam November 2012
	 *                	  Check for frequency decrease is lower than hotplug value and put cores to sleep accordingly
	 *                	  Following will disable 3 cores in a row (cpu0 is always on!)
	 *
	 * zzmoove v0.2 	- changed logic to be able to tune down threshold per core via sysfs
	 *
	 * zzmoove v0.5 	- fixed non switching cores at 0+2 and 0+3 situations
	 *              	- optimized hotplug logic by removing locks and skipping hotplugging if not needed
	 *              	- try to avoid deadlocks at critical events by using a flag if we are in the middle of hotplug decision
	 *
	 * zzmoove 0.5.1b 	- optimised hotplug logic by reducing code and concentrating only on essential parts
	 *                	- preperation for automatic core detection
	 *
	 * zzmoove v0.6 	- reduced hotplug loop to a minimum.
	 *
	 */

    if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != 1)
	    schedule_work_on(0, &hotplug_offline_work);

//	if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != 1) {
//	for (i = num_possible_cpus() - 1; i >= 1; i--) {
//		if (cpu_online(i) && skip_hotplug_flag == 0 && max_load < hotplug_thresholds[1][i-1]) {
//		cpu_down(i);
//		printk(KERN_ALERT "ZZMOOVE DEBUG: Core %d disabled %s %d \n",i,__FUNCTION__,__LINE__);
//		printk(KERN_ALERT "ZZMOOVE DEBUG: skip hotplug flag was %d \n",skip_hotplug_flag);
//		}
//	    }
//	}

	/* ZZ: Sampling down momentum - if momentum is inactive switch to down skip method and if sampling_down_factor is active break out early */
	if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1) {
	    if (++this_dbs_info->down_skip < dbs_tuners_ins.sampling_down_factor)
		return;
	this_dbs_info->down_skip = 0;
	}
	
	/* ZZ: Sampling down momentum - calculate momentum and update sampling down factor */
	if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder > 1) {
	this_dbs_info->momentum_adder -= 2;
	dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
	dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
	}

	 /* Check for frequency decrease */
	if (max_load < dbs_tuners_ins.down_threshold) {

	    /* ZZ: Sampling down momentum - No longer fully busy, reset rate_mult */
	    this_dbs_info->rate_mult = 1;
		
		/* if we cannot reduce the frequency anymore, break out early */
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
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, SCALE_FREQ_DOWN, max_load);

		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_L); // ZZ: changed to relation low 
		return;
	}

    		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, SCALE_FREQ_DOWN, max_load);

		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_L); // ZZ: changed to relation low
		return;
	}
}

// ZZ: added function for hotplug down work
static void __cpuinit hotplug_offline_work_fn(struct work_struct *work)
{
    int i=0;
	
	    for (i = num_possible_cpus() - 1; i >= 1; i--) {
		    if (cpu_online(i) && skip_hotplug_flag == 0 && cur_load < hotplug_thresholds[1][i-1])
			    cpu_down(i);
		}
}
				
// ZZ: added function for hotplug up work
static void __cpuinit hotplug_online_work_fn(struct work_struct *work)
{
    int i=0;
				    
	for (i = 1; i < num_possible_cpus(); i++) {
		if (!cpu_online(i) && skip_hotplug_flag == 0 && hotplug_thresholds[0][i-1] != 0 && cur_load > hotplug_thresholds[0][i-1])
		cpu_up(i);
	    }
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate * dbs_info->rate_mult); // ZZ: Sampling down momentum - added multiplier

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
	INIT_DEFERRABLE_WORK(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	dbs_info->enable = 0;
	cancel_delayed_work(&dbs_info->work); // ZZ: Use asyncronous mode to avoid freezes / reboots when leaving zzmoove
}

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	struct cpufreq_frequency_table *table; // Yank : Use system frequency table
	unsigned int j;
	int rc;
	int i=0;
	
	this_dbs_info = &per_cpu(cs_cpu_dbs_info, cpu);

	table = cpufreq_frequency_get_table(0); // Yank : Get system frequency table

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
                                                kcpustat_cpu(j).cpustat[CPUTIME_NICE];
			}
			j_dbs_info->time_in_idle = get_cpu_idle_time_us(cpu, &j_dbs_info->idle_exit_time); // ZZ: added idle exit time handling
		}
		this_dbs_info->cpu = cpu; 		// ZZ: Initialise the cpu field during conservative governor start (https://github.com/ktoonsez/KT747-JB/commit/298dd04a610a6a655d7b77f320198d9f6c7565b6)
		this_dbs_info->rate_mult = 1; 		// ZZ: Sampling down momentum - reset multiplier
		this_dbs_info->momentum_adder = 0; 	// ZZ: Sampling down momentum - reset momentum adder
		this_dbs_info->down_skip = 0;		// ZZ: Sampling down - reset down_skip
		this_dbs_info->check_cpu_skip = 1;	// ZZ: we do not want to crash because of hotplugging so we start without it by skipping check_cpu
		this_dbs_info->requested_freq = policy->cur;
		max_scaling_freq_hard = 0;		// ZZ: set freq scaling start point to 0 (all frequencies up to table max)
		max_scaling_freq_soft = 0;		// ZZ: set freq scaling start point to 0 (all frequencies up to table max)
		
		// ZZ: save default values in threshold array
		for (i = 0; i < num_possible_cpus(); i++) {
		    hotplug_thresholds[0][i] = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG;
		    hotplug_thresholds[1][i] = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG;
		}
		
		// ZZ: initialisation of freq search in scaling table
		for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) {
			if (policy->max == table[i].frequency) {
				max_scaling_freq_hard = max_scaling_freq_soft = i; // ZZ: init soft and hard value
				// Yank : Continue looping until table end is reached, we need this to set the table size limit below
			}
		}

		freq_table_size = i - 1; // Yank: last valid frequency step (lowest frequency)

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
			orig_sampling_down_factor = dbs_tuners_ins.sampling_down_factor;	// ZZ: Sampling down momentum - set down factor
			orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;	// ZZ: Sampling down momentum - set max momentum
			cpufreq_register_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
		}
		mutex_unlock(&dbs_mutex);
		dbs_timer_init(this_dbs_info);
		skip_hotplug_flag = 0;			// ZZ: enable hotplugging again (this is important or hotplugging will stay off after gov restart!)
		break;

	case CPUFREQ_GOV_STOP:
		skip_hotplug_flag = 1; 			// ZZ: disable hotplugging during stop to avoid deadlocks if we are in the hotplugging logic
		this_dbs_info->check_cpu_skip = 1;	// ZZ: and we disable cpu_check also on next 25 samples
		
		mutex_lock(&dbs_mutex);			// ZZ: added for deadlock fix on governor stop
		dbs_timer_exit(this_dbs_info);
		mutex_unlock(&dbs_mutex);		// ZZ: added for deadlock fix on governor stop
		
		this_dbs_info->idle_exit_time = 0;	// ZZ: added idle exit time handling
		
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
		skip_hotplug_flag = 0; 						// ZZ: enable hotplugging again
		break;

	case CPUFREQ_GOV_LIMITS:
		skip_hotplug_flag = 1;			// ZZ: disable hotplugging during limit change
		this_dbs_info->check_cpu_skip = 1;	// ZZ: to avoid deadlocks skip check_cpu next 25 samples
		for (i = 0; i < 1000; i++);		// ZZ: wait a few samples to be sure hotplugging is off (never be sure so this is dirty)
		/*
		 * ZZ: we really want to do this limit update but here are deadlocks possible if hotplugging locks are active, so if we are about
		 * to crash skip the whole freq limit change attempt by using mutex_trylock instead of mutex_lock.
		 * so now this is a real fix but on the other hand it could also avoid limit changes so we keep all the other workarounds
		 * to reduce the chance of such situations!
		 */
		if (mutex_trylock(&this_dbs_info->timer_mutex)) {
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		} else {
		return 0;
		}
		/*
		* ZZ: obviously this "limit case" will be executed multible times at suspend (not sure why!?)
		* but we have already a early suspend code to handle scaling search limits so we have to use a flag to avoid double execution at suspend!
		*/
		
		    for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) { 		// ZZ: trim search in scaling table
			    if (policy->max == table[i].frequency) {
				max_scaling_freq_hard = i; 	// ZZ: set new freq scaling number
				break;
			    }
		    }
		
		if (max_scaling_freq_soft < max_scaling_freq_hard) { 		// ZZ: if we would go under soft limits reset them
			    max_scaling_freq_soft = max_scaling_freq_hard; 	// ZZ: if soft value is lower than hard (freq higher than hard max limit) then set it to hard max limit value
			    if (policy->max <= dbs_tuners_ins.freq_limit) 	// ZZ: check limit
			    dbs_tuners_ins.freq_limit = 0;			// ZZ: and delete active limit if it is under hard limit
		} else if (max_scaling_freq_soft > max_scaling_freq_hard && dbs_tuners_ins.freq_limit == 0) {
			    max_scaling_freq_soft = max_scaling_freq_hard; 	// ZZ: if no limit is set and new limit has a higher number than soft (freq lower than limit) then set back to hard max limit value
		} 								// ZZ: if nothing applies then leave search range as it is (in case of soft limit most likely)
		
		skip_hotplug_flag = 0; 						// ZZ: enable hotplugging again
		this_dbs_info->time_in_idle = get_cpu_idle_time_us(cpu, &this_dbs_info->idle_exit_time); // ZZ: added idle exit time handling
		printk(KERN_ALERT "ZZMOOVE DEBUG: Limit case Passed %s %d \n",__FUNCTION__,__LINE__);
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

static int __init cpufreq_gov_dbs_init(void) // ZZ: added idle exit time handling
{
    unsigned int i;
    struct cpu_dbs_info_s *this_dbs_info;
    /* Initalize per-cpu data: */
    for_each_possible_cpu(i) {
	this_dbs_info = &per_cpu(cs_cpu_dbs_info, i);
	this_dbs_info->time_in_idle = 0;
	this_dbs_info->idle_exit_time = 0;
    }

    INIT_WORK(&hotplug_offline_work, hotplug_offline_work_fn); // ZZ: init hotplug work
    INIT_WORK(&hotplug_online_work, hotplug_online_work_fn); // ZZ: init hotplug work
    
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
		"by ZaneZam November 2012/13");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
