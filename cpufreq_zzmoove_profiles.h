/*
 *  drivers/cpufreq/cpufreq_zzmoove_profiles.h - Profiles
 *
 *  Copyright (C)  2013 Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

static char profiles_file_version[20] = "0.1-beta10";
#define PROFILE_TABLE_END ~1
#define END_OF_PROFILES "end"

struct zzmoove_profile {
	unsigned int profile_number;
	char         profile_name[20];
	unsigned int disable_hotplug;
	unsigned int disable_hotplug_sleep;
	unsigned int down_threshold;
	unsigned int down_threshold_hotplug1;
	unsigned int down_threshold_hotplug2;
	unsigned int down_threshold_hotplug3;
	unsigned int down_threshold_hotplug4;
	unsigned int down_threshold_hotplug5;
	unsigned int down_threshold_hotplug6;
	unsigned int down_threshold_hotplug7;
	unsigned int down_threshold_hotplug_freq1;
	unsigned int down_threshold_hotplug_freq2;
	unsigned int down_threshold_hotplug_freq3;
	unsigned int down_threshold_hotplug_freq4;
	unsigned int down_threshold_hotplug_freq5;
	unsigned int down_threshold_hotplug_freq6;
	unsigned int down_threshold_hotplug_freq7;
	unsigned int down_threshold_sleep;
	unsigned int early_demand;
	unsigned int early_demand_sleep;
	unsigned int fast_scaling;
	unsigned int fast_scaling_sleep;
	unsigned int freq_limit;
	unsigned int freq_limit_sleep;
	unsigned int freq_step;
	unsigned int freq_step_sleep;
	unsigned int grad_up_threshold;
	unsigned int grad_up_threshold_sleep;
	unsigned int hotplug_block_cycles;
	unsigned int hotplug_idle_threshold;
	unsigned int hotplug_sleep;
	unsigned int ignore_nice_load;
	int lcdfreq_enable;
	unsigned int lcdfreq_kick_in_cores;
	unsigned int lcdfreq_kick_in_down_delay;
	unsigned int lcdfreq_kick_in_freq;
	unsigned int lcdfreq_kick_in_up_delay;
	unsigned int sampling_down_factor;
	unsigned int sampling_down_max_momentum;
	unsigned int sampling_down_momentum_sensitivity;
	unsigned int sampling_rate;
	unsigned int sampling_rate_idle;
	unsigned int sampling_rate_idle_delay;
	unsigned int sampling_rate_idle_threshold;
	unsigned int sampling_rate_sleep_multiplier;
	unsigned int scaling_block_cycles;
	unsigned int scaling_block_freq;
	unsigned int scaling_block_threshold;
	unsigned int smooth_up;
	unsigned int smooth_up_sleep;
	unsigned int up_threshold;
	unsigned int up_threshold_hotplug1;
	unsigned int up_threshold_hotplug2;
	unsigned int up_threshold_hotplug3;
	unsigned int up_threshold_hotplug4;
	unsigned int up_threshold_hotplug5;
	unsigned int up_threshold_hotplug6;
	unsigned int up_threshold_hotplug7;
	unsigned int up_threshold_hotplug_freq1;
	unsigned int up_threshold_hotplug_freq2;
	unsigned int up_threshold_hotplug_freq3;
	unsigned int up_threshold_hotplug_freq4;
	unsigned int up_threshold_hotplug_freq5;
	unsigned int up_threshold_hotplug_freq6;
	unsigned int up_threshold_hotplug_freq7;
	unsigned int up_threshold_sleep;
	unsigned int legacy_mode;
};

struct zzmoove_profile zzmoove_profiles[] = {
	{
		1,		// first profile
		"def",		// Default settings as hardcoded in the governor (please don't remove this profile)
		0,		// disable_hotplug (1=disable hotplugging, 0=enable hotplugging)
		0,		// disable_hotplug_sleep (1=disable hotplugging, 0=enable hotplugging)
		52,		// down_threshold (range from 11 to 100 and must be lower than up_threshold - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug1 (range from 11 to 100 and must be lower than up_threshold_hotplug1 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug2 (range from 11 to 100 and must be lower than up_threshold_hotplug2 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug3 (range from 11 to 100 and must be lower than up_threshold_hotplug3 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug4 (range from 11 to 100 and must be lower than up_threshold_hotplug4 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug5 (range from 11 to 100 and must be lower than up_threshold_hotplug5 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug6 (range from 11 to 100 and must be lower than up_threshold_hotplug6 - wrong values will not be applied in the governor!)
		55,		// down_threshold_hotplug7 (range from 11 to 100 and must be lower than up_threshold_hotplug7 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq1 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq1 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq2 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq2 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq3 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq3 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq4 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq4 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq5 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq5 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq6 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq6 - wrong values will not be applied in the governor!)
		0,		// down_threshold_hotplug_freq7 (range from 0 to scaling max and must be lower than up_threshold_hotplug_freq7 - wrong values will not be applied in the governor!)
		44,		// down_threshold_sleep (range from 11 to 100)
		0,		// early_demand (any value=enable, 0=disable)
		1,		// early_demand_sleep (any value=enable, 0=disable)
		0,		// fast_scaling (range from 1 to 8)
		0,		// fast_scaling_sleep (range from 1 to 8)
		0,		// freq_limit (0=disable, range in system table from freq->min to freq->max in khz)
		0,		// freq_limit_sleep (0=disable, range in system table from freq->min to freq->max in khz)
		5,		// freq_step (range from 1 to 100)
		5,		// freq_step_sleep (range from 1 to 100)
		25,		// grad_up_threshold (range from 11 to 100)
		35,		// grad_up_threshold_sleep (range from 11 to 100)
		5,		// hotplug_block_cycles (0=disable, any value above 0)
		0,		// hotplug_idle_threshold (range from 1 to 100)
		0,		// hotplug_sleep (0=all cores enabled, range 1 to MAX_CORES - 1)
		0,		// ignore_nice_load (0=disable, 1=enable)
		0,		// lcdfreq_enable (0=disable, 1=enable)
		0,		// lcdfreq_kick_in_cores (range from 0 to 4)
		20,		// lcdfreq_kick_in_down_delay (any value above 0)
		500000,		// lcdfreq_kick_in_freq (all valid system frequencies)
		50,		// lcdfreq_kick_in_up_delay (any value above 0)
		1,		// sampling_down_factor (1=disable, range from 2 to MAX_SAMPLING_DOWN_FACTOR)
		0,		// sampling_down_max_momentum (0=disable, range from 1 to MAX_SAMPLING_DOWN_FACTOR)
		50,		// sampling_down_momentum_sensitivity (range from 1 to MAX_SAMPLING_DOWn_SENSITIVITY)
		100000,		// sampling_rate (range from MIN_SAMPLING_RATE to any value)
		180000,		// sampling_rate_idle (range from MIN_SAMPLING_RATE to any value)
		0,		// sampling_rate_idle_delay (0=disable, any value above 0)
		40,		// sampling_rate_idle_threshold (range from 1 to 100)
		2,		// sampling_rate_sleep_multiplier (range from 1 to 4)
		100,		// scaling_block_cycles (0=disable, any value above 0)
		1200000,	// scaling_block_freq (all valid system frequencies)
		10,		// scaling_block_threshold (0=disable, range from 1 to 100)
		75,		// smooth_up (range from 1 to 100)
		100,		// smooth_up_sleep (range from 1 to 100)
		70,		// up_threshold (range 1 to 100 and must be higher than down_threshold - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug1 (range 1 to 100 and must be higher than down_threshold_hotplug1 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug2 (range 1 to 100 and must be higher than down_threshold_hotplug2 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug3 (range 1 to 100 and must be higher than down_threshold_hotplug3 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug4 (range 1 to 100 and must be higher than down_threshold_hotplug4 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug5 (range 1 to 100 and must be higher than down_threshold_hotplug5 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug6 (range 1 to 100 and must be higher than down_threshold_hotplug6 - wrong values will not be applied in the governor!)
		68,		// up_threshold_hotplug7 (range 1 to 100 and must be higher than down_threshold_hotplug7 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq1 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq1 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq2 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq2 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq3 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq3 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq4 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq4 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq5 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq5 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq6 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq6 - wrong values will not be applied in the governor!)
		0,		// up_threshold_hotplug_freq7 (0 to disable core, range from 1 to scaling max and must be higher than down_threshold_hotplug_freq7 - wrong values will not be applied in the governor!)
		90,		// up_threshold_sleep; (range from above down_threshold_sleep to 100)
		0,		// legacy_mode (if enabled by LEGACY_MODE macro 0=disabled, 1=enabled)
	},
	{
		2,
		"ybat",		// Yank555.lu battery profile (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		40,		// down_threshold
		65,		// down_threshold_hotplug1
		75,		// down_threshold_hotplug2
		85,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		600000,		// down_threshold_hotplug_freq1
		800000,		// down_threshold_hotplug_freq2
		1000000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		75,		// down_threshold_sleep
		1,		// early_demand
		1,		// early_demand_sleep
		4,		// fast_scaling
		1,		// fast_scaling_sleep
		0,		// freq_limit
		600000,		// freq_limit_sleep
		10,		// freq_step
		1,		// freq_step_sleep
		50,		// grad_up_threshold
		35,		// grad_up_threshold_sleep
		5,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		20,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		50,		// lcdfreq_kick_in_up_delay
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		75000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		65,		// smooth_up
		90,		// smooth_up_sleep
		60,		// up_threshold
		85,		// up_threshold_hotplug1
		90,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		700000,		// up_threshold_hotplug_freq1
		1000000,	// up_threshold_hotplug_freq2
		1200000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		85,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		3,
		"ybatext",	// Yank555.lu extreme battery profile (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		50,		// down_threshold
		70,		// down_threshold_hotplug1
		80,		// down_threshold_hotplug2
		90,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		800000,		// down_threshold_hotplug_freq1
		900000,		// down_threshold_hotplug_freq2
		1100000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		75,		// down_threshold_sleep
		1,		// early_demand
		1,		// early_demand_sleep
		4,		// fast_scaling
		0,		// fast_scaling_sleep
		0,		// freq_limit
		600000,		// freq_limit_sleep
		10,		// freq_step
		1,		// freq_step_sleep
		50,		// grad_up_threshold
		35,		// grad_up_threshold_sleep
		5,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		20,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		50,		// lcdfreq_kick_in_up_delay
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		60000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		75,		// smooth_up
		90,		// smooth_up_sleep
		70,		// up_threshold
		90,		// up_threshold_hotplug1
		95,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		900000,		// up_threshold_hotplug_freq1
		1100000,	// up_threshold_hotplug_freq2
		1300000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		85,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		4,
		"zzbat",	// ZaneZam battery profile (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		40,		// down_threshold
		45,		// down_threshold_hotplug1
		55,		// down_threshold_hotplug2
		65,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		600000,		// down_threshold_hotplug_freq1
		800000,		// down_threshold_hotplug_freq2
		1000000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		60,		// down_threshold_sleep
		0,		// early_demand
		1,		// early_demand_sleep
		0,		// fast_scaling
		0,		// fast_scaling_sleep
		0,		// freq_limit
		500000,		// freq_limit_sleep
		10,		// freq_step
		1,		// freq_step_sleep
		50,		// grad_up_threshold
		35,		// grad_up_threshold_sleep
		5,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		5,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		1,		// lcdfreq_kick_in_up_delay
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		100000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		75,		// smooth_up
		100,		// smooth_up_sleep
		95,		// up_threshold
		60,		// up_threshold_hotplug1
		80,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		700000,		// up_threshold_hotplug_freq1
		1000000,	// up_threshold_hotplug_freq2
		1200000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		100,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		5,
		"zzopt",	// ZaneZam optimized profile (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		52,		// down_threshold
		45,		// down_threshold_hotplug1
		55,		// down_threshold_hotplug2
		65,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		400000,		// down_threshold_hotplug_freq1
		600000,		// down_threshold_hotplug_freq2
		800000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		60,		// down_threshold_sleep
		1,		// early_demand
		1,		// early_demand_sleep
		1,		// fast_scaling
		2,		// fast_scaling_sleep
		0,		// freq_limit
		500000,		// freq_limit_sleep
		5,		// freq_step
		1,		// freq_step_sleep
		35,		// grad_up_threshold
		35,		// grad_up_threshold_sleep
		5,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		5,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		1,		// lcdfreq_kick_in_up_delay
		4,		// sampling_down_factor
		20,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		45000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		75,		// smooth_up
		100,		// smooth_up_sleep
		67,		// up_threshold
		68,		// up_threshold_hotplug1
		78,		// up_threshold_hotplug2
		88,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		500000,		// up_threshold_hotplug_freq1
		700000,		// up_threshold_hotplug_freq2
		900000,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		100,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		6,
		"zzperf",	// ZaneZam performance profile (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		20,		// down_threshold
		25,		// down_threshold_hotplug1
		35,		// down_threshold_hotplug2
		45,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		300000,		// down_threshold_hotplug_freq1
		700000,		// down_threshold_hotplug_freq2
		900000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		60,		// down_threshold_sleep
		1,		// early_demand
		1,		// early_demand_sleep
		5,		// fast_scaling
		2,		// fast_scaling_sleep
		0,		// freq_limit
		500000,		// freq_limit_sleep
		25,		// freq_step
		1,		// freq_step_sleep
		25,		// grad_up_threshold
		35,		// grad_up_threshold_sleep
		5,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		5,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		1,		// lcdfreq_kick_in_up_delay
		4,		// sampling_down_factor
		50,		// sampling_down_max_momentum
		25,		// sampling_down_momentum_sensitivity
		40000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		70,		// smooth_up
		100,		// smooth_up_sleep
		60,		// up_threshold
		65,		// up_threshold_hotplug1
		75,		// up_threshold_hotplug2
		85,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		400000,		// up_threshold_hotplug_freq1
		800000,		// up_threshold_hotplug_freq2
		1000000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		100,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		7,
		"zzbatplus",	// ZaneZam TEST bat profile based on yank bat (please don't remove this profile)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		58,		// down_threshold
		65,		// down_threshold_hotplug1
		75,		// down_threshold_hotplug2
		85,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
		600000,		// down_threshold_hotplug_freq1
		800000,		// down_threshold_hotplug_freq2
		1000000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		75,		// down_threshold_sleep
		0,		// early_demand
		1,		// early_demand_sleep
		0,		// fast_scaling
		0,		// fast_scaling_sleep
		0,		// freq_limit
		500000,		// freq_limit_sleep
		10,		// freq_step
		1,		// freq_step_sleep
		50,		// grad_up_threshold
		40,		// grad_up_threshold_sleep
		0,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		1,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		20,		// lcdfreq_kick_in_down_delay
		500000,		// lcdfreq_kick_in_freq
		50,		// lcdfreq_kick_in_up_delay
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		75000,		// sampling_rate
		200000,		// sampling_rate_idle
		5,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
		4,		// sampling_rate_sleep_multiplier
		100,		// scaling_block_cycles
		1200000,	// scaling_block_freq
		10,		// scaling_block_threshold
		75,		// smooth_up
		100,		// smooth_up_sleep
		60,		// up_threshold
		85,		// up_threshold_hotplug1
		90,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
		700000,		// up_threshold_hotplug_freq1
		1000000,	// up_threshold_hotplug_freq2
		1200000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		85,		// up_threshold_sleep
		0,		// legacy_mode
	},
	{
		PROFILE_TABLE_END,
		END_OF_PROFILES,// End of table entry (DON'T REMOVE THIS PROFILE !!!)
		0,		// disable_hotplug
		0,		// disable_hotplug_sleep
		0,		// down_threshold
		0,		// down_threshold_hotplug1
		0,		// down_threshold_hotplug2
		0,		// down_threshold_hotplug3
		0,		// down_threshold_hotplug4
		0,		// down_threshold_hotplug5
		0,		// down_threshold_hotplug6
		0,		// down_threshold_hotplug7
		0,		// down_threshold_hotplug_freq1
		0,		// down_threshold_hotplug_freq2
		0,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
		0,		// down_threshold_sleep
		0,		// early_demand
		0,		// early_demand_sleep
		0,		// fast_scaling
		0,		// fast_scaling_sleep
		0,		// freq_limit
		0,		// freq_limit_sleep
		0,		// freq_step
		0,		// freq_step_sleep
		0,		// grad_up_threshold
		0,		// grad_up_threshold_sleep
		0,		// hotplug_block_cycles
		0,		// hotplug_idle_threshold
		0,		// hotplug_sleep
		0,		// ignore_nice_load
		0,		// lcdfreq_enable
		0,		// lcdfreq_kick_in_cores
		0,		// lcdfreq_kick_in_down_delay
		0,		// lcdfreq_kick_in_freq
		0,		// lcdfreq_kick_in_up_delay
		0,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		0,		// sampling_down_momentum_sensitivity
		0,		// sampling_rate
		0,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		0,		// sampling_rate_idle_threshold
		0,		// sampling_rate_sleep_multiplier
		0,		// scaling_block_cycles
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		0,		// smooth_up
		0,		// smooth_up_sleep
		0,		// up_threshold
		0,		// up_threshold_hotplug1
		0,		// up_threshold_hotplug2
		0,		// up_threshold_hotplug3
		0,		// up_threshold_hotplug4
		0,		// up_threshold_hotplug5
		0,		// up_threshold_hotplug6
		0,		// up_threshold_hotplug7
		0,		// up_threshold_hotplug_freq1
		0,		// up_threshold_hotplug_freq2
		0,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
		0,		// up_threshold_sleep
		0		// legacy_mode
	}
};