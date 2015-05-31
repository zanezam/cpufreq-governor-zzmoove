/*
 * drivers/cpufreq/cpufreq_zzmoove_profiles.h - Profiles
 *
 * Copyright (C)  2013 Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * ZZMoove Governor profiles header file modified by Zane Zaminsky 2013/14
 *
 * Changelog:
 *
 * Version 0.1 (inital) for governor Version 0.8
 *
 *  - idea and prototype version of this file by Yank555 (credits and thx!)
 *  - added all currently available settings and added tuneables from governor version 0.8 to all available settings
 *  - added version information variable and made changes for tuneable apply loop in governor
 *  - added descriptions for all tuneables in profile (1)
 *  - adjusted values for new features in all profiles
 *  - added new settings:
 *    'zzbatp' (a new battery friendly but still fast setting)
 *    'zzmod'  (optimized based 2 core moderate setting)
 *    'zzinz'  (based on performance with insane scaling enabled)
 *    'zzgame' (based on performance with scaling block enabled)
 *  - documentation added
 *
 * Version 0.2 for governor Version 0.9 alpha1 (Yank555.lu)
 *
 *  - split fast_scaling and fast_scaling_sleep into fast_scaling_up/fast_scaling_down
 *    and fast_scaling_sleep_up/fast_scaling_sleep_down
 *
 * Version 0.2 alpha2 for governor Version 0.9 alpha2
 *
 *  - corrected documentation
 *  - corrected version information
 *  - added auto fast scaling step tuneables
 *
 * Version 0.2 beta1 for governor Version 0.9 beta1
 *
 *  - bump version to beta for public
 *  - corrected version informations
 *
 * Version 0.2 beta2 for governor Version 0.9 beta3
 *
 *  - added values for following new tuneables (credits to ffolkes):
 *    hotplug_engage_freq (disabled by default in all profiles)
 *    scaling_fastdown_freq (disabled by default in all profiles)
 *    scaling_fastdown_up_threshold (default to 95 in all profiles)
 *    scaling_fastdown_down_threshold (default to 90 in all profiles)
 *    scaling_responsiveness_freq (disabled by default in all profiles)
 *    scaling_responsiveness_up_threshold (default to 30 in all profiles)
 *  - adjusted up/down thresholds for core 2 in moderate setting
 *  - changed sampling rate sleep multiplier from 4 to 6 in all settings (except in default setting)
 *
 * Version 0.2 beta3 for governor Version 0.9 beta4
 *
 *  - added scaling block temperature tuneable to all profiles if CONFIG_EXYNOS4_EXPORT_TEMP is defined
 *  - use CPU temperature treshold of 65°C instead of 15 scaling block cycles in game profile if
 *    CONFIG_EXYNOS4_EXPORT_TEMP is defined
 *  - added proportional frequency tuneable to all profiles
 *  - added auto adjust freq thresholds to all profiles (disabled by default)
 *  - enabled scaling proportional in ybat, ybatext, zzbat, zzbatp, zzmod and zzgame profile
 *  - enabled scaling fast down over 1200MHz and resposiveness over 400Mhz with up threshold of 20%
 *    in ybat, ybatext, zzbat, zzbatp, and zzmod profile
 *  - added core macros to exclude not used code like it is in governor
 *  - removed freq_step tuneable from all profiles
 *
 * Version 0.3 beta1 for governor Version 1.0 beta1
 *
 *  - bump version to 0.3 beta1 because of brought forward plan 'outbreak'
 *  - removed dynamic freq scaling tuneable leftovers
 *  - added macros to switch code depending of used power management implementation
 *    or used supend/resume backlight hook (opo specific)
 *  - added macros to be able to disable hotplugging
 *
 * Version 0.3 beta5 for governor Version 1.0 beta6
 *
 *  - added hotplug stagger up/down tuneables to all profiles
 *  - added hotplug up/down multiplier tuneables to all profiles
 *  - added hotplug max/min/lock tuneables to all profiles
 *
 * Version 0.3 beta6 for governor Version 1.0 beta6a (Andip71 aka Lord Boeffla)
 *
 *  - changed macro naming from CONFIG_BACKLIGHT_EXT_CONTROL to USE_LCD_NOTIFIER accordingly to
 *    governor version 1.0 beta6a
 *  - corrected version information in profiles file version variable
 *
 * Version 0.3 beta7 for governor Version 1.0 beta7 (sync)
 *
 *  - added PROFILE_MAX_FREQ definition for freq adaption functionality:
 *    this should hold the maximal possible freq (possible OC frequencies inclusive)
 *    for all the given profiles
 *  - added music min/max freq and music max cores tuneables to all profiles
 *  - setup default values for music* and inputbooster* tuneables in all profiles
 *  - reduced all hotplug block values to a minimum for faster hotplug reaction in all profiles
 *  - added new 'relax' profile with relaxed sleep settings and a forced 2 cores minimal hotplug setting
 *  - some documentation corrected
 *
 * currently available profiles by ZaneZam and Yank555:
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (1)'def'    -> Default              -> will set governor defaults                                                                     -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (2)'ybat    -> Yank Battery         -> a very good battery/performance balanced setting                                               -
 * -                                         DEV-NOTE: highly recommended!                                                                  -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (3)'ybatext'-> Yank Battery Extreme -> like yank battery but focus on battery saving                                                  -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (4)'zzbat'  -> ZaneZam Battery      -> a more 'harsh' setting strictly focused on battery saving                                      -
 * -                                         DEV-NOTE: might give some lags!                                                                -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (5)'zzbatp' -> ZaneZam Battery Plus -> NEW! reworked 'faster' battery setting                                                         -
 * -                                         DEV-NOTE: recommended too!:)                                                                   -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (6)'zzopt'  -> ZaneZam Optimized    -> balanced setting with no focus in any direction                                                -
 * -                                         DEV-NOTE: relict from back in the days, even though some people still like it!                 -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (7)'zzmod'  -> ZaneZam Moderate     -> NEW! setting based on 'zzopt' which has mainly (but not strictly only!) 2 cores online         -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (8)'zzperf' -> ZaneZam Performance  -> all you can get from zzmoove in terms of performance but still has the fast                    -
 * -                                         down scaling/hotplugging behaving                                                              -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * -  (9)'zzinz'  -> ZaneZam InZane       -> NEW! based on performance with new insane scaling active. a new experience!                    -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * - (10)'zzgame' -> ZaneZam Gaming       -> NEW! based on performance with scaling block enabled to avoid cpu overheating during gameplay  -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * - (11)'zzrelax'-> ZaneZam Relax        -> NEW! based on moderate (except hotplug settings) with relaxed sleep settings                   -
 * ------------------------------------------------------------------------------------------------------------------------------------------
 *
 * NOTE: be aware when setting tuneables which have a 'should' in comments below that giving
 *       them 'wrong' values can lead to odd hotplug behaving!
 *
 */

// NOTE: profile values in this version are mainly for Samsung (i9300) devices but might be compatible with other exynos devices too!
static char profiles_file_version[20] = "0.3 beta7";
#define PROFILE_TABLE_END ~1
#define END_OF_PROFILES "end"
#define PROFILE_MAX_FREQ (1920000)	// ZZ: max possible freq in system table for freq adaption (possible OC frequencies inclusive)

struct zzmoove_profile {
	unsigned int profile_number;
	char         profile_name[20];
	unsigned int auto_adjust_freq_thresholds;
#ifdef ENABLE_HOTPLUGGING
	unsigned int disable_hotplug;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int disable_hotplug_sleep;
#endif
#endif
	unsigned int down_threshold;
#ifdef ENABLE_HOTPLUGGING
	unsigned int down_threshold_hotplug1;
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int down_threshold_hotplug2;
	unsigned int down_threshold_hotplug3;
	unsigned int down_threshold_hotplug4;
#endif
#if (MAX_CORES == 8)
	unsigned int down_threshold_hotplug5;
	unsigned int down_threshold_hotplug6;
	unsigned int down_threshold_hotplug7;
#endif
	unsigned int down_threshold_hotplug_freq1;
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int down_threshold_hotplug_freq2;
	unsigned int down_threshold_hotplug_freq3;
	unsigned int down_threshold_hotplug_freq4;
#endif
#if (MAX_CORES == 8)
	unsigned int down_threshold_hotplug_freq5;
	unsigned int down_threshold_hotplug_freq6;
	unsigned int down_threshold_hotplug_freq7;
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int down_threshold_sleep;
#endif
	unsigned int early_demand;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int early_demand_sleep;
#endif
	unsigned int fast_scaling_up;
	unsigned int fast_scaling_down;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int fast_scaling_sleep_up;
	unsigned int fast_scaling_sleep_down;
#endif
	unsigned int afs_threshold1;
	unsigned int afs_threshold2;
	unsigned int afs_threshold3;
	unsigned int afs_threshold4;
	unsigned int freq_limit;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int freq_limit_sleep;
#endif
	unsigned int grad_up_threshold;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int grad_up_threshold_sleep;
#endif
#ifdef ENABLE_HOTPLUGGING
	unsigned int hotplug_block_up_cycles;
	unsigned int block_up_multiplier_hotplug1;
	unsigned int block_up_multiplier_hotplug2;
	unsigned int block_up_multiplier_hotplug3;
	unsigned int hotplug_block_down_cycles;
	unsigned int block_down_multiplier_hotplug1;
	unsigned int block_down_multiplier_hotplug2;
	unsigned int block_down_multiplier_hotplug3;
	unsigned int hotplug_stagger_up;
	unsigned int hotplug_stagger_down;
	unsigned int hotplug_idle_threshold;
	unsigned int hotplug_idle_freq;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int hotplug_sleep;
#endif
	unsigned int hotplug_engage_freq;
	unsigned int hotplug_max_limit;
	unsigned int hotplug_min_limit;
	unsigned int hotplug_lock;
#endif
	unsigned int ignore_nice_load;
	unsigned int sampling_down_factor;
	unsigned int sampling_down_max_momentum;
	unsigned int sampling_down_momentum_sensitivity;
	unsigned int sampling_rate;
	unsigned int sampling_rate_idle;
	unsigned int sampling_rate_idle_delay;
	unsigned int sampling_rate_idle_threshold;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int sampling_rate_sleep_multiplier;
#endif
	unsigned int scaling_block_cycles;
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	unsigned int scaling_block_temp;
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
	unsigned int scaling_trip_temp;
#endif
	unsigned int scaling_block_freq;
	unsigned int scaling_block_threshold;
	unsigned int scaling_block_force_down;
	unsigned int scaling_fastdown_freq;
	unsigned int scaling_fastdown_up_threshold;
	unsigned int scaling_fastdown_down_threshold;
	unsigned int scaling_responsiveness_freq;
	unsigned int scaling_responsiveness_up_threshold;
	unsigned int scaling_proportional;
#ifdef ENABLE_INPUTBOOSTER
	unsigned int inputboost_cycles;
	unsigned int inputboost_up_threshold;
	unsigned int inputboost_punch_cycles;
	unsigned int inputboost_punch_freq;
	unsigned int inputboost_punch_on_fingerdown;
	unsigned int inputboost_punch_on_fingermove;
	unsigned int inputboost_punch_on_epenmove;
	unsigned int inputboost_typingbooster_up_threshold;
	unsigned int inputboost_typingbooster_cores;
#endif /* ENABLE_INPUTBOOSTER */
	unsigned int music_max_freq;
	unsigned int music_min_freq;
	unsigned int music_min_cores;
	unsigned int smooth_up;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int smooth_up_sleep;
#endif
	unsigned int up_threshold;
#ifdef ENABLE_HOTPLUGGING
	unsigned int up_threshold_hotplug1;
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int up_threshold_hotplug2;
	unsigned int up_threshold_hotplug3;
	unsigned int up_threshold_hotplug4;
#endif
#if (MAX_CORES == 8)
	unsigned int up_threshold_hotplug5;
	unsigned int up_threshold_hotplug6;
	unsigned int up_threshold_hotplug7;
#endif
	unsigned int up_threshold_hotplug_freq1;
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int up_threshold_hotplug_freq2;
	unsigned int up_threshold_hotplug_freq3;
	unsigned int up_threshold_hotplug_freq4;
#endif
#if (MAX_CORES == 8)
	unsigned int up_threshold_hotplug_freq5;
	unsigned int up_threshold_hotplug_freq6;
	unsigned int up_threshold_hotplug_freq7;
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
	unsigned int up_threshold_sleep;
#endif
};

struct zzmoove_profile zzmoove_profiles[] = {
	{
		1,		// Default Profile
		"def",		// default settings as hardcoded in the governor (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds (any value=enable, 0=disable)
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug (1=disable hotplugging, 0=enable hotplugging)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep (1=disable hotplugging, 0=enable hotplugging)
#endif
#endif
		52,		// down_threshold (range from 11 to 100 and must be lower than up_threshold)
#ifdef ENABLE_HOTPLUGGING
		55,		// down_threshold_hotplug1 (range from 1 to 100 and should be lower than up_threshold_hotplug1)
#if (MAX_CORES == 4 || MAX_CORES == 8)
		55,		// down_threshold_hotplug2 (range from 1 to 100 and should be lower than up_threshold_hotplug2)
		55,		// down_threshold_hotplug3 (range from 1 to 100 and should be lower than up_threshold_hotplug3)
		55,		// down_threshold_hotplug4 (range from 1 to 100 and should be lower than up_threshold_hotplug4)
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5 (range from 1 to 100 and should be lower than up_threshold_hotplug5)
		55,		// down_threshold_hotplug6 (range from 1 to 100 and should be lower than up_threshold_hotplug6)
		55,		// down_threshold_hotplug7 (range from 1 to 100 and should be lower than up_threshold_hotplug7)
#endif
		0,		// down_threshold_hotplug_freq1 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq1)
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq2 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq2)
		0,		// down_threshold_hotplug_freq3 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq3)
		0,		// down_threshold_hotplug_freq4 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq4)
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq5)
		0,		// down_threshold_hotplug_freq6 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq6)
		0,		// down_threshold_hotplug_freq7 (range from 0 to scaling max and should be lower than up_threshold_hotplug_freq7)
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		44,		// down_threshold_sleep (range from 11 to 100 and must be lower than up_threshold_sleep)
#endif
		0,		// early_demand (any value=enable, 0=disable)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep (any value=enable, 0=disable)
#endif
		0,		// fast_scaling_up (range from 0 to 4)
		0,		// fast_scaling_down (range from 0 to 4)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up (range from 0 to 4)
		0,		// fast_scaling_sleep_down (range from 0 to 4)
#endif
		25,		// auto fast scaling step one (range from 1 to 100)
		50,		// auto fast scaling step two (range from 1 to 100)
		75,		// auto fast scaling step three (range from 1 to 100)
		90,		// auto fast scaling step four (range from 1 to 100)
		0,		// freq_limit (0=disable, range in system table from freq->min to freq->max in khz)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// freq_limit_sleep (0=disable, range in system table from freq->min to freq->max in khz)
#endif
		25,		// grad_up_threshold (range from 1 to 100)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep (range from 1 to 100)
#endif
#ifdef ENABLE_HOTPLUGGING
		2,		// hotplug_block_up_cycles (0=disable, any value above 0)
		1,		// block_up_multiplier_hotplug1 (1=disable hotplug up block cycles for 2nd core, 2 to 25x)
		1,		// block_up_multiplier_hotplug2 (1=disable hotplug up block cycles for 3rd core, 2 to 25x)
		1,		// block_up_multiplier_hotplug3 (1=disable hotplug up block cycles for 4th core, 2 to 25x)
		0,		// hotplug_block_down_cycles (0=disable, any value above 0)
		1,		// block_down_multiplier_hotplug1 (1=disable hotplug down block cycles for 2nd core, 2 to 25x)
		1,		// block_down_multiplier_hotplug2 (1=disable hotplug down block cycles for 3rd core, 2 to 25x)
		1,		// block_down_multiplier_hotplug3 (1=disable hotplug down block cycles for 4th core, 2 to 25x)
		0,		// hotplug_stagger_up (0=disable, any value above 0)
		0,		// hotplug_stagger_down (0=disable, any value above 0)
		0,		// hotplug_idle_threshold (0=disable, range from 1 to 100)
		0,		// hotplug_idle_freq (0=disable, range in system table from freq->min to freq->max in khz)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// hotplug_sleep (0=all cores enabled, range 1 to MAX_CORES)
#endif
		0,		// hotplug_engage_freq (0=disable, range in system table from freq->min to freq->max in khz)
		0,		// hotplug_max_limit (0=disable, range from 1 to MAX_CORES)
		0,		// hotplug_min_limit (0=disable, range from 1 to MAX_CORES)
		0,		// hotplug_lock (0=disable, range from 1 to MAX_CORES)
#endif
		0,		// ignore_nice_load (0=disable, 1=enable)
		1,		// sampling_down_factor (1=disable, range from 2 to MAX_SAMPLING_DOWN_FACTOR)
		0,		// sampling_down_max_momentum (0=disable, range from 1 to MAX_SAMPLING_DOWN_FACTOR)
		50,		// sampling_down_momentum_sensitivity (range from 1 to MAX_SAMPLING_DOWN_SENSITIVITY)
		100000,		// sampling_rate (range from MIN_SAMPLING_RATE to any value)
		180000,		// sampling_rate_idle (range from MIN_SAMPLING_RATE to any value)
		0,		// sampling_rate_idle_delay (0=disable, any value above 0)
		40,		// sampling_rate_idle_threshold (range from 1 to 100)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// sampling_rate_sleep_multiplier (range from 1 to 4)
#endif
		0,		// scaling_block_cycles (0=disable, any value above 0)
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp (0=disable, range from 30°C to 80°C)
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp (0=disable, range from 40°C to 69°C)
#endif
		1200000,	// scaling_block_freq (all valid system frequencies)
		10,		// scaling_block_threshold (0=disable, range from 1 to 100)
		2,		// scaling_block_force_down (0=disable, range from 2 to any value)
		0,		// scaling_fastdown_freq (0=disable, range in system table from freq->min to freq->max in khz)
		95,		// scaling_fastdown_up_threshold (range from over scaling_fastdown_down_threshold to 100)
		90,		// scaling_fastdown_down_threshold (range from 11 to under scaling_fastdown_up_threshold)
		0,		// scaling_responsiveness_freq (0=disable, range in system table from freq->min to freq->max in khz)
		30,		// scaling_responsiveness_up_threshold (0=disable, range from 11 to 100)
		0,		// scaling_proportional (0=disable, any value above 0)
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles (0=disable, range from 0 to 1000)
		80,		// inputboost_up_threshold (0=disable, range from 0 to 100)
		20,		// inputboost_punch_cycles (0= disable, range form 0 to 500)
		1000000,	// inputboost_punch_freq (0=disable, range from 0 to freq->max in khz)
		1,		// inputboost_punch_on_fingerdown (0=disable, any value above 0)
		0,		// inputboost_punch_on_fingermove (0=disable, any value above 0)
		0,		// inputboost_punch_on_epenmove (0=disable, any value above 0)
		40,		// inputboost_typingbooster_up_threshold (0=disable, range from 0 to 100)
		3,		// inputboost_typingbooster_cores (0=disable, range from 1 to MAX_CORES)
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq (0=disable, range from 0 to freq->max in khz)
		700000,		// music_min_freq (0=disable, range from 0 to freq->min in khz)
		2,		// music_min_cores (0=disable, range from 1 to MAX_CORES)
		75,		// smooth_up (range from 1 to 100)
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep (range from 1 to 100)
#endif
		70,		// up_threshold (range 1 to 100 and must be higher than down_threshold)
#ifdef ENABLE_HOTPLUGGING
		68,		// up_threshold_hotplug1 (range 1 to 100 and should be higher than down_threshold_hotplug1)
#if (MAX_CORES == 4 || MAX_CORES == 8)
		68,		// up_threshold_hotplug2 (range 1 to 100 and should be higher than down_threshold_hotplug2)
		68,		// up_threshold_hotplug3 (range 1 to 100 and should be higher than down_threshold_hotplug3)
		68,		// up_threshold_hotplug4 (range 1 to 100 and should be higher than down_threshold_hotplug4)
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5 (range 1 to 100 and should be higher than down_threshold_hotplug5)
		68,		// up_threshold_hotplug6 (range 1 to 100 and should be higher than down_threshold_hotplug6)
		68,		// up_threshold_hotplug7 (range 1 to 100 and should be higher than down_threshold_hotplug7)
#endif
		0,		// up_threshold_hotplug_freq1 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq1)
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq2 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq2)
		0,		// up_threshold_hotplug_freq3 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq3)
		0,		// up_threshold_hotplug_freq4 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq4)
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq5)
		0,		// up_threshold_hotplug_freq6 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq6)
		0,		// up_threshold_hotplug_freq7 (0 to disable core, range from 1 to scaling max and should be higher than down_threshold_hotplug_freq7)
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		90		// up_threshold_sleep (range from above down_threshold_sleep to 100)
#endif
	},
	{
		2,
		"ybat",		// Yank555.lu Battery Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		40,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		65,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		75,		// down_threshold_hotplug2
		85,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		800000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1000000,	// down_threshold_hotplug_freq2
		1200000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		75,		// down_threshold_sleep
#endif
		0,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// early_demand_sleep
#endif
		5,		// fast_scaling_up
		2,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		600000,		// freq_limit_sleep
#endif
		50,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		0,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		75000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		95,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		90,		// smooth_up_sleep
#endif
		60,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		85,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		90,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		1000000,	// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1200000,	// up_threshold_hotplug_freq2
		1400000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		85		// up_threshold_sleep
#endif
	},
	{
		3,
		"ybatext",	// Yank555.lu Battery Extreme Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		50,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		70,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		80,		// down_threshold_hotplug2
		90,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		800000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1000000,	// down_threshold_hotplug_freq2
		1200000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		75,		// down_threshold_sleep
#endif
		0,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// early_demand_sleep
#endif
		5,		// fast_scaling_up
		3,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		600000,		// freq_limit_sleep
#endif
		50,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		0,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		60000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		95,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		90,		// smooth_up_sleep
#endif
		70,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		90,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		95,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		1000000,	// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1200000,	// up_threshold_hotplug_freq2
		1400000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		85		// up_threshold_sleep
#endif
	},
	{
		4,
		"zzbat",	// ZaneZam Battery Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		40,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		45,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		55,		// down_threshold_hotplug2
		65,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		600000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		800000,		// down_threshold_hotplug_freq2
		1000000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		60,		// down_threshold_sleep
#endif
		0,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		0,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		50,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		100000,		// sampling_rate
		180000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		75,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		95,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		60,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		80,		// up_threshold_hotplug2
		98,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		700000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1000000,	// up_threshold_hotplug_freq2
		1200000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		5,
		"zzbatp",	// ZaneZam Battery Plus Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		70,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		20,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		70,		// down_threshold_hotplug2
		80,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		500000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		600000,		// down_threshold_hotplug_freq2
		1100000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		65,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		0,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		60,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		2,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		0,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		1,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		120000,		// sampling_rate
		200000,		// sampling_rate_idle
		5,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		80,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		75,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		20,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		50,		// up_threshold_hotplug2
		90,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		500000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1100000,	// up_threshold_hotplug_freq2
		1200000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		6,
		"zzopt",	// ZaneZam Optimized Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		52,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		45,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		55,		// down_threshold_hotplug2
		65,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		400000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		600000,		// down_threshold_hotplug_freq2
		800000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		60,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		1,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		35,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		20,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		45000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		0,		// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		0,		// scaling_responsiveness_freq
		0,		// scaling_responsiveness_up_threshold
		0,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		75,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		67,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		68,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		78,		// up_threshold_hotplug2
		88,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		500000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		700000,		// up_threshold_hotplug_freq2
		900000,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		7,
		"zzmod",	// ZaneZam Moderate Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		52,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		30,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		60,		// down_threshold_hotplug2
		70,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		300000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq2
		0,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		55,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		3,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		40,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		0,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		20,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		45000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		68,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		60,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		20,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		100,		// up_threshold_hotplug2
		100,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		400000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq2
		0,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		8,
		"zzperf",	// ZaneZam Performance Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		20,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		25,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		35,		// down_threshold_hotplug2
		45,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		300000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		700000,		// down_threshold_hotplug_freq2
		900000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		60,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		1,		// fast_scaling_up
		1,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		25,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		50,		// sampling_down_max_momentum
		25,		// sampling_down_momentum_sensitivity
		40000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		0,		// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		0,		// scaling_responsiveness_freq
		0,		// scaling_responsiveness_up_threshold
		0,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		70,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		60,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		65,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		75,		// up_threshold_hotplug2
		85,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		400000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		800000,		// up_threshold_hotplug_freq2
		1000000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		9,
		"zzinz",	// ZaneZam InZane Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		20,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		25,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		35,		// down_threshold_hotplug2
		45,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		200000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		600000,		// down_threshold_hotplug_freq2
		800000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		60,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		5,		// fast_scaling_up
		5,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		25,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		80,		// sampling_down_max_momentum
		15,		// sampling_down_momentum_sensitivity
		40000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		0,		// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		0,		// scaling_responsiveness_freq
		0,		// scaling_responsiveness_up_threshold
		0,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		60,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		50,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		60,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		70,		// up_threshold_hotplug2
		80,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		300000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		700000,		// up_threshold_hotplug_freq2
		900000,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		10,
		"zzgame",	// ZaneZam Game Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		20,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		25,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		35,		// down_threshold_hotplug2
		45,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		300000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		700000,		// down_threshold_hotplug_freq2
		900000,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		60,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		0,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		500000,		// freq_limit_sleep
#endif
		25,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		60,		// sampling_down_max_momentum
		20,		// sampling_down_momentum_sensitivity
		40000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		6,		// sampling_rate_sleep_multiplier
#endif
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_cycles
		65,		// scaling_block_temp
#else
		15,		// scaling_block_cycles
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		1100000,	// scaling_block_freq
		5,		// scaling_block_threshold
		3,		// scaling_block_force_down
		0,		// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		0,		// scaling_responsiveness_freq
		0,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		70,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100,		// smooth_up_sleep
#endif
		60,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		65,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		75,		// up_threshold_hotplug2
		85,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		400000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		800000,		// up_threshold_hotplug_freq2
		1000000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		100		// up_threshold_sleep
#endif
	},
	{
		11,
		"zzrelax",	// ZaneZam Relax Profile (please don't remove this profile)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		52,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		30,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		60,		// down_threshold_hotplug2
		70,		// down_threshold_hotplug3
		55,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		55,		// down_threshold_hotplug5
		55,		// down_threshold_hotplug6
		55,		// down_threshold_hotplug7
#endif
		300000,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		800000,		// down_threshold_hotplug_freq2
		1200000,	// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		59,		// down_threshold_sleep
#endif
		1,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		1,		// early_demand_sleep
#endif
		3,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		30,		// afs_threshold1
		50,		// afs_threshold2
		70,		// afs_threshold3
		90,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// freq_limit_sleep
#endif
		40,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		28,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		1,		// block_up_multiplier_hotplug1
		1,		// block_up_multiplier_hotplug2
		1,		// block_up_multiplier_hotplug3
		2,		// hotplug_block_down_cycles
		1,		// block_down_multiplier_hotplug1
		1,		// block_down_multiplier_hotplug2
		1,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		2,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		4,		// sampling_down_factor
		20,		// sampling_down_max_momentum
		50,		// sampling_down_momentum_sensitivity
		45000,		// sampling_rate
		100000,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		40,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		2,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// scaling_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		2,		// scaling_block_force_down
		1200000,	// scaling_fastdown_freq
		95,		// scaling_fastdown_up_threshold
		90,		// scaling_fastdown_down_threshold
		400000,		// scaling_responsiveness_freq
		20,		// scaling_responsiveness_up_threshold
		1,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		80,		// inputboost_up_threshold
		20,		// inputboost_punch_cycles
		1000000,	// inputboost_punch_freq
		1,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		40,		// inputboost_typingbooster_up_threshold
		3,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		1200000,	// music_max_freq
		700000,		// music_min_freq
		2,		// music_min_cores
		68,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		68,		// smooth_up_sleep
#endif
		60,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		68,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		78,		// up_threshold_hotplug2
		88,		// up_threshold_hotplug3
		68,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		68,		// up_threshold_hotplug5
		68,		// up_threshold_hotplug6
		68,		// up_threshold_hotplug7
#endif
		400000,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		1000000,	// up_threshold_hotplug_freq2
		1300000,	// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		70		// up_threshold_sleep
#endif
	},
	{
		PROFILE_TABLE_END,
		END_OF_PROFILES,// End of table entry (DON'T REMOVE THIS PROFILE !!!)
		0,		// auto_adjust_freq_thresholds
#ifdef ENABLE_HOTPLUGGING
		0,		// disable_hotplug
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// disable_hotplug_sleep
#endif
#endif
		0,		// down_threshold
#ifdef ENABLE_HOTPLUGGING
		0,		// down_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// down_threshold_hotplug2
		0,		// down_threshold_hotplug3
		0,		// down_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug5
		0,		// down_threshold_hotplug6
		0,		// down_threshold_hotplug7
#endif
		0,		// down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq2
		0,		// down_threshold_hotplug_freq3
		0,		// down_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// down_threshold_hotplug_freq5
		0,		// down_threshold_hotplug_freq6
		0,		// down_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// down_threshold_sleep
#endif
		0,		// early_demand
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// early_demand_sleep
#endif
		0,		// fast_scaling_up
		0,		// fast_scaling_down
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// fast_scaling_sleep_up
		0,		// fast_scaling_sleep_down
#endif
		0,		// afs_threshold1
		0,		// afs_threshold2
		0,		// afs_threshold3
		0,		// afs_threshold4
		0,		// freq_limit
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// freq_limit_sleep
#endif
		0,		// grad_up_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// grad_up_threshold_sleep
#endif
#ifdef ENABLE_HOTPLUGGING
		0,		// hotplug_block_up_cycles
		0,		// block_up_multiplier_hotplug1
		0,		// block_up_multiplier_hotplug2
		0,		// block_up_multiplier_hotplug3
		0,		// hotplug_block_down_cycles
		0,		// block_down_multiplier_hotplug1
		0,		// block_down_multiplier_hotplug2
		0,		// block_down_multiplier_hotplug3
		0,		// hotplug_stagger_up
		0,		// hotplug_stagger_down
		0,		// hotplug_idle_threshold
		0,		// hotplug_idle_freq
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// hotplug_sleep
#endif
		0,		// hotplug_engage_freq
		0,		// hotplug_max_limit
		0,		// hotplug_min_limit
		0,		// hotplug_lock
#endif
		0,		// ignore_nice_load
		0,		// sampling_down_factor
		0,		// sampling_down_max_momentum
		0,		// sampling_down_momentum_sensitivity
		0,		// sampling_rate
		0,		// sampling_rate_idle
		0,		// sampling_rate_idle_delay
		0,		// sampling_rate_idle_threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// sampling_rate_sleep_multiplier
#endif
		0,		// scaling_block_cycles
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		0,		// hotplug_block_temp
#endif
#ifdef ENABLE_SNAP_THERMAL_SUPPORT
		0,		// scaling_trip_temp
#endif
		0,		// scaling_block_freq
		0,		// scaling_block_threshold
		0,		// scaling_block_force_down
		0,		// scaling_fastdown_freq
		0,		// scaling_fastdown_up_threshold
		0,		// scaling_fastdown_down_threshold
		0,		// scaling_responsiveness_freq
		0,		// scaling_responsiveness_up_threshold
		0,		// scaling_proportional
#ifdef ENABLE_INPUTBOOSTER
		0,		// inputboost_cycles
		0,		// inputboost_up_threshold
		0,		// inputboost_punch_cycles
		0,		// inputboost_punch_freq
		0,		// inputboost_punch_on_fingerdown
		0,		// inputboost_punch_on_fingermove
		0,		// inputboost_punch_on_epenmove
		0,		// inputboost_typingbooster_up_threshold
		0,		// inputboost_typingbooster_cores
#endif /* ENABLE_INPUTBOOSTER */
		0,		// music_max_freq
		0,		// music_min_freq
		0,		// music_min_cores
		0,		// smooth_up
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0,		// smooth_up_sleep
#endif
		0,		// up_threshold
#ifdef ENABLE_HOTPLUGGING
		0,		// up_threshold_hotplug1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// up_threshold_hotplug2
		0,		// up_threshold_hotplug3
		0,		// up_threshold_hotplug4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug5
		0,		// up_threshold_hotplug6
		0,		// up_threshold_hotplug7
#endif
		0,		// up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq2
		0,		// up_threshold_hotplug_freq3
		0,		// up_threshold_hotplug_freq4
#endif
#if (MAX_CORES == 8)
		0,		// up_threshold_hotplug_freq5
		0,		// up_threshold_hotplug_freq6
		0,		// up_threshold_hotplug_freq7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(USE_LCD_NOTIFIER)
		0		// up_threshold_sleep
#endif
	}
};
