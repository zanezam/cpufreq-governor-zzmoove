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

#define PROFILE_VERSION "0.8-beta1"
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
	unsigned int fast_scaling;
	unsigned int fast_scaling_sleep;
	unsigned int freq_limit;
	unsigned int freq_limit_sleep;
	unsigned int freq_step;
	unsigned int freq_step_sleep;
	unsigned int grad_up_threshold;
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
	unsigned int scaling_up_block_cycles;
	unsigned int scaling_up_block_freq;
	unsigned int scaling_up_block_threshold;
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
};

struct zzmoove_profile zzmoove_profiles[] = {
	{
		1,		// first profile
		"1-default",	// Default settings as hardcoded in the governor (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		52,		// down_threshold;
		55,		// down_threshold_hotplug1;
		55,		// down_threshold_hotplug2;
		55,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		0,		// down_threshold_hotplug_freq1;
		0,		// down_threshold_hotplug_freq2;
		0,		// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		44,		// down_threshold_sleep;
		0,		// early_demand;
		0,		// fast_scaling;
		0,		// fast_scaling_sleep;
		0,		// freq_limit;
		0,		// freq_limit_sleep;
		5,		// freq_step;
		5,		// freq_step_sleep;
		25,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		0,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		20,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		50,		// lcdfreq_kick_in_up_delay;
		0,		// sampling_down_factor;
		0,		// sampling_down_max_momentum;
		50,		// sampling_down_momentum_sensitivity;
		100000,		// sampling_rate;
		180000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		2,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		75,		// smooth_up;
		100,		// smooth_up_sleep;
		70,		// up_threshold;
		68,		// up_threshold_hotplug1;
		68,		// up_threshold_hotplug2;
		68,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		0,		// up_threshold_hotplug_freq1;
		0,		// up_threshold_hotplug_freq2;
		0,		// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		90,		// up_threshold_sleep;
	},
	{
		2,
		"2-yank-batt",	// Yank555.lu battery profile (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		40,		// down_threshold;
		65,		// down_threshold_hotplug1;
		75,		// down_threshold_hotplug2;
		85,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		600000,		// down_threshold_hotplug_freq1;
		800000,		// down_threshold_hotplug_freq2;
		1000000,	// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		75,		// down_threshold_sleep;
		1,		// early_demand;
		4,		// fast_scaling;
		0,		// fast_scaling_sleep;
		0,		// freq_limit;
		600000,		// freq_limit_sleep;
		10,		// freq_step;
		1,		// freq_step_sleep;
		50,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		1,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		20,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		50,		// lcdfreq_kick_in_up_delay;
		1,		// sampling_down_factor;
		0,		// sampling_down_max_momentum;
		50,		// sampling_down_momentum_sensitivity;
		75000,		// sampling_rate;
		180000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		4,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		65,		// smooth_up;
		90,		// smooth_up_sleep;
		60,		// up_threshold;
		85,		// up_threshold_hotplug1;
		90,		// up_threshold_hotplug2;
		98,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		700000,		// up_threshold_hotplug_freq1;
		1000000,	// up_threshold_hotplug_freq2;
		1200000,	// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		85,		// up_threshold_sleep;
	},
	{
		3,
		"3-yank-batt-ext",	// Yank555.lu extreme battery profile (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		50,		// down_threshold;
		70,		// down_threshold_hotplug1;
		80,		// down_threshold_hotplug2;
		90,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		800000,		// down_threshold_hotplug_freq1;
		900000,		// down_threshold_hotplug_freq2;
		1100000,	// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		75,		// down_threshold_sleep;
		1,		// early_demand;
		4,		// fast_scaling;
		0,		// fast_scaling_sleep;
		0,		// freq_limit;
		600000,		// freq_limit_sleep;
		10,		// freq_step;
		1,		// freq_step_sleep;
		50,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		1,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		20,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		50,		// lcdfreq_kick_in_up_delay;
		1,		// sampling_down_factor;
		0,		// sampling_down_max_momentum;
		50,		// sampling_down_momentum_sensitivity;
		60000,		// sampling_rate;
		180000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		4,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		75,		// smooth_up;
		90,		// smooth_up_sleep;
		70,		// up_threshold;
		90,		// up_threshold_hotplug1;
		95,		// up_threshold_hotplug2;
		98,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		900000,		// up_threshold_hotplug_freq1;
		1100000,	// up_threshold_hotplug_freq2;
		1300000,	// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		85,		// up_threshold_sleep;
	},
	{
		4,
		"4-battery",	// ZaneZam battery profile (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		40,		// down_threshold;
		45,		// down_threshold_hotplug1;
		55,		// down_threshold_hotplug2;
		65,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		600000,		// down_threshold_hotplug_freq1;
		800000,		// down_threshold_hotplug_freq2;
		1000000,	// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		60,		// down_threshold_sleep;
		0,		// early_demand;
		4,		// fast_scaling;
		0,		// fast_scaling_sleep;
		0,		// freq_limit;
		500000,		// freq_limit_sleep;
		10,		// freq_step;
		1,		// freq_step_sleep;
		50,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		1,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		5,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		1,		// lcdfreq_kick_in_up_delay;
		1,		// sampling_down_factor;
		0,		// sampling_down_max_momentum;
		50,		// sampling_down_momentum_sensitivity;
		100000,		// sampling_rate;
		180000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		4,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		75,		// smooth_up;
		100,		// smooth_up_sleep;
		95,		// up_threshold;
		60,		// up_threshold_hotplug1;
		80,		// up_threshold_hotplug2;
		98,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		700000,		// up_threshold_hotplug_freq1;
		1000000,	// up_threshold_hotplug_freq2;
		1200000,	// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		100,		// up_threshold_sleep;
	},
	{
		5,
		"5-optimized",	// ZaneZam optimized profile (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		52,		// down_threshold;
		45,		// down_threshold_hotplug1;
		55,		// down_threshold_hotplug2;
		65,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		400000,		// down_threshold_hotplug_freq1;
		600000,		// down_threshold_hotplug_freq2;
		800000,		// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		60,		// down_threshold_sleep;
		1,		// early_demand;
		1,		// fast_scaling;
		2,		// fast_scaling_sleep;
		0,		// freq_limit;
		500000,		// freq_limit_sleep;
		5,		// freq_step;
		1,		// freq_step_sleep;
		35,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		1,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		5,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		1,		// lcdfreq_kick_in_up_delay;
		4,		// sampling_down_factor;
		20,		// sampling_down_max_momentum;
		50,		// sampling_down_momentum_sensitivity;
		45000,		// sampling_rate;
		100000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		4,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		75,		// smooth_up;
		100,		// smooth_up_sleep;
		67,		// up_threshold;
		68,		// up_threshold_hotplug1;
		78,		// up_threshold_hotplug2;
		88,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		500000,		// up_threshold_hotplug_freq1;
		700000,		// up_threshold_hotplug_freq2;
		900000,		// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		100,		// up_threshold_sleep;
	},
	{
		6,
		"6-performance",	// ZaneZam performance profile (please don't remove this profile)
		0,		// disable_hotplug;
		0,		// disable_hotplug_sleep;
		20,		// down_threshold;
		25,		// down_threshold_hotplug1;
		35,		// down_threshold_hotplug2;
		45,		// down_threshold_hotplug3;
		55,		// down_threshold_hotplug4;
		55,		// down_threshold_hotplug5;
		55,		// down_threshold_hotplug6;
		55,		// down_threshold_hotplug7;
		300000,		// down_threshold_hotplug_freq1;
		700000,		// down_threshold_hotplug_freq2;
		900000,		// down_threshold_hotplug_freq3;
		0,		// down_threshold_hotplug_freq4;
		0,		// down_threshold_hotplug_freq5;
		0,		// down_threshold_hotplug_freq6;
		0,		// down_threshold_hotplug_freq7;
		60,		// down_threshold_sleep;
		1,		// early_demand;
		5,		// fast_scaling;
		2,		// fast_scaling_sleep;
		0,		// freq_limit;
		500000,		// freq_limit_sleep;
		25,		// freq_step;
		1,		// freq_step_sleep;
		25,		// grad_up_threshold;
		5,		// hotplug_block_cycles;
		0,		// hotplug_idle_threshold;
		1,		// hotplug_sleep;
		0,		// ignore_nice_load;
		0,		// lcdfreq_enable;
		0,		// lcdfreq_kick_in_cores;
		5,		// lcdfreq_kick_in_down_delay;
		500000,		// lcdfreq_kick_in_freq;
		1,		// lcdfreq_kick_in_up_delay;
		4,		// sampling_down_factor;
		50,		// sampling_down_max_momentum;
		25,		// sampling_down_momentum_sensitivity;
		40000,		// sampling_rate;
		100000,		// sampling_rate_idle;
		0,		// sampling_rate_idle_delay;
		40,		// sampling_rate_idle_threshold;
		4,		// sampling_rate_sleep_multiplier;
		5,		// scaling_up_block_cycles;
		500000,		// scaling_up_block_freq;
		50,		// scaling_up_block_threshold;
		70,		// smooth_up;
		100,		// smooth_up_sleep;
		60,		// up_threshold;
		65,		// up_threshold_hotplug1;
		75,		// up_threshold_hotplug2;
		85,		// up_threshold_hotplug3;
		68,		// up_threshold_hotplug4;
		68,		// up_threshold_hotplug5;
		68,		// up_threshold_hotplug6;
		68,		// up_threshold_hotplug7;
		400000,		// up_threshold_hotplug_freq1;
		800000,		// up_threshold_hotplug_freq2;
		1000000,	// up_threshold_hotplug_freq3;
		0,		// up_threshold_hotplug_freq4;
		0,		// up_threshold_hotplug_freq5;
		0,		// up_threshold_hotplug_freq6;
		0,		// up_threshold_hotplug_freq7;
		100,		// up_threshold_sleep;
	},
	{
		PROFILE_TABLE_END,
		END_OF_PROFILES,// End of table entry (DON'T REMOVE THIS PROFILE !!!)
		0,		// unsigned int sampling_rate
		0,		// unsigned int sampling_rate_sleep_multiplier
		0,		// unsigned int sampling_down_factor
		0,		// unsigned int sampling_down_max_mom
		0,		// unsigned int sampling_down_mom_sens
		0,		// unsigned int up_threshold
		0,		// unsigned int up_threshold_hotplug1
		0,		// unsigned int up_threshold_hotplug2
		0,		// unsigned int up_threshold_hotplug3
		0,		// unsigned int up_threshold_hotplug4
		0,		// unsigned int up_threshold_hotplug5
		0,		// unsigned int up_threshold_hotplug6
		0,		// unsigned int up_threshold_hotplug7
		0,		// unsigned int up_threshold_sleep
		0,		// unsigned int down_threshold
		0,		// unsigned int down_threshold_hotplug1
		0,		// unsigned int down_threshold_hotplug2
		0,		// unsigned int down_threshold_hotplug3
		0,		// unsigned int down_threshold_hotplug4
		0,		// unsigned int down_threshold_hotplug5
		0,		// unsigned int down_threshold_hotplug6
		0,		// unsigned int down_threshold_hotplug7
		0,		// unsigned int down_threshold_sleep
		0,		// unsigned int ignore_nice
		0,		// unsigned int freq_step
		0,		// unsigned int freq_step_sleep
		0,		// unsigned int smooth_up
		0,		// unsigned int smooth_up_sleep
		0,		// unsigned int hotplug_sleep
		0,		// unsigned int freq_limit
		0,		// unsigned int freq_limit_sleep
		0,		// unsigned int fast_scaling
		0,		// unsigned int fast_scaling_sleep
		0,		// unsigned int grad_up_threshold
		0,		// unsigned int early_demand
		0,		// unsigned int disable_hotplug
		0,		// int lcdfreq_enable
		0,		// unsigned int lcdfreq_kick_in_down_delay
		0,		// unsigned int lcdfreq_kick_in_up_delay
		0,		// unsigned int lcdfreq_kick_in_freq
		0		// unsigned int lcdfreq_kick_in_cores
	}
};