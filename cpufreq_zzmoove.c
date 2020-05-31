/*
 *  drivers/cpufreq/cpufreq_zzmoove.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *            (C)  2012 Michael Weingaertner <mialwe@googlemail.com>
 *                      Zane Zaminsky <cyxman@yahoo.com>
 *                      Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *                      ffolkes <ffolkes@ffolkes.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * -------------------------------------------------------------------------------------------------------------------------------------------------------
 * -  Description:																	 -
 * -------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * 'ZZMoove' governor is based on the modified 'conservative' (original author Alexander Clouter <alex@digriz.org.uk>) 'smoove' governor from Michael
 * Weingaertner <mialwe@googlemail.com> (source: https://github.com/mialwe/mngb/blob/master/drivers/cpufreq/cpufreq_smoove.c) ported/modified/optimzed
 * for Samsung GT-I9300 since November 2012 and further improved in general for Exynos and Snapdragon platforms (but also working on other platforms
 * like OMAP) by ZaneZam, Yank555 and ffolkes. This version was ported to and improved for big.LITTLE architecture by ZaneZam from 2016 till 2020
 *
 * -------------------------------------------------------------------------------------------------------------------------------------------------------
 * -																			 -
 * -------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <linux/slab.h>
#include "cpufreq_governor.h"

// ZZ: for version information tunable
#define ZZMOOVE_VERSION				"bLE-develop-k49x-010520"

// ZZMoove governor macros
#define DEF_FREQUENCY_UP_THRESHOLD		(80)	// ZZ: load when up scaling should start
#define DEF_FREQUENCY_DOWN_THRESHOLD		(40)	// ZZ: load when down scaling should start
#define DEF_SAMPLING_DOWN_FACTOR		(1)	// ZZ: sampling down factor is a delay for down scaling
#define MAX_SAMPLING_DOWN_FACTOR		(10)	// ZZ: maximal amount of delay counts for down scaling
#define DEF_SAMPLING_UP_FACTOR			(1)	// ZZ: sampling up factor is a delay for up scaling
#define MAX_SAMPLING_UP_FACTOR			(10)	// ZZ: maximal amount of delay counts for up scaling
#define DEF_SMOOTH_UP				(75)	// ZZ: default cpu load trigger for 'boosting' scaling frequency
#define DEF_SCALING_PROPORTIONAL		(0)	// ZZ: default for proportional scaling, disabled here
#define DEF_FAST_SCALING_UP			(0)	// Yank: default fast scaling for upscaling
#define DEF_FAST_SCALING_DOWN			(0)	// Yank: default fast scaling for downscaling
#define DEF_AFS_UP				(0)	// ZZ: default auto fast scaling up
#define DEF_AFS_DOWN				(0)	// ZZ: default auto fast scaling down
#define DEF_AFS_THRESHOLD1			(25)	// ZZ: default auto fast scaling step one
#define DEF_AFS_THRESHOLD2			(50)	// ZZ: default auto fast scaling step two
#define DEF_AFS_THRESHOLD3			(75)	// ZZ: default auto fast scaling step three
#define DEF_AFS_THRESHOLD4			(90)	// ZZ: default auto fast scaling step four

struct zz_policy_dbs_info {
	struct cpu_dbs_info cdbs;
	struct policy_dbs_info policy_dbs;
	struct cpufreq_frequency_table *freq_table;
	unsigned int down_skip;				// ZZ: down skip value for sampling down factor
	unsigned int up_skip;				// ZZ: up skip value for sampling up factor
	unsigned int pol_max;				// ZZ: holds actual max policy
	unsigned int pol_min;				// ZZ: holds actual min policy
	unsigned int requested_freq;			// ZZ: holds last requested frequency
	bool freq_table_desc;				// ZZ: flag for table order ascending or descending (=true)
	bool scaling_init_eval_done;			// ZZ: flag for initial scaling range evaluation
	unsigned int freq_table_size;			// ZZ: size of freq table (index count)
	unsigned int zz_prev_load;			// ZZ: previous load saved for afs calculation
	unsigned int min_scaling_freq;			// ZZ: saved min freq for range detection
	unsigned int limit_table_start;			// ZZ: table start index for range detection
	unsigned int limit_table_end;			// ZZ: table end index for range detection
	unsigned int max_scaling_freq_hard;		// ZZ: hard limit table max index
	unsigned int min_scaling_freq_hard;		// ZZ: hard limit table min index
	unsigned int max_scaling_freq_soft;		// ZZ: soft limit table index
};

static inline struct zz_policy_dbs_info *to_dbs_info(struct policy_dbs_info *policy_dbs)
{
	return container_of(policy_dbs, struct zz_policy_dbs_info, policy_dbs);
}

// ZZ: tunable vars
struct zz_dbs_tuners {
	unsigned int ignore_nice_load;			// ZZ: od/cs shared common tunable
	unsigned int sampling_rate;			// ZZ: od/cs shared common tunable
	unsigned int sampling_up_factor;		// ZZ: zzmoove tunable
	unsigned int sampling_down_factor;		// ZZ: zzmoove tunable
	unsigned int up_threshold;			// ZZ: od/cs shared common tunable
	unsigned int down_threshold;			// ZZ: zzmoove tunable
	unsigned int smooth_up;				// ZZ: zzmoove tunable
	unsigned int scaling_proportional;		// ZZ: zzmoove tunable
	unsigned int fast_scaling_up;			// ZZ: zzmoove tunable
	unsigned int fast_scaling_down;			// ZZ: zzmoove tunable
	unsigned int afs_up;				// ZZ: zzmoove tunable
	unsigned int afs_down;				// ZZ: zzmoove tunable
	unsigned int afs_threshold1;			// ZZ: zzmoove tunable
	unsigned int afs_threshold2;			// ZZ: zzmoove tunable
	unsigned int afs_threshold3;			// ZZ: zzmoove tunable
	unsigned int afs_threshold4;			// ZZ: zzmoove tunable
};

// ZZ: function for frequency table order detection and limit optimization
static inline void evaluate_scaling_order_limit_range(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct zz_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	int i = 0;
	int calc_index = 0;

	// ZZ: init dbs variables
	dbs_info->freq_table_size = 0;
	dbs_info->freq_table_desc = false;
	dbs_info->max_scaling_freq_hard = 0;
	dbs_info->max_scaling_freq_soft = 0;
	dbs_info->min_scaling_freq_hard = 0;
	dbs_info->min_scaling_freq = 0;
	dbs_info->limit_table_start = 0;
	dbs_info->limit_table_end = CPUFREQ_TABLE_END;

	// ZZ: initialisation of freq search in scaling table
	for (i = 0; (likely(dbs_info->freq_table[i].frequency != CPUFREQ_TABLE_END)); i++) {
		if (unlikely(dbs_info->pol_max == dbs_info->freq_table[i].frequency))
			dbs_info->max_scaling_freq_hard = dbs_info->max_scaling_freq_soft = i;		// ZZ: init soft and hard max value
		if (unlikely(dbs_info->pol_min == dbs_info->freq_table[i].frequency))
			dbs_info->min_scaling_freq_hard = i;						// ZZ: init hard min value
	/*
	 * Yank: continue looping until table end is reached,
	 * we need this to set the table size limit below
	 */
	}

	dbs_info->freq_table_size = i - 1;								// Yank: upper index limit of freq. table

        /*
         * ZZ: we have to take care about where we are in the frequency table. when using kernel sources without OC capability
         * it might be that the first few indexes are containg no frequencies so a save index start point is needed.
         */
	calc_index = dbs_info->freq_table_size - dbs_info->max_scaling_freq_hard;			// ZZ: calculate the difference and use it as start point

	if (calc_index == dbs_info->freq_table_size)							// ZZ: if we are at the end of the table
		calc_index = calc_index - 1;								// ZZ: shift in range for order calculation below

        // Yank: assert if CPU freq. table is in ascending or descending order
	if (dbs_info->freq_table[calc_index].frequency > dbs_info->freq_table[calc_index+1].frequency) {
		dbs_info->freq_table_desc = true;							// Yank: table is in descending order as expected, lowest freq at the bottom of the table
		dbs_info->min_scaling_freq = i - 1;							// Yank: last valid frequency step (lowest frequency)
		dbs_info->limit_table_start = dbs_info->max_scaling_freq_soft;				// ZZ: we should use the actual max scaling soft limit value as search start point
        } else {
		dbs_info->freq_table_desc = false;							// Yank: table is in ascending order, lowest freq at the top of the table
		dbs_info->min_scaling_freq = 0;								// Yank: first valid frequency step (lowest frequency)
		dbs_info->limit_table_start = dbs_info->min_scaling_freq_hard;				// ZZ: we should use the actual min scaling hard limit value as search start point
		dbs_info->limit_table_end = dbs_info->pol_max;						// ZZ: end searching at highest frequency limit
        }
}

// Yank: return a valid value between min and max
static int validate_min_max(int val, int min, int max)
{
	return min(max(val, min), max);
}

// ZZ: system table scaling mode with freq search optimizations and proportional frequency target option
static inline int zz_get_next_freq(unsigned int curfreq, unsigned int updown, unsigned int load, struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct zz_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	struct dbs_data *dbs_data = policy->governor_data;
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	int i = 0;
	unsigned int prop_target = 0;									// ZZ: proportional freq
	unsigned int zz_target = 0;									// ZZ: system table freq
	unsigned int dead_band_freq = 0;								// ZZ: dead band freq
	int smooth_up_steps = 0;									// Yank: smooth up steps
	static int tmp_limit_table_start = 0;
	static int tmp_max_scaling_freq_soft = 0;
	static int tmp_limit_table_end = 0;

	prop_target = dbs_info->pol_min + load * (dbs_info->pol_max - dbs_info->pol_min) / 100;		// ZZ: prepare proportional target freq whitout deadband (directly mapped to min->max load)

	if (zz_tuners->scaling_proportional == 2)							// ZZ: mode '2' use proportional target frequencies only
	    return prop_target;

	if (zz_tuners->scaling_proportional == 3) {							// ZZ: mode '3' use proportional target frequencies only and switch to pol_min in deadband range
	    dead_band_freq = dbs_info->pol_max / 100 * load;						// ZZ: use old calculation to get deadband frequencies (=lower than pol_min)
	    if (dead_band_freq > dbs_info->pol_min)							// ZZ: the load usually is too unsteady so we rarely would reach pol_min when load is low
		return prop_target;									// ZZ: in fact it only will happen when load=0, so only return proportional frequencies if they
	    else											//     are out of deadband range and if we are in deadband range return min freq
		return dbs_info->pol_min;								//     (thats a similar behaving as with old propotional freq calculation)
	}

	if (load <= zz_tuners->smooth_up)								// Yank: consider smooth up
	    smooth_up_steps = 0;									// Yank: load not reached, move by one step
	else
	    smooth_up_steps = 1;									// Yank: load reached, move by two steps

	// ZZ: first assign new limits...
	tmp_limit_table_start = dbs_info->limit_table_start;
	tmp_limit_table_end = dbs_info->limit_table_end;
	tmp_max_scaling_freq_soft = dbs_info->max_scaling_freq_soft;

	// ZZ: asc: min freq limit changed
	if (!dbs_info->freq_table_desc && curfreq
	    < dbs_info->freq_table[dbs_info->min_scaling_freq].frequency)				// ZZ: asc: but reset starting index if current freq is lower than soft/hard min limit otherwise we are
	    tmp_limit_table_start = 0;									//     shifting out of range and proportional freq is used instead because freq can't be found by loop

	// ZZ: asc: max freq limit changed
	if (!dbs_info->freq_table_desc && curfreq
	    > dbs_info->freq_table[dbs_info->max_scaling_freq_soft].frequency)				// ZZ: asc: but reset ending index if current freq is higher than soft/hard max limit otherwise we are
	    tmp_limit_table_end = dbs_info->freq_table[dbs_info->freq_table_size].frequency;		//     shifting out of range and proportional freq is used instead because freq can't be found by loop

	// ZZ: desc: max freq limit changed
	if (dbs_info->freq_table_desc && curfreq
	    > dbs_info->freq_table[dbs_info->limit_table_start].frequency)				// ZZ: desc: but reset starting index if current freq is higher than soft/hard max limit otherwise we are
	    tmp_limit_table_start = 0;									//     shifting out of range and proportional freq is used instead because freq can't be found by loop

	// ZZ: feq search loop with optimization
	if (dbs_info->freq_table_desc) {
	    for (i = tmp_limit_table_start; (likely(dbs_info->freq_table[i].frequency != tmp_limit_table_end)); i++) {
		if (unlikely(curfreq == dbs_info->freq_table[i].frequency)) {				// Yank: we found where we currently are (i)
		    if (updown == 1) {									// Yank: scale up, but don't go above softlimit
			zz_target = min(dbs_info->freq_table[tmp_max_scaling_freq_soft].frequency,
		        dbs_info->freq_table[validate_min_max(i - 1 - smooth_up_steps - zz_tuners->fast_scaling_up, 0, dbs_info->freq_table_size)].frequency);
			if (zz_tuners->scaling_proportional == 1)					// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);						// ZZ: check which freq is lower and return it
			else
			    return zz_target;								// ZZ: or return the found system table freq as usual
		    } else {										// Yank: scale down, but don't go below min. freq.
			zz_target = max(dbs_info->freq_table[dbs_info->min_scaling_freq].frequency,
		        dbs_info->freq_table[validate_min_max(i + 1 + zz_tuners->fast_scaling_down, 0, dbs_info->freq_table_size)].frequency);
			if (zz_tuners->scaling_proportional == 1)					// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);						// ZZ: check which freq is lower and return it
			else
			    return zz_target;								// ZZ: or return the found system table freq as usual
		    }
		}
	    }												// ZZ: this shouldn't happen but if the freq is not found in system table
	    return prop_target;										//     fall back to proportional freq target to avoid returning 0
	} else {
	    for (i = tmp_limit_table_start; (likely(dbs_info->freq_table[i].frequency <= tmp_limit_table_end)); i++) {
		if (unlikely(curfreq == dbs_info->freq_table[i].frequency)) {				// Yank: we found where we currently are (i)
		    if (updown == 1) {									// Yank: scale up, but don't go above softlimit
			zz_target = min(dbs_info->freq_table[tmp_max_scaling_freq_soft].frequency,
			dbs_info->freq_table[validate_min_max(i + 1 + smooth_up_steps + zz_tuners->fast_scaling_up, 0, dbs_info->freq_table_size)].frequency);
			if (zz_tuners->scaling_proportional == 1)					// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);						// ZZ: check which freq is lower and return it
			else
			    return zz_target;								// ZZ: or return the found system table freq as usual
		    } else {										// Yank: scale down, but don't go below min. freq.
			zz_target = max(dbs_info->freq_table[dbs_info->min_scaling_freq].frequency,
			dbs_info->freq_table[validate_min_max(i - 1 - zz_tuners->fast_scaling_down, 0, dbs_info->freq_table_size)].frequency);
			if (zz_tuners->scaling_proportional == 1)					// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);						// ZZ: check which freq is lower and return it
			else
			    return zz_target;								// ZZ: or return the found system table freq as usual
		    }
		}
	    }												// ZZ: this shouldn't happen but if the freq is not found in system table
	    return prop_target;										//     fall back to proportional freq target to avoid returning 0
	}
}

/*
 * Every sampling_rate * sampling_up_factor we check, if current idle time is less than 20% (default)
 * then we try to increase frequency. Every sampling_rate * sampling_down_factor we check if current
 * idle time is more than 60% (default), then we try to decrease frequency
 */
static unsigned int zz_dbs_timer(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct zz_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	struct dbs_data *dbs_data = policy_dbs->dbs_data;
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int load = dbs_update(policy);

	// ZZ: save pol limits in gov data and evaluate scaling range if not done already at init or limits have changed
	if (dbs_info->pol_min != policy->min || dbs_info->pol_max != policy->max || !dbs_info->scaling_init_eval_done) {
	    dbs_info->pol_min = policy->min;
	    dbs_info->pol_max = policy->max;
	    evaluate_scaling_order_limit_range(policy);
	}

	/*
	 * ZZ/Yank: Auto fast scaling mode
	 * Switch to all 4 fast scaling modes depending on load gradient
	 * the mode will start switching at given afs threshold load changes in both directions
	 */
	if (zz_tuners->afs_up       > 0) {
	    if (load > dbs_info->zz_prev_load && load - dbs_info->zz_prev_load <= zz_tuners->afs_threshold1) {
		zz_tuners->fast_scaling_up = 0;
	    } else if (load - dbs_info->zz_prev_load <= zz_tuners->afs_threshold2) {
		zz_tuners->fast_scaling_up = 1;
	    } else if (load - dbs_info->zz_prev_load <= zz_tuners->afs_threshold3) {
		zz_tuners->fast_scaling_up = 2;
	    } else if (load - dbs_info->zz_prev_load <= zz_tuners->afs_threshold4) {
		zz_tuners->fast_scaling_up = 3;
	    } else {
		zz_tuners->fast_scaling_up = 4;
	    }
	}

	if (zz_tuners->afs_down       > 0) {
	  if (load < dbs_info->zz_prev_load && dbs_info->zz_prev_load - load <= zz_tuners->afs_threshold1) {
		zz_tuners->fast_scaling_down = 0;
	    } else if (dbs_info->zz_prev_load - load <= zz_tuners->afs_threshold2) {
		zz_tuners->fast_scaling_down = 1;
	    } else if (dbs_info->zz_prev_load - load <= zz_tuners->afs_threshold3) {
		zz_tuners->fast_scaling_down = 2;
	    } else if (dbs_info->zz_prev_load - load <= zz_tuners->afs_threshold4) {
		zz_tuners->fast_scaling_down = 3;
	    } else {
		zz_tuners->fast_scaling_down = 4;
	    }
	}

	/* if sampling_up_factor is active break out early */
	if (++dbs_info->up_skip < zz_tuners->sampling_up_factor)
		goto out;

	dbs_info->up_skip = 0;

	/* Check for frequency increase */
	if (load > dbs_data->up_threshold) {
		dbs_info->down_skip = 0;

		/* if we are already at full speed then break out early */
		if (dbs_info->requested_freq == policy->max)
			goto out;

		dbs_info->requested_freq = zz_get_next_freq(policy->cur, 1, load, policy);

		// ZZ: this is for proportional scaling mode only as zzmoove scaling delivers only frequencies which are 'in range'
		if (dbs_info->requested_freq > policy->max)
			dbs_info->requested_freq = policy->max;

		__cpufreq_driver_target(policy, dbs_info->requested_freq, CPUFREQ_RELATION_H);
		goto out;
	}

	/* if sampling_down_factor is active break out early */
	if (++dbs_info->down_skip < zz_tuners->sampling_down_factor)
		goto out;

	dbs_info->down_skip = 0;

	/* Check for frequency decrease */
	if (load < zz_tuners->down_threshold) {
		dbs_info->up_skip = 0;

		 /* if we cannot reduce the frequency anymore, break out early */
		if (policy->cur == policy->min)
			goto out;

		dbs_info->requested_freq = zz_get_next_freq(policy->cur, 0, load, policy);

		__cpufreq_driver_target(policy,  dbs_info->requested_freq, CPUFREQ_RELATION_L);
	}

    out:
	dbs_info->zz_prev_load = load;
	return dbs_data->sampling_rate;
}

/************************** sysfs interface ************************/

static ssize_t store_sampling_down_factor(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;

	dbs_data->sampling_down_factor = input;
	return count;
}

static ssize_t store_sampling_up_factor(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_UP_FACTOR || input < 1)
		return -EINVAL;

	zz_tuners->sampling_up_factor = input;
	return count;
}

static ssize_t store_up_threshold(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input <= zz_tuners->down_threshold)
		return -EINVAL;

	dbs_data->up_threshold = input;
	return count;
}

static ssize_t store_down_threshold(struct gov_attr_set *attr_set, const char *buf,
		size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 1 otherwise freq will not fall */
	if (ret != 1 || input < 1 || input > 100 ||
			input >= dbs_data->up_threshold)
		return -EINVAL;

	zz_tuners->down_threshold = input;
	return count;
}

static ssize_t store_ignore_nice_load(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_data->ignore_nice_load) /* nothing to do */
		return count;

	dbs_data->ignore_nice_load = input;

	/* we need to re-evaluate prev_cpu_idle */
	gov_update_cpu_data(dbs_data);

	return count;
}

// ZZ: tunable -> possible values: range from 1 to 100, if not set default is 75
static ssize_t store_smooth_up(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1 || input > 100 || input < 1)
	    return -EINVAL;

	zz_tuners->smooth_up = input;

	return count;
}

/*
 * ZZ: tunable scaling proportinal -> possible values: 0 to disable,
 * 1 to enable comparision between proportional and optimized freq,
 * 2 to enable propotional freq usage only
 * 3 to enable propotional freq usage only but with dead brand range
 * to avoid not reaching of pol min freq,
 * if not set default is 0
 */
static ssize_t store_scaling_proportional(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || input > 3)
	    return -EINVAL;

	zz_tuners->scaling_proportional = input;

	return count;
}

/*
 * Yank: tunable -> possible values 1-4 to enable fast upscaling (value 1-4 = steps)
 */
static ssize_t store_fast_scaling_up(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 4 || input < 0)
	    return -EINVAL;

	zz_tuners->fast_scaling_up = input;
	zz_tuners->afs_up = 0;

	return count;
}

/*
 * Yank: tunable -> possible values 1-4 to enable fast downscaling (value 1-4 = steps)
 */
static ssize_t store_fast_scaling_down(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 4 || input < 0)
	    return -EINVAL;

	zz_tuners->fast_scaling_down = input;
	zz_tuners->afs_down = 0;

	return count;
}

/*
 * ZZ: tunable -> possible values 1 to enable auto fast scaling (insane scaling)
 * for upscaling and 0 to disable auto fast scaling for upscaling
 */
static ssize_t store_afs_up(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 1 || input < 0)
	    return -EINVAL;

	zz_tuners->afs_up = input;

	if (input == 0)
	    zz_tuners->fast_scaling_up = 0;

	return count;
}

/*
 * ZZ: tunable -> possible values 1 to enable auto fast scaling (insane scaling)
 * for downscaling and 0 to disable auto fast scaling for downscaling
 */
static ssize_t store_afs_down(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 1 || input < 0)
	    return -EINVAL;

	zz_tuners->afs_down = input;

	if (input == 0)
	    zz_tuners->fast_scaling_down = 0;

	return count;
}

// ZZ: afs tunable -> possible values from 0 to 100
#define store_afs_threshold(name)					\
static ssize_t store_afs_threshold##name(struct gov_attr_set *attr_set,	\
		const char *buf, size_t count)				\
{									\
	struct dbs_data *dbs_data = to_dbs_data(attr_set);		\
	struct zz_dbs_tuners *zz_tuners = dbs_data->tuners;		\
	unsigned int input;						\
	int ret;							\
									\
	ret = sscanf(buf, "%u", &input);				\
									\
	if (ret != 1 || input > 100 || input < 0)			\
	    return -EINVAL;						\
									\
	zz_tuners->afs_threshold##name = input;				\
									\
	return count;							\
}									\

// ZZ: show zzmoove version info in sysfs
static ssize_t show_version(struct gov_attr_set *attr_set, char *buf)
{
	return sprintf(buf, "%s\n", ZZMOOVE_VERSION);
}

store_afs_threshold(1);
store_afs_threshold(2);
store_afs_threshold(3);
store_afs_threshold(4);

gov_show_one_common(sampling_rate);
gov_show_one_common(sampling_down_factor);
gov_show_one_common(up_threshold);
gov_show_one_common(ignore_nice_load);
gov_show_one_common(min_sampling_rate);

gov_show_one(zz, sampling_up_factor);
gov_show_one(zz, down_threshold);
gov_show_one(zz, smooth_up);
gov_show_one(zz, scaling_proportional);
gov_show_one(zz, fast_scaling_up);
gov_show_one(zz, fast_scaling_down);
gov_show_one(zz, afs_up);
gov_show_one(zz, afs_down);
gov_show_one(zz, afs_threshold1);
gov_show_one(zz, afs_threshold2);
gov_show_one(zz, afs_threshold3);
gov_show_one(zz, afs_threshold4);

gov_attr_rw(sampling_rate);
gov_attr_rw(sampling_down_factor);
gov_attr_rw(sampling_up_factor);
gov_attr_rw(up_threshold);
gov_attr_rw(down_threshold);
gov_attr_rw(ignore_nice_load);
gov_attr_rw(smooth_up);
gov_attr_rw(scaling_proportional);
gov_attr_rw(fast_scaling_up);
gov_attr_rw(fast_scaling_down);
gov_attr_rw(afs_up);
gov_attr_rw(afs_down);
gov_attr_rw(afs_threshold1);
gov_attr_rw(afs_threshold2);
gov_attr_rw(afs_threshold3);
gov_attr_rw(afs_threshold4);
gov_attr_ro(version);
gov_attr_ro(min_sampling_rate);

static struct attribute *zz_attributes[] = {
	&min_sampling_rate.attr,
	&sampling_rate.attr,
	&sampling_down_factor.attr,
	&sampling_up_factor.attr,
	&up_threshold.attr,
	&down_threshold.attr,
	&ignore_nice_load.attr,
	&smooth_up.attr,
	&scaling_proportional.attr,
	&fast_scaling_up.attr,
	&fast_scaling_down.attr,
	&afs_up.attr,
	&afs_down.attr,
	&afs_threshold1.attr,
	&afs_threshold2.attr,
	&afs_threshold3.attr,
	&afs_threshold4.attr,
	&version.attr,
	NULL
};

/************************** sysfs end ************************/

static struct policy_dbs_info *zz_alloc(void)
{
	struct zz_policy_dbs_info *dbs_info;

	dbs_info = kzalloc(sizeof(*dbs_info), GFP_KERNEL);
	return dbs_info ? &dbs_info->policy_dbs : NULL;
}

static void zz_free(struct policy_dbs_info *policy_dbs)
{
	kfree(to_dbs_info(policy_dbs));
}

static int zz_init(struct dbs_data *dbs_data)
{
	struct zz_dbs_tuners *tuners;

	tuners = kzalloc(sizeof(*tuners), GFP_KERNEL);
	if (!tuners)
	    return -ENOMEM;

	tuners->sampling_up_factor = DEF_SAMPLING_UP_FACTOR;
	tuners->down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD;
	tuners->smooth_up = DEF_SMOOTH_UP;
	tuners->scaling_proportional = DEF_SCALING_PROPORTIONAL;
	tuners->fast_scaling_up = DEF_FAST_SCALING_UP;
	tuners->fast_scaling_down = DEF_FAST_SCALING_DOWN;
	tuners->afs_up = DEF_AFS_UP;
	tuners->afs_down = DEF_AFS_DOWN;
	tuners->afs_threshold1 = DEF_AFS_THRESHOLD1;
	tuners->afs_threshold2 = DEF_AFS_THRESHOLD2;
	tuners->afs_threshold3 = DEF_AFS_THRESHOLD3;
	tuners->afs_threshold4 = DEF_AFS_THRESHOLD4;

	dbs_data->up_threshold = DEF_FREQUENCY_UP_THRESHOLD;
	dbs_data->sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;
	dbs_data->ignore_nice_load = 0;
	dbs_data->tuners = tuners;
	dbs_data->min_sampling_rate = MIN_SAMPLING_RATE_RATIO *
		jiffies_to_usecs(10);

	return 0;
}

static void zz_exit(struct dbs_data *dbs_data)
{
	kfree(dbs_data->tuners);
}

static void zz_start(struct cpufreq_policy *policy)
{
	struct zz_policy_dbs_info *dbs_info = to_dbs_info(policy->governor_data);

	dbs_info->down_skip = 0;
	dbs_info->up_skip = 0;
	dbs_info->pol_max = policy->max;
	dbs_info->pol_min = policy->min;
	dbs_info->requested_freq = policy->cur;
	dbs_info->freq_table = policy->freq_table;
}

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
static
#endif
struct dbs_governor zz_governor = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("zzmoove"),
	.kobj_type = { .default_attrs = zz_attributes },
	.gov_dbs_timer = zz_dbs_timer,
	.alloc = zz_alloc,
	.free = zz_free,
	.init = zz_init,
	.exit = zz_exit,
	.start = zz_start,
};

#define CPU_FREQ_GOV_ZZMOOVE	(&zz_governor.gov)

static int __init cpufreq_gov_dbs_init(void)
{
    return cpufreq_register_governor(CPU_FREQ_GOV_ZZMOOVE);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
    cpufreq_unregister_governor(CPU_FREQ_GOV_ZZMOOVE);
}

MODULE_AUTHOR("Zane Zaminsky <cyxman@yahoo.com>");
MODULE_DESCRIPTION("'cpufreq_zzmoove' - A dynamic cpufreq governor based "
	"on smoove governor from Michael Weingaertner which was originally based on "
	"conservative governor from Alexander Clouter. Optimized for use with Samsung GT-I9300 "
	"using a fast scaling logic - ported/modified/optimized for GT-I9300 since November 2012 "
	"and further improved in general for Exynos, Snapdragon platforms by ZaneZam, Yank555 and ffolkes "
	"This version was ported to and improved for big.LITTLE architecture by ZaneZam from 2016 till 2020");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return CPU_FREQ_GOV_ZZMOOVE;
}
E
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
