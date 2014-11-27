/*
 *  drivers/cpufreq/cpufreq_zzmoove.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *            (C)  2012 Michael Weingaertner <mialwe@googlemail.com>
 *                      Zane Zaminsky <cyxman@yahoo.com>
 *                      Jean-Pierre Rasquin <yank555.lu@gmail.com>
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
 * Weingaertner <mialwe@googlemail.com> (source: https://github.com/mialwe/mngb/) modified by Zane Zaminsky since November 2012 (and further improved
 * in 2013/14) to be hotplug-able and optimzed for use with Samsung I9300. CPU Hotplug modifications partially taken from ktoonservative governor from
 * ktoonsez KT747-JB kernel (https://github.com/ktoonsez/KT747-JB)
 *
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 * -  ZZMoove Governor by ZaneZam Changelog:														  -
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
 *	  up_threshold_sleep		 -> up threshold on early suspend (possible range from above 'down_threshold_sleep' up to 100, default: 90)
 *	  down_threshold_sleep		 -> down threshold on early suspend (possible range from 11 to 'under up_threshold_sleep', default: 44)
 *	  smooth_up_sleep		 -> smooth up scaling on early suspend (possible range from 1 to 100, default: 100)
 *	  up_threshold_hotplug1		 -> hotplug threshold for cpu1 (0 disable core1, possible range from 'down_threshold' up to 100, default: 68)
 *	  up_threshold_hotplug2		 -> hotplug threshold for cpu2 (0 disable core2, possible range from 'down_threshold' up to 100, default: 68)
 *	  up_threshold_hotplug3		 -> hotplug threshold for cpu3 (0 disable core3, possible range from 'down_threshold' up to 100, default: 68)
 *	  down_threshold_hotplug1	 -> hotplug threshold for cpu1 (possible range from 1 to under 'up_threshold', default: 55)
 *	  down_threshold_hotplug2	 -> hotplug threshold for cpu2 (possible range from 1 to under 'up_threshold', default: 55)
 *	  down_threshold_hotplug3	 -> hotplug threshold for cpu3 (possible range from 1 to under 'up_threshold', default: 55)
 *
 * Version 0.3 - more improvements
 *
 *	- added tuneable 'hotplug_sleep' to be able to turn cores offline only on early suspend (screen off) via sysfs
 *	  possible values: 0 do not touch hotplug-settings on early suspend, values 1, 2 or 3 are equivalent to
 *	  cores which should be online at early suspend
 *	- modified scaling frequency table to match 'overclock' freqencies to max 1600 mhz
 *	- fixed black screen of dead problem in hotplug logic due to missing mutexing on 3-core and 2-core settings
 *	- code cleaning and documentation
 *
 * Version 0.4 - limits
 *
 *	- added 'soft'-freqency-limit. the term 'soft' means here that this is unfortuneately not a hard limit. a hard limit is only possible with
 *	  cpufreq driver which is the freqency 'giver' the governor is only the 'consultant'. So now the governor will scale up to only the given up
 *	  limit on higher system load but if the cpufreq driver 'wants' to go above that limit the freqency will go up there. u can see this for
 *	  example with touchboost or wake up freqencies (1000 and 800 mhz) where the governor obviously will be 'bypassed' by the cpufreq driver.
 *	  but nevertheless this soft-limit will now reduce the use of freqencies higer than given limit and therefore it will reduce power consumption.
 *
 *	  for this function following new tuneables were indroduced:
 *
 *	  freq_limit_sleep		 -> limit freqency at early suspend (possible values 0 disable limit, 200-1600, default: 0)
 *	  freq_limit			 -> limit freqency at awake (possible values 0 disable limit, 200-1600, default: 0)
 *
 *	- added scaling frequencies to frequency tables for a faster up/down scaling. This should bring more performance but on the other hand it
 *	  can be of course a little bit more power consumptive.
 *
 *	  for this function following new tuneables were indroduced:
 *
 *	  fast_scaling			 -> fast scaling at awake (possible values 0 disable or 1 enable, default: 0)
 *	  fast_scaling_sleep (sysfs)	 -> fast scaling at early suspend (possible values 0 disable or 1 enable, default: 0)
 *
 *	- added tuneable 'freq_step_sleep' for setting the freq step at early suspend (possible values same as freq_step 0 to 100, default 5)
 *	- added DEF_FREQ_STEP and IGNORE_NICE macros
 *	- changed downscaling cpufreq relation to the 'lower way'
 *	- code/documentation cleaning
 *
 * Version 0.5 - performance and fixes
 *
 *	- completely reworked fast scaling functionality. now using a 'line jump' logic instead of fixed freq 'colums'.
 *	  fast scaling now in 4 steps and 2 modes possible (mode 1: only fast scaling up and mode2: fast scaling up/down)
 *	- added support for 'Dynamic Screen Frequency Scaling' (original implementation into zzmoove governor highly improved by Yank555)
 *	  originated by AndreiLux more info: http://forum.xda-developers.com/showpost.php?p=38499071&postcount=3
 *	- re-enabled broken conservative sampling down factor functionality ('down skip' method).
 *	  originated by Stratosk - upstream kernel 3.10rc1:
 *	  https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/log/?id=refs%2Ftags%2Fv3.10-rc1&qt=author&q=Stratos+Ka
 *	- changed down threshold check to act like it should.
 *	  originated by Stratosk - upstream kernel 3.10rc1:
 *	  https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/log/?id=refs%2Ftags%2Fv3.10-rc1&qt=author&q=Stratos+Ka
 *	- implemented/ported 'early demand' from ondemand governor.
 *	  originated by Stratosk - more info: http://www.semaphore.gr/80-latests/98-ondemand-early-demand
 *	- implemented/ported 'sampling down momentum' from ondemand governor.
 *	  originated by Stratosk - more info: http://www.semaphore.gr/80-latests/80-sampling-down-momentum
 *	- modified some original conservative code parts regarding frequency scaling which should work better now.
 *	  originated by DerTeufel1980: https://github.com/DerTeufel/android_kernel_samsung_smdk4412/commit/6bab622344c548be853db19adf28c3917896f0a0
 *	- added the possibility to use sampling down momentum or conservative 'down skip' method.
 *	- increased possible max sampling rate sleep multiplier to 4 and sampling down factor to 100000
 *	  accordingly to sampling down momentum implementation.
 *	- added frequency search limit for more efficient frequency searching in scaling 'table' and for improving
 *	  frequency 'hard' and 'soft' limit handling.
 *	- added cpu idle exit time handling like it is in lulzactive
 *	  again work from ktoonsez : https://github.com/ktoonsez/KT747-JB/commit/a5931bee6ea9e69f386a340229745da6f2443b78
 *	  description in lulzactive governor:
 *	  https://github.com/ktoonsez/KT747-JB/blob/a5931bee6ea9e69f386a340229745da6f2443b78/drivers/cpufreq/cpufreq_lulzactive.c
 *	- fixed a little scaling step mistake and added overclocking frequencies up to 1800 mhz in scaling frequency 'tables'.
 *	- fixed possible freezes during start/stop/reload of governor and frequency limit change.
 *	- fixed hotplugging logic at online core 0+3 or 0+2 situations and improved hotplugging in general by
 *	  removing mutex locks and skipping hotplugging when it is not needed.
 *	- added possibility to disable hotplugging (that's a debugging relict but i thought maybe someone will find that usefull so i didn't remove it)
 *	- try to fix lags when coming from suspend if hotplug limitation at sleep was active by enabling all offline cores during resume.
 *	- code cleaning and documentation.
 *
 *	  for this functions following new tuneables were indroduced:
 *
 *	  Early Demand:
 *	  -------------
 *	  early_demand			-> switch to enable/disable early demand functionality (possible values 0 disable or 1 enable, default: 0)
 *	  grad_up_threshold		-> scale up frequency if the load goes up in one step of grad up value (possible range from 1 to 100, default 50)
 *	                                   little example for understanding: when the load rises up in one big 50% step then the
 *	                                   frequency will be scaled up immediately instead of wating till up_threshold is reached.
 *
 *	  Fast Scaling (improved):
 *	  ------------------------
 *	  Fast scaling has now 8 levels which at the same time have 2 modes included. Values from 1-4 equals to scaling jumps in the frequency table
 *	  and uses the Fast Scaling up but normal scaling down mode. Values from 5-8 equals to 1-4 scaling jumps but uses the fast scaling up and fast
 *	  scaling down mode.
 *
 *	  Hotplugging switch:
 *	  -------------------
 *	  disable_hotplug		-> switch to enable/disable hotplugging (possible values are any value above 0 to disable hotplugging and 0 to
 *	                                   enable it, default 0)
 *
 *	  Sampling Down Factor and Sampling Down Momentum:
 *	  ------------------------------------------------
 *	  Description: From the original author of ondemand_sampling_factor David Niemi:
 *	  'This improves performance by reducing the overhead of load evaluation and helping the CPU stay
 *	  at its top speed when truly busy, rather than shifting back and forth in speed.'
 *
 *	  And that 'Sampling Down Momentum' function from stratosk does this dynamicly now! ;)
 *
 *	  sampling_down_max_momentum		-> max sampling down factor which should be set by momentum (0 disable momentum, possible range from
 *	                                           sampling_down_factor up to MAX_SAMPLING_DOWN_FACTOR, default 0 disabled)
 *	  sampling_down_momentum_sensitivity	-> how fast the sampling down factor should be switched (possible values from 1 to 500, default 50)
 *	  sampling_down_factor			-> depending on which mode is active the factor for sampling rate multiplier which influences the whole
 *	                                           sampling rate or the value for stock 'down skip' functionality which influences only the down scaling
 *	                                           mechanism (possible values are from 1 to MAX_SMPLING_DOWN_FACTOR, default 1 disabled)
 *
 *	  Original conservative 'down skip' or 'stock' method can be enabled by setting the momentum tuneable to 0. so if momentum is inactive there will
 *	  be a fallback to the stock method. as the name 'down skip' says this method works 'slightly' different from the ondemand stock sampling down method
 *	  (on which momentum was based on). It just skips the scaling down code for the given samples. if u want to completely disable the sampling down
 *	  functionality u can achieve this by setting sampling down factor to 1. so concluded: setting sampling_down_momentum = 0 and sampling_down_factor = 1
 *	  will disable sampling down completely (that is also the governor default setting)
 *
 *	  Dynamic Screen Frequency Scaling:
 *	  --------------------------------
 *
 *	  Dynamicly switches the screen frequency to 40hz or 60hz depending on cpu scaling and hotplug settings.
 *	  For compiling and enabling this functionality u have to do some more modification to the kernel sources, please take a look at AndreiLux Perseus
 *	  repository and there at following commit: https://github.com/AndreiLux/Perseus-S3/commit/3476799587d93189a091ba1db26a36603ee43519
 *	  After adding this patch u can enable the feature by setting 'CPU_FREQ_LCD_FREQ_DFS=y' in your kernel config and if u want to check if it is
 *	  really working at runtime u can also enable the accounting which AndreiLux added by setting LCD_FREQ_SWITCH_ACCOUNTING=y in the kernel config.
 *	  If all goes well and u have the DFS up and running u can use following tuneables to do some screen magic:
 *	  (thx to Yank555 for highly extend and improving this!)
 *
 *	  lcdfreq_enable		-> to enable/disable LCDFreq scaling (possible values 0 disable or 1 enable, default: 0)
 *	  lcdfreq_kick_in_down_delay	-> the amount of samples to wait below the threshold frequency before entering low display frequency mode (40hz)
 *	  lcdfreq_kick_in_up_delay	-> the amount of samples to wait over the threshold frequency before entering high display frequency mode (60hz)
 *	  lcdfreq_kick_in_freq		-> the frequency threshold - below this cpu frequency the low display frequency will be active
 *	  lcdfreq_kick_in_cores		-> the number of cores which should be online before switching will be active. (also useable in combination
 *	                                   with kickin_freq)
 *
 *	  So this version is a kind of 'featured by' release as i took (again *g*) some ideas and work from other projects and even some of that work
 *	  comes directly from other devs so i wanna thank and give credits:
 *
 *	  First of all to stratosk for his great work 'sampling down momentum' and 'early demand' and for all the code fixes which found their way into
 *	  the upstream kernel version of conservative governor! congrats and props on that stratos, happy to see such a nice and talented dev directly
 *	  contibuting to the upstream kernel, that is a real enrichment for all of us!
 *
 *	  Second to Yank555 for coming up with the idea and improving/completeing (leaves nothing to be desired now *g*) my first
 *	  rudimentary implementation of Dynamic Screen Frequency Scaling from AndreiLux (credits for the idea/work also to him at this point!).
 *
 *	  Third to DerTeufel1980 for his first implementation of stratosk's early demand functionality into version 0.3 of zzmoove governor
 *	  (even though i had to modify the original implementation a 'little bit' to get it working properly ;)) and for some code optimizations/fixes
 *	  regarding scaling.
 *
 *	  Last but not least again to ktoonsez - I 'cherry picked' again some code parts of his ktoonservative governor which should improve this governor
 *	  too.
 *
 * Version 0.5.1b - bugfixes and more optimisations (in cooperation with Yank555)
 *
 *	- highly optimised scaling logic (thx and credits to Yank555)
 *	- simplified some tuneables by using already available stuff instead of using redundant code (thx Yank555)
 *	- reduced/optimised hotplug logic and preperation for automatic detection of available cores
 *	  (maybe this fixes also the scaling/core stuck problems)
 *	- finally fixed the freezing issue on governor stop!
 *
 * Version 0.6 - flexibility (in cooperation with Yank555)
 *
 *	- removed fixed scaling lookup tables and use the system frequency table instead
 *	  changed scaling logic accordingly for this modification (thx and credits to Yank555)
 *	- reduced new hotplug logic loop to a minimum
 *	- again try to fix stuck issues by using seperate hotplug functions out of dbs_check_cpu (credits to ktoonesz)
 *	- added support for 2 and 8 core systems and added automatic detection of cores were it is needed
 *	  (for setting the different core modes you can use the macro 'MAX_CORES'. possible values are: 2,4 or 8, default are 4 cores)
 *	  reduced core threshold defaults to only one up/down default and use an array to hold all threshold values
 *	- fixed some mistakes in 'frequency tuneables' (Yank555):
 *	  stop looping once the frequency has been found
 *	  return invalid error if new frequency is not found in the frequency table
 *
 * Version 0.6a - scaling logic flexibility (in cooperation with Yank555)
 *
 *	- added check if CPU freq. table is in ascending or descending order and scale accordingly
 *	  (compatibility for systems with 'inverted' frequency table like it is on OMAP4 platform)
 *	  thanks and credits to Yank555!
 *
 * Version 0.7 - slow down (in cooperation with Yank555)
 *
 *	- reindroduced the 'old way' of hotplugging and scaling in form of the 'Legacy Mode' (macros for enabling/disabling this done by Yank555, thx!)
 *	  NOTE: this mode can only handle 4 cores and a scaling max frequency up to 1800mhz.
 *	- added hotplug idle threshold for a balanced load at CPU idle to reduce possible higher idle temperatures when running on just one core.
 *        (inspired by JustArchi's observations, thx!)
 *	- added hotplug block cycles to reduce possible hotplugging overhead (credits to ktoonsez)
 *	- added possibility to disable hotplugging only at suspend (inspired by a request of STAticKY, thx for the idea)
 *	- introduced hotplug frequency thresholds (credits to Yank555)
 *	- hotplug tuneables handling optimized (credits to Yank555)
 *	- added version information tuneable (credits to Yank555)
 *
 *	  for this functions following new tuneables were indroduced:
 *
 *	  legacy_mode			-> for switching to the 'old' method of scaling/hotplugging. possible values 0 to disable,
 *					   any values above 0 to enable (default is 0)
 *					   NOTE: the legacy mode has to be enabled by uncommenting the macro ENABLE_LEGACY_MODE below!
 *	  hotplug_idle_threshold	-> amount of load under which hotplugging should be disabled at idle times (respectively at scaling minimum).
 *
 *	  hotplug_block_cycles		-> slow down hotplugging by waiting a given amount of cycles before plugging.
 *					   possible values 0 disbale, any values above 0 (default is 0)
 *	  disable_hotplug_sleep		-> same as disable_hotplug but will only take effect at suspend.
 *					   possible values 0 disable, any values above 0 to enable (default is 0)
 *	  up_threshold_hotplug_freq1	-> hotplug up frequency threshold for core1.
 *					   possible values 0 disable and range from over down_threshold_hotplug_freq1 to max scaling freqency (default is 0)
 *	  up_threshold_hotplug_freq2	-> hotplug up frequency threshold for core2.
 *					   possible values 0 disable and range from over down_threshold_hotplug_freq2 to max scaling freqency (default is 0)
 *	  up_threshold_hotplug_freq3	-> hotplug up frequency threshold for core3.
 *					   possible values 0 disable and range from over down_threshold_hotplug_freq3 to max scaling freqency (default is 0)
 *	  down_threshold_hotplug_freq1	-> hotplug down frequency threshold for core1.
 *					   possible values 0 disable and range from min saling to under up_threshold_hotplug_freq1 freqency (default is 0)
 *	  down_threshold_hotplug_freq2	-> hotplug down frequency threshold for core2.
 *					   possible values 0 disable and range from min saling to under up_threshold_hotplug_freq2 freqency (default is 0)
 *	  down_threshold_hotplug_freq3	-> hotplug down frequency threshold for core3.
 *					   possible values 0 disable and range from min saling to under up_threshold_hotplug_freq3 freqency (default is 0)
 *	  version			-> show the version of zzmoove governor
 *
 * Version 0.7a - little fix
 *
 *	- fixed a glitch in hotplug freq threshold tuneables which prevented setting of values in hotplug down freq thresholds when hotplug
 *	  up freq thresholds were set to 0
 *
 * Version 0.7b - compatibility improved and forgotten things
 *
 *	- fixed stuck at max scaling frequency when using stock kernel sources with unmodified cpufreq driver and without any oc capabilities.
 *	- readded forgotten frequency search optimisation in scaling logic (only effective when using governor soft or cpufreq frequency limit)
 *	- readded forgotten minor optimisation in dbs_check_cpu function
 *	- as forgotten to switch in last version Legacy Mode now again disabled by default
 *	- minor code format and comment fixes
 *
 * Version 0.7c - again compatibility and optimisations
 *
 *	- frequency search optimisation now fully compatible with ascending ordered system frequency tables (thx to psndna88 for testing!)
 *	- again minor optimisations at multiple points in dbs_check_cpu function
 *	- code cleaning - removed some unnecessary things and whitespaces nuked (sry for the bigger diff but from now on it will be clean ;))
 *	- corrected changelog for previous version regarding limits
 *
 * Version 0.7d - broken things
 *
 *	- fixed hotplug up threshold tuneables to be able again to disable cores manually via sysfs by setting them to 0
 *	- fixed the problem caused by a 'wrong' tuneable apply order of non sticking values in hotplug down threshold tuneables when
 *	  hotplug up values are lower than down values during apply.
 *	  NOTE: due to this change right after start of the governor the full validation of given values to these tuneables is disabled till
 *	  all the tuneables were set for the first time. so if you set them for example with an init.d script or let them set automatically
 *	  with any tuning app be aware that there are illogical value combinations possible then which might not work properly!
 *	  simply be sure that all up values are higher than the down values and vice versa. after first set full validation checks are enabled
 *	  again and setting of values manually will be checked again.
 *	- fixed a typo in hotplug threshold tuneable macros (would have been only a issue in 8-core mode)
 *	- fixed unwanted disabling of cores when setting hotplug threshold tuneables to lowest or highest possible value
 *	  which would be a load of 100%/1% in up/down_hotplug_threshold and/or scaling frequency min/max in up/down_hotplug_threshold_freq
 *
 * Version 0.8 - cool down baby!
 *
 *	- indroduced scaling block cycles (in normal and legacy mode) to reduce unwanted jumps to higher frequencies (how high depends on settings)
 *	  when a load comes up just for a short peroid of time or is hitting the scaling up threshold more often because it is within some higher
 *	  to mid load range. reducing these jumps lowers CPU temperature in general and may also save some juice. so with this function u can influence
 *	  the little bit odd scaling behaving when you are running apps like for example games which are constantly 'holding' system load in
 *	  some range and the freq is scaled up too high for that range. in fact it just looks like so, monitoring apps are mostly too slow to catch load in
 *	  realtime so you are watching almost only 'the past'. so actually it's not really odd it's more like: an app is stressing the system and holding
 *	  it on a higher load level, due to this load level scaling up threshold is more often reached (even if monitoring shows a lower load
 *	  than up_threshold!) the governor scales up very fast (usual zzmoove behaving) and almost never scales down again (even if monitoring shows
 *	  a lower load than down_threshold!). now in patricular these scaling block cycles are throttling up scaling by just skipping it
 *	  for the amount of cycles that you have set up and after that are making a forced scale down in addition for the same amount of cycles. this
 *	  should bring us down to a 'appropriate' frequency when load is hanging around near up threshold.
 *	- indroduced (D)ynamic (S)ampling (R)ate - thx to hellsgod for having the same idea at the same time and pointing me to an example. even though
 *	  at the end i did 'my way' :) DSR switches between two sampling rates depending on the given load threshold from an 'idle' to a 'normal' one.
 *	- added read only tuneable 'sampling_rate_current' for DSR to show current SR and internally use this sampling rate instead of automatically
 *	  changing 'sampling_rate' tuneable in DSR. this keeps things more compatible and avoids problems when governor tuneables are set with tuning
 *	  apps which are saving actual shown values.
 *	- changed setting of sampling rate in governor from 'sampling_rate' to 'sampling_rate_current' and the value at suspend to 'sampling_rate_idle'
 *	  value instead of using current active sampling rate to avoid accidentally setting of 'normal operation' sampling rate in sampling rate idle mode
 *	  which has usually a much lower value
 *	- indroduced build-in profiles in seperate header file (credits to Yank555 for idea and prototype header file)
 *	  you can switch between multible build in profiles by just piping a number into the tuneable 'profile_number'. all tuneable values of the set
 *	  profile will be applied on-the-fly then. if a profile is active and you change any tuneable value from it to a custom one the profile will
 *	  switch to '0' in 'profile_number' and 'custom' in 'profile' for information. with this profiles support developers can simplify their governor
 *	  settings handling by adding all their desired and well proven governor settings into the governor itself instead of having to fiddle around with
 *	  init.d scripts or tuning apps. NOTE: this is just an optional feature, you can still set all tuneables as usual just pay attention that the
 *	  tuneable 'profile_number' is set to '0' then to avoid overwriting values by any build-in profile! for further details about profiles and
 *	  adding own ones check provided 'cpufreq_zzmoove_profiles.h' file.
 *	- added 'profiles_version' tuneable to be able to show seperate profiles header file version
 *	- added enabling of offline cores on governor exit to avoid cores 'stucking' in offline state when switching to a non-hotplug-able governor
 *	  and by the way reduced reduntant code by using an inline function for switching cores on and using the better 'sheduled_work_on-way'
 *	  at all needed places in the code for that purpose
 *	- moved some code parts to legacy mode macro which has only relevance when including the legacy mode in the governor and in addition
 *	  excluded it during runtime if legacy mode is disabled.
 *	- improved freq limit handling in tuneables and in dbs_check_cpu function
 *	- changed value restriction from '11' to '1' in hotplug down threshold and grad up tuneables as this restriction is only nessesary in
 *	  scaling down tuneable
 *	- added missing fast scaling down/normal scaling up mode to fast scaling functionality (value range 9-12 and only available in non-legacy mode
 *	  thx OldBoy.Gr for pointing me to that missing mode!)
 *	- added auto fast scaling aka 'insane' scaling mode to fast scaling functionality - lucky number 13 enables this mode in fast_scaling tuneable
 *	  NOTE: a really funny mode, try it :) but keep in mind setting this in combination with a set freq limit (at sleep or awake)would not make much
 *	  sense as there is not enough frequency range available to jump around then.
 *	- back from the precautious 'mutex_try_lock' to normal 'mutex_lock' in governor 'LIMIT' case -> this should be save again,
 *	  no deadlocks expected any more since hotplug logic has significantly changed in zzmoove version 0.6
 *	- removed also the no longer required and precautious skipping of hotplugging and dbs_check_cpu on multiple places in code and removed
 *	  the mutex locks at governor stop and early suspend/late resume
 *	- added hotlug freq thresholds to legacy scaling mode (same usage as in normal scaling mode)
 *	- seperated hotplug down and up block cycles to be more flexible. this replaces 'hotplug_block_cycles' with 'hotplug_block_up_cycles' tuneable
 *	  and adds one new tunable 'hotplug_block_down_cycles'. functionality is the same as before but u can now differentiate the up and down value.
 *	- added 'early demand sleep' combined with automatic fast scaling (fixed to scaling up mode 2) and if not set already automatic
 *	  (depending on load) switching of sampling rate sleep multiplier to a fixed minimum possible multiplier of 2.
 *	  this should avoid mostly audio or general device connection problems with 'resource hungrier' apps like some music players,
 *	  skype, navigation apps etc. and/or in combination with using bluetooth equipment during screen is off.
 *	  NOTE: this overwrites a possible fast 'scaling_sleep' setting so use either this or 'fast_scaling_sleep'
 *	- added some missing governor tunebable default value definitions
 *	- removed tuneable apply order exception and removed analog value checks in hotplug threshold and hotplug frequency tuneables to avoid
 *	  tuneable values not changing issues. NOTE: keep in mind that all 'down' values should be lower then the analog 'up' values and vice versa!
 *	- removed 200 mhz up hotplugging restriction, so up hotplugging starts at 200 mhz now
 *	- removed some unnecessary macros in scaling logic
 *	- added maximum fast scaling and frequency boost to late resume to react wakeup lags
 *	- merged some improvements from ktoonservativeq governor version for the SGS4 (credits to ktoonsez)
 *	  changes from here: https://github.com/ktoonsez/KT-SGS4/commits/aosp4.4/drivers/cpufreq/cpufreq_ktoonservativeq.c
 *	  Use dedicated high-priority workqueues
 *	  Use NEW high-priority workqueue for hotplugging
 *	  Improved hotplugging in general by reducing calls to plugging functions if they are currently running,
 *	  by reducing calls to external function in up plugging and by changing the down plug loop to an optimized one
 *	- added hotplug boost switch to early demand functionality and up hotplugging function
 *	- added 'hotplug_idle_freq' tuneable to be able to adjust at which frequency idle should begin
 *	- transfered code for frequency table order detection and limit optimisation into a inline function
 *	  and use this function in START,LIMIT case and early suspend/late resume instead of using redundant code
 *	- execute table order detection and freq limit optimization calculations at 'START' and 'LIMIT' case to avoid possible wrong setting of freq
 *	  max after governor start (currently set max frequency value was sometimes not applied) and a wrong soft limit optimization setting
 *	  after undercutting the soft limit with a lower hard limit value
 *	- minor optimisation in hotplug, hotplug block and in all freq search logic parts
 *	- added debugging sysfs interface (can be included/excluded using #define ZZMOOVE_DEBUG) - credits to Yank555!
 *	- added some missing annotation as a prepareation and mainly to avoid some errors when compiling zzmoove in combination with 3.4+ kernel sources
 *	- fixed hotplugging issues when cpufreq limit was set under one or more hotplugging frequency thresholds
 *	  NOTE: now hotplugging frequency thresholds will be completely disabled and a fall back to normal load thresholds will happen
 *	  if the maximal possible frequency will undercut any frequency thresholds
 *	- fixed stopping of up scaling at 100% load when up threshold tuneable is set to the max value of 100
 *	- fixed smooth up not active at 100% load when smooth up tuneable is set to the max value of 100
 *	- fixed many code style and dokumentation issues and made a massive code re-arrangement
 *
 *	  for this functions following new tuneables were indroduced:
 *
 *	  early_demand_sleep		-> same function as early demand on awake but in addition combined with fast scaling and sampling rate
 *					   switch and only active at sleep. (possible values 0 disable or 1 enable, default is 1)
 *	  grad_up_threshold_sleep	-> 2 way functionality: early demand sleep grad up (load gradient) threshold and at the same time load threshold
 *					   for switching internally (tuneables are staying at set values!) sampling_rate_sleep_multiplier to 2 and
 *					   fast_scaling to 2 (possible values from 1 to 100, default is 35)
 *	  hotplug_block_up_cycles	-> (replaces hotplug_block_cycles) slow down up hotplugging by waiting a given amount of cycles before plugging.
 *					   possible values 0 disbale, any values above 0 (default is 0)
 *	  hotplug_block_down_cycles	-> (replaces hotplug_block_cycles) slow down down hotplugging by waiting a given amount of cycles before plugging.
 *					   possible values 0 disbale, any values above 0 (default is 0)
 *	  hotplug_idle_freq		-> freq at which the idle should be active (possible values 0 disable and any possible scaling freq, default is 0)
 *	  sampling_rate_current		-> read only and shows currently active sampling rate
 *	  sampling_rate_idle		-> sampling rate which should be used at 'idle times'
 *					   (possible values are any sampling rate > 'min_sampling_rate', 0 to disable whole function, default is 0)
 *	  sampling_rate_idle_delay	-> delay in cycles for switching from idle to normal sampling rate and vice versa
 *					   (possible values are any value and 0 to disable delay, default is 0)
 *	  sampling_rate_idle_threshold	-> threshold under which idle sampling rate should be active
 *					   (possible values 1 to 100, 0 to disable function, default is 0)
 *	  scaling_block_cycles		-> amount of gradients which should be counted (if block threshold is set) and at the same time up scaling should be
 *					   blocked and after that a forced down scaling should happen (possible values are any value, 0 to disable that
 *					   function, default is 0)
 *	  scaling_bock_freq		-> frequency at and above the blocking should be active
 *					   (possible values are any possible scaling freq, 0 to enable blocking permanently at every frequency, default is 0)
 *	  scaling_block_threshold	-> gradient (min value) of load in both directions (up/down) to count-up cycles
 *					   (possible value are 1 to 100, 0 to disable gradient counting)
 *	  scaling_block_force_down	-> multiplicator for the maximal amount of forced down scaling cycles (force down cycles = block_cycles * force_down)
 *					   therefore the forced down scaling duration (possible value are 2 to any value, 0 to disable forced down scaling
 *					   and use only scaling up blocks)
 *	  profile			-> read only and shows name of currently active profile ('none' = no profile, 'custom' = a profile value has changed)
 *	  profile_number		-> switches profile (possible value depends on amount of profiles in cpufreq_zzmoove_profiles.h file,
 *					   please check this file for futher details!) 0 no profile set = tuneable mode, default 0)
 *	  version_profiles		-> read only and shows version of profile header file
 *
 *	  if ZZMOOVE_DEBUG is defined:
 *	  debug				-> read only and shows various usefull debugging infos
 *
 * Version 0.9 alpha1 (Yank555.lu)
 *
 *	- splitted fast_scaling into two separate tunables fast_scaling_up and fast_scaling_down so each can be set individually to 0-4
 *	  (skip 0-4 frequency steps) or 5 to use autoscaling.
 *	- splitted fast_scaling_sleep into two separate tunables fast_scaling_sleep_up and fast_scaling_sleep_down so each can be set individually to 0-4
 *	  (skip 0-4 frequency steps) or 5 to use autoscaling.
 *	- removed legacy mode (necessary to be able to split fast_scaling tunable)
 *	- removed LCD frequency DFS
 *
 * Version 0.9 alpha2
 *
 *	- added auto fast scaling step tuneables:
 *	  afs_threshold1 for step one (range from 1 to 100)
 *	  afs_threshold2 for step two (range from 1 to 100)
 *	  afs_threshold3 for step three (range from 1 to 100)
 *	  afs_threshold4 for step four (range from 1 to 100)
 *
 * Version 0.9 beta1
 *
 *	- bump version to beta for public
 *	- added/corrected version informations and removed obsolete ones
 *
 * Version 0.9 beta2
 *
 *	- support for setting a default settings profile at governor start without the need of using the tuneable 'profile_number'
 *	  a default profile can be set with the already available macro 'DEF_PROFILE_NUMBER' check zzmoove_profiles.h for details about
 *	  which profile numbers are possible. this functionality was only half baken in previous versions, now any given profile will be really
 *	  applied when the governor starts. the value '0' (=profile name 'none') in 'DEF_PROFILE_NUMBER' disables this profile hardcoding and
 *	  that's also the default in the source. u still can (with or without enabling a default profile in the macro) as usual use the tuneable
 *	  'profile_number' to switch to a desired profile via sysfs at any later time after governor has started
 *	- added 'blocking' of sysfs in all tuneables during apply of a settings profile to avoid a possible and unwanted overwriting/double
 *	  setting of tuneables mostly in combination with tuning apps where the tuneable apply order isn't influenceable
 *	- added tuneable 'profile_list' for printing out a list of all profiles which are available in the profile header file
 *	- fixed non setting of 'scaling_block_force_down' tuneable when applying profiles
 *	- some documentation added and a little bit of source cleaning
 *
 * Version 0.9 beta3
 *
 *	merged some changes originated by ffolkes (all credits and thx to him)
 *	(source https://github.com/ffolkes/android_kernel_samsung_smdk4412/commits/534ec13388175eb77def021e92b0a44e1e3190d5/drivers/cpufreq/cpufreq_zzmoove.c)
 *
 *	- reordered sysfs attributes
 *	  description by ffolkes: some apps set tuneables by the order in which they are listed in the filesystem. this causes problems when one tuneable
 *	  needs another set first in order to correctly validate. (e.g. you cannot set down_threshold properly until you have first set up_threshold)
 *	- added 'fast down' functionality (based on commits to pegasusq in perseus by andreilux and extended for this version by ZaneZam)
 *	  description by ffolkes: fastdown dynamically applies a (presumably) higher up_threshold and down_threshold after a frequency threshold has been
 *	  reached. the goal is to encourage less time spent on the highest frequencies
 *	- added 'hotplug engage' functionality
 *	  description by ffolkes: when set >0, will not bring any cores online until this frequency is met or exceeded
 *	  goal: reduce unnecessary cores online at low loads
 *	- added 'scaling responsiveness' functioniality
 *	  description by ffolkes: similar to 'frequency for responsiveness' in other governors
 *	  defines a frequency below which we use a different up_threshold to help eliminate lag when starting tasks
 *	- instead of failing when set too high in down_threshold tuneables set the value to the highest it can safely go
 *	- increased possible sampling rate sleep multiplier value to a max of 8 (ZaneZam)
 *	- added missing error handling to some tuneables (ZaneZam)
 *	- fixed non setting of 'hotplug_sleep' tuneable when applying profiles (ZaneZam)
 *	- added up/down threshold to debug tuneable (ZaneZam)
 *	- some code style and comment changes/fixes (ZaneZam)
 *
 *	  for this functions following new tuneables were introduced:
 *
 *	  scaling_fastdown_freq			-> will be enabled once this frequency has been met or exceeded
 *						   (0 to disable, all possible system frequencies, default is 0)
 *	  scaling_fastdown_up_threshold		-> once the above frequency threshold has been met, this will become the new up_threshold until we fall
 *						   below the scaling_fastdown_freq again. (range from over fastdown_down_threshold to 100, default is 95)
 *	  scaling_fastdown_down_threshold	-> once the above frequency threshold has been met, this will become the new down_threshold until we fall
 *						   below the scaling_fastdown_freq again. (range from 11 to under fastdown_up_threshold, default is 90)
 *	  scaling_responsiveness_freq		-> will be enabled once this frequency has been met or exceeded
 *						   (0 to disable, all possible system frequencies, default is 0)
 *	  scaling_responsiveness_up_threshold	-> the up_threshold that will take effect if scaling_responsiveness_freq is set
 *						   (range from 11 to 100, default is 30)
 *	  hotplug_engage_freq			-> will not bring any cores online until this frequency is met or exceeded
 *						   (0 to disable, all possible system frequencies, default is 0)
 *
 * Version 0.9 beta4
 *
 *	- removed 'freq_step' functionality as it never had any function in this governor! it was a left over from mialwes 'smoove' governor and also
 *	  had no function in his governor back then!! so yeah all the 'feelings' about it's influence were placebo! :)
 *	- introduced 'proportional scaling' for more 'connectivity' to current load, this should give more 'balanced' frequencies
 *	  in general. when enabled all targeted frequencies in scaling logic will be compared with the ones from system table method and at the end
 *	  the lowest of them both will be used. so all used scaling frequencies will be 'tentential' lower in both directions
 *	- added support for exynos4 CPU temperature reading (patches available in zzmoove repositories: https://github.com/zanezam)
 *	  this must be enabled via 'CONFIG_EXYNOS4_EXPORT_TEMP=y' in the config of a kernel which has exynos4 CPU temperature export implementation
 *	  included. the default temp polling interval is 1000 ms and can be set with DEF_TMU_READ_DELAY. however the TMU driver has it's own polling
 *	  interval which is 10 seconds, so leaving it at the default value of 1 second is recommended temperature reading will only be enabled if the
 *	  tuneable 'scaling_block_temp' is set and will be disabled whenever early suspend is entered
 *	- if exynos4 CPU temperature reading is enabled in the code use current CPU temperature in scaling block functionality to be able to 'hold' the cpu
 *	  temperatue to the given one in 'scaling_block_temp'. this function is used in combination with the already existent tuneable 'scaling_block_freq'
 *	  so u have to set both to enable it. the possible temperature range is 30°C to 80°C (lower temps are making no sense and higher temps would reach
 *	  into exynos4 TMU driver trottling range)
 *	- if exynos4 CPU temperature reading is enabled added current CPU temperature to debug info tuneable
 *	- added auto adjustment of all available frequency thresholds when scaling max limit has changed
 *	- improved sampling rate idle switching by making it scaling thresholds independent
 *	- again some code style and comment changes/fixes
 *
 *	  for this functions following new tuneables were introduced:
 *
 *	  scaling_proportional			-> if enabled load-proportional frequencies will be calculated in parallel and will be used in decision for
 *						   next freq step. after a comparison between normal system table step and proportional step the lowest of
 *						   these two frequencies will be used for next freq step in both directions.
 *						   (0 to disable, any value above 0 to enable)
 *	  scaling_block_temp			-> CPU temperature threshold from where governors freq 'holding' should start. if the given temperature
 *	  (if CPU temp reading is enabled)	   is reached the frequency used in tuneable 'scaling_block_freq' will be forced targeted and scaling
 *						   stays on this freq till the temperature is under the threshold again. at the same time hotplugging up
 *						   work is blocked so in this throttling phase offline cores are staying offline even if the hotplug up
 *						   thresholds are reached
 *						   (0 to disable, values between 30 and 80 in °C)
 *	  auto_adjust_freq_thresholds		-> if enabled all freq thresholds used by the governor will be adjusted accordingly to the new scaling
 *						   max policy. in particular the thresholds will be increased/decreased by the actual changed max freq step
 *						   if that change will undercut/exceed the actual min/max freq policy it will stop at the max possible
 *						   frequency step before undercutting/exceeding min/max freq policy
 *						   (0 to disable, any value above 0 to enable)
 *
 * Version 1.0 beta1
 *
 *	- bump version to 1.0 beta1 because of brought forward plan 'outbreak'
 *	- reworked scaling logic:
 *	  removed unessesary calls of external cpufreq function and use a static variable instead to hold the system freq table during runtime
 *	  fixed frequency stuck on max hard and soft frequency limit (under some circumstances freq was out of scope for the main search loop)
 *	  and added precautions to avoid problems when for what ever reason the freq table is 'messed' or even not available for the governor
 *	  fixed not properly working scaling with descend ordered frequency table like it is for example on qualcomm platform
 *	  added additional propotional scaling mode (mode '1' as usual decide and use the lowest freq, new mode '2' use only propotional
 *	  frequencies like ondemand governor does - switchable as before in 'scaling_proportional' tuneable)
 *	- use static frequency table variable in all frequency checks agains system frequency table in the governor
 *	- fixed 'update_ts_time_stat idle accounting' (kernel patch for kernel version 3.0.x needed, example available in github zzmoove repository)
 *	- fixed non setting of scaling down threshold tuneables under some circumstances (issue on kernel 3.4 when running multible zzmoove instances)
 *	- changed some variable names in scaling range evaluation and debugging tuneable
 *	- added compatibility for kernel version 3.4 (or higher, but only tested on 3.4.0 yet)
 *	- added compatibility for cpufreq implementation used since kernel version 3.10 (NOTE: for backports u can use the macro CPU_IDLE_TIME_IN_CPUFREQ)
 *	- added support for powersuspend (used on some platforms since kernel version 3.4)
 *	- added support for opo specific 'backlight ext control' (kernel patch for opo bacon devices needed, example available in github zzmoove repository)
 *	- added macros to exclude hotplugging functionality (default in this version is disabled=commented for opo devices because of mpdecision
 *	  in general and unmotivated kernel crashes when using it without mpdecision) in addition disable hopplugging per default if hotplugging
 *	  code is used to avoid accetential usage of mpdecision and buildin governor hotplugging at the same time
 *
 * Version 1.0 beta2 (bugfix!)
 *
 *	- avoid kernel crash (usually a oops in smp.c) by checking if a core is online before putting work on it: this problem appeared on opo qualcomm
 *	  platform with proprietary mpdecision hotplugging service. assumption is that there is a delay between initiating hotplugging events from 'userland'
 *	  and gathering core state info in 'kernel land' so under some rare circumestances the governor doesn't 'know about' a changed core state and tries to
 *	  put work on a meanwhile offline core or that hotplug event happend during putting work on a core in the governor.
 *
 * Version 1.0 beta3 (bugfix!)
 *
 *	- changed back canceling of dbs work to syncron instead of asyncron in dbs_timer_exit function to avoid random kernel chrashes (again oops in smp.c)
 *	  when using this governor with the cpufreq implementation of kernel versions 3.10+. problem was initiated by governor restarts during hotplugging
 *	- as an additional precaution check if a core is online before doing critical stuff in dbs_check_cpu main function (might be removeable at a later
 *	  time, more analyses/tests will show)
 *
 * Version 1.0 beta4 (sync)
 *
 *	- removed all previous kernel crash fix attempts and precautions as only the real fix of canceling dbs work syncron introduced in beta3 is needed.
 *
 * ---------------------------------------------------------------------------------------------------------------------------------------------------------
 * -                                                                                                                                                       -
 * ---------------------------------------------------------------------------------------------------------------------------------------------------------
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
#include <linux/version.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_POWERSUSPEND
#include <linux/powersuspend.h>
#endif
#ifdef CONFIG_BACKLIGHT_EXT_CONTROL
#include <linux/backlight_ext_control.h>
#endif
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
#include <linux/exynos4_export_temp.h>
#endif

// Yank: enable/disable sysfs interface to display current zzmoove version
#define ZZMOOVE_VERSION "1.0 beta4"

// ZZ: support for 2,4 or 8 cores (this will enable/disable hotplug threshold tuneables)
#define MAX_CORES					(4)

// ZZ: enable/disable hotplug support
// #define ENABLE_HOTPLUGGING

// ZZ: enable for sources with backported cpufreq implementation of 3.10 kernel
// #define CPU_IDLE_TIME_IN_CPUFREQ

// ZZ: include profiles header file and set name for 'custom' profile (informational for a changed profile value)
#include "cpufreq_zzmoove_profiles.h"
#define DEF_PROFILE_NUMBER				(0)	// ZZ: default profile number (profile = 0 = 'none' = tuneable mode)
static char custom_profile[20] = "custom";			// ZZ: name to show in sysfs if any profile value has changed

// Yank: enable/disable debugging code
// #define ZZMOOVE_DEBUG

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate samplingrate. For CPUs
 * with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work. All times here are in uS.
 */
#define TRANSITION_LATENCY_LIMIT	    (10 * 1000 * 1000)	// ZZ: default transition latency limit
#define LATENCY_MULTIPLIER				(1000)	// ZZ: default latency multiplier
#define MIN_LATENCY_MULTIPLIER				(100)	// ZZ: default min latency multiplier
#define MIN_SAMPLING_RATE_RATIO				(2)	// ZZ: default min sampling rate ratio

// ZZ: general tuneable defaults
#define DEF_FREQUENCY_UP_THRESHOLD			(70)	// ZZ: default regular scaling up threshold
#ifdef ENABLE_HOTPLUGGING
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG		(68)	// ZZ: default hotplug up threshold for all cpus (cpu0 stays allways on)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ		(0)	// Yank: default hotplug up threshold frequency for all cpus (0 = disabled)
#endif
#define DEF_SMOOTH_UP					(75)	// ZZ: default cpu load trigger for 'boosting' scaling frequency
#define DEF_FREQUENCY_DOWN_THRESHOLD			(52)	// ZZ: default regular scaling down threshold
#ifdef ENABLE_HOTPLUGGING
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG		(55)	// ZZ: default hotplug down threshold for all cpus (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ	(0)	// Yank: default hotplug down threshold frequency for all cpus (0 = disabled)
#endif
#define DEF_IGNORE_NICE					(0)	// ZZ: default ignore nice load
#define DEF_AUTO_ADJUST_FREQ				(0)	// ZZ: default auto adjust frequency thresholds

// ZZ: hotplug-switch, -block, -idle, -limit and scaling-block, -fastdown, -responiveness, -proportional tuneable defaults
#ifdef ENABLE_HOTPLUGGING
#define DEF_DISABLE_HOTPLUG				(1)	// ZZ: default hotplug switch
#define DEF_HOTPLUG_BLOCK_UP_CYCLES			(0)	// ZZ: default hotplug up block cycles
#define DEF_HOTPLUG_BLOCK_DOWN_CYCLES			(0)	// ZZ: default hotplug down block cycles
#define DEF_HOTPLUG_IDLE_THRESHOLD			(0)	// ZZ: default hotplug idle threshold
#define DEF_HOTPLUG_IDLE_FREQ				(0)	// ZZ: default hotplug idle freq
#define DEF_HOTPLUG_ENGAGE_FREQ				(0)	// ZZ: default hotplug engage freq. the frequency below which we run on only one core (0 = disabled) (ffolkes)
#endif
#define DEF_SCALING_BLOCK_THRESHOLD			(0)	// ZZ: default scaling block threshold
#define DEF_SCALING_BLOCK_CYCLES			(0)	// ZZ: default scaling block cycles
#define DEF_SCALING_BLOCK_FREQ				(0)	// ZZ: default scaling block freq
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
#define DEF_TMU_READ_DELAY				(1000)	// ZZ: delay for cpu temperature reading in ms (tmu driver polling intervall is 10 sec)
#define DEF_SCALING_BLOCK_TEMP				(0)	// ZZ: default cpu temperature threshold in °C
#endif
#define DEF_SCALING_BLOCK_FORCE_DOWN			(2)	// ZZ: default scaling block force down
#define DEF_SCALING_FASTDOWN_FREQ			(0)	// ZZ: default scaling fastdown freq. the frequency beyond which we apply a different up_threshold (ffolkes)
#define DEF_SCALING_FASTDOWN_UP_THRESHOLD		(95)	// ZZ: default scaling fastdown up threshold. the up threshold when scaling fastdown freq has been exceeded (ffolkes)
#define DEF_SCALING_FASTDOWN_DOWN_THRESHOLD		(90)	// ZZ: default scaling fastdown up threshold. the down threshold when scaling fastdown freq has been exceeded (ffolkes)
#define DEF_SCALING_RESPONSIVENESS_FREQ			(0)	// ZZ: default frequency below which we use a lower up threshold (ffolkes)
#define DEF_SCALING_RESPONSIVENESS_UP_THRESHOLD		(30)	// ZZ: default up threshold we use when below scaling responsiveness freq (ffolkes)
#define DEF_SCALING_PROPORTIONAL			(0)	// ZZ: default proportional scaling

// ZZ: sampling rate idle and sampling down momentum tuneable defaults
#define DEF_SAMPLING_RATE_IDLE_THRESHOLD		(0)	// ZZ: default sampling rate idle threshold
#define DEF_SAMPLING_RATE_IDLE				(180000)// ZZ: default sampling rate idle (must not be 0!)
#define DEF_SAMPLING_RATE_IDLE_DELAY			(0)	// ZZ: default sampling rate idle delay
#define DEF_SAMPLING_DOWN_FACTOR			(1)	// ZZ: default sampling down factor (stratosk default = 4) here disabled by default
#define MAX_SAMPLING_DOWN_FACTOR			(100000)// ZZ: changed from 10 to 100000 for sampling down momentum implementation
#define DEF_SAMPLING_DOWN_MOMENTUM			(0)	// ZZ: default sampling down momentum, here disabled by default
#define DEF_SAMPLING_DOWN_MAX_MOMENTUM			(0)	// ZZ: default sampling down max momentum stratosk default=16, here disabled by default
#define DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY		(50)	// ZZ: default sampling down momentum sensitivity
#define MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY		(1000)	// ZZ: default maximum for sampling down momentum sensitivity

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
// ZZ: tuneable defaults for early suspend
#define MAX_SAMPLING_RATE_SLEEP_MULTIPLIER		(8)	// ZZ: default maximum for sampling rate sleep multiplier
#define DEF_SAMPLING_RATE_SLEEP_MULTIPLIER		(2)	// ZZ: default sampling rate sleep multiplier
#define DEF_UP_THRESHOLD_SLEEP				(90)	// ZZ: default up threshold sleep
#define DEF_DOWN_THRESHOLD_SLEEP			(44)	// ZZ: default down threshold sleep
#define DEF_SMOOTH_UP_SLEEP				(75)	// ZZ: default smooth up sleep
#define DEF_EARLY_DEMAND_SLEEP				(1)	// ZZ: default early demand sleep
#define DEF_GRAD_UP_THRESHOLD_SLEEP			(30)	// ZZ: default grad up sleep
#define DEF_FAST_SCALING_SLEEP_UP			(0)	// Yank: default fast scaling sleep for upscaling
#define DEF_FAST_SCALING_SLEEP_DOWN			(0)	// Yank: default fast scaling sleep for downscaling
#define DEF_FREQ_LIMIT_SLEEP				(0)	// ZZ: default freq limit sleep
#ifdef ENABLE_HOTPLUGGING
#define DEF_DISABLE_HOTPLUG_SLEEP			(1)	// ZZ: default hotplug sleep switch
#endif
#endif

/*
 * ZZ: Hotplug Sleep: 0 do not touch hotplug settings at suspend, so all cores will stay online
 * the value is equivalent to the amount of cores which should be online at suspend
 */
#ifdef ENABLE_HOTPLUGGING
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
#define DEF_HOTPLUG_SLEEP				(0)	// ZZ: default hotplug sleep
#endif
#endif

// ZZ: tuneable defaults for Early Demand
#define DEF_GRAD_UP_THRESHOLD				(25)	// ZZ: default grad up threshold
#define DEF_EARLY_DEMAND				(0)	// ZZ: default early demand, default off

/*
 * ZZ: Frequency Limit: 0 do not limit frequency and use the full range up to policy->max limit
 * values policy->min to policy->max in khz
 */
#define DEF_FREQ_LIMIT					(0)	// ZZ: default freq limit

/*
 * ZZ: Fast Scaling: 0 do not activate fast scaling function
 * values 1-4 to enable fast scaling and 5 for auto fast scaling (insane scaling)
 */
#define DEF_FAST_SCALING_UP				(0)	// Yank: default fast scaling for upscaling
#define DEF_FAST_SCALING_DOWN				(0)	// Yank: default fast scaling for downscaling
#define DEF_AFS_THRESHOLD1				(25)	// ZZ: default auto fast scaling step one
#define DEF_AFS_THRESHOLD2				(50)	// ZZ: default auto fast scaling step two
#define DEF_AFS_THRESHOLD3				(75)	// ZZ: default auto fast scaling step three
#define DEF_AFS_THRESHOLD4				(90)	// ZZ: default auto fast scaling step four

// ZZ: Sampling Down Momentum variables
static unsigned int min_sampling_rate;				// ZZ: minimal possible sampling rate
static unsigned int orig_sampling_down_factor;			// ZZ: for saving previously set sampling down factor
static unsigned int orig_sampling_down_max_mom;			// ZZ: for saving previously set smapling down max momentum

// ZZ: search limit for frequencies in scaling table, variables for scaling modes and state flag for suspend detection
static struct cpufreq_frequency_table *system_freq_table;	// ZZ: static system frequency table
static int scaling_mode_up;					// ZZ: fast scaling up mode holding up value during runtime
static int scaling_mode_down;					// ZZ: fast scaling down mode holding down value during runtime
static bool freq_table_desc = false;				// ZZ: true for descending order, false for ascending order
static int freq_init_count = 0;					// ZZ: flag for executing freq table order and limit optimization code at gov start
static unsigned int max_scaling_freq_soft = 0;			// ZZ: init value for 'soft' scaling limit, 0 = full range
static unsigned int max_scaling_freq_hard = 0;			// ZZ: init value for 'hard' scaling limit, 0 = full range
static unsigned int system_table_end = CPUFREQ_TABLE_END;	// ZZ: system freq table end for order detection, table size calculation and freq validations
static unsigned int limit_table_end = CPUFREQ_TABLE_END;	// ZZ: initial (full range) search end limit for frequency table in descending ordered table
static unsigned int limit_table_start = 0;			// ZZ: search start limit for frequency table in ascending order
static unsigned int freq_table_size = 0;			// Yank: upper index limit of frequency table
static unsigned int min_scaling_freq = 0;			// Yank: lowest frequency index in global frequency table
static bool suspend_flag = false;				// ZZ: flag for suspend status, true = in early suspend

// ZZ: hotplug-, scaling-block, scaling fastdown vars and sampling rate idle counters. flags for scaling, setting profile, cpu temp reading and hotplugging
#ifdef ENABLE_HOTPLUGGING
static int possible_cpus = 0;					// ZZ: for holding the maximal amount of cores for hotplugging
static unsigned int hplg_down_block_cycles = 0;			// ZZ: delay cycles counter for hotplug down blocking
static unsigned int hplg_up_block_cycles = 0;			// ZZ: delay cycles counter for hotplug up blocking
#endif
static unsigned int scaling_block_cycles_count = 0;		// ZZ: scaling block cycles counter
static unsigned int sampling_rate_step_up_delay = 0;		// ZZ: sampling rate idle up delay cycles
static unsigned int sampling_rate_step_down_delay = 0;		// ZZ: sampling rate idle down delay cycles
static unsigned int scaling_up_threshold = 0;			// ZZ: scaling up threshold for fastdown/responsiveness functionality
static unsigned int scaling_down_threshold = 0;			// ZZ: scaling down threshold for fastdown functionality
#ifdef ENABLE_HOTPLUGGING
static bool hotplug_idle_flag = false;				// ZZ: flag for hotplug idle mode
static bool max_freq_too_low = false;				// ZZ: flag for overwriting freq thresholds if max freq is lower than thresholds
static bool __refdata enable_cores = false;			// ZZ: flag for enabling offline cores at governor stop and late resume
static bool hotplug_up_in_progress;				// ZZ: flag for hotplug up function call - block if hotplugging is in progress
static bool hotplug_down_in_progress;				// ZZ: flag for hotplug down function call - block if hotplugging is in progress
static bool boost_hotplug = false;				// ZZ: early demand boost hotplug flag
#endif
static bool boost_freq = false;					// ZZ: early demand boost freq flag
static bool force_down_scaling = false;				// ZZ: force down scaling flag
static bool cancel_up_scaling = false;				// ZZ: cancel up scaling flag
static bool set_profile_active = false;				// ZZ: flag to avoid changing of any tuneables during profile apply
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
#ifdef ENABLE_HOTPLUGGING
static bool hotplug_up_temp_block;				// ZZ: flag for blocking up hotplug work during temp freq blocking
#endif
static bool cancel_temp_reading = false;			// ZZ: flag for starting temp reading work
static bool temp_reading_started = false;			// ZZ: flag for canceling temp reading work

// ZZ: CPU temp reading work
static void tmu_read_temperature(struct work_struct * tmu_read_work);	// ZZ: prepare temp reading work
static DECLARE_DELAYED_WORK(tmu_read_work, tmu_read_temperature);	// ZZ: declare delayed work for temp reading
static unsigned int cpu_temp;						// ZZ: static var for holding current cpu temp
#endif

// ZZ: current load & frequency for hotplugging work and scaling. max/min frequency for proportional scaling and auto freq threshold adjustment
static unsigned int cur_load = 0;				// ZZ: current load for hotplugging functions
static unsigned int cur_freq = 0;				// ZZ: current frequency for hotplugging functions
static unsigned int pol_max = 0;				// ZZ: current max freq for proportional scaling and auto adjustment of freq thresholds
static unsigned int old_pol_max = 0;				// ZZ: previous max freq for auto adjustment of freq thresholds
static unsigned int pol_min = 0;				// ZZ: current min freq for auto adjustment of freq thresholds
static unsigned int pol_step = 0;				// ZZ: policy change step for auto adjustment of freq thresholds

// ZZ: temp variables and flags to hold offset values for auto adjustment of freq thresholds
#ifdef ENABLE_HOTPLUGGING
static unsigned int temp_hotplug_engage_freq = 0;
static bool temp_hotplug_engage_freq_flag = false;
static unsigned int temp_hotplug_idle_freq = 0;
static bool temp_hotplug_idle_freq_flag = false;
#endif
static unsigned int temp_scaling_block_freq = 0;
static bool temp_scaling_block_freq_flag = false;
static unsigned int temp_scaling_fastdown_freq = 0;
static bool temp_scaling_fastdown_freq_flag = false;
static unsigned int temp_scaling_responsiveness_freq = 0;
static bool temp_scaling_responsiveness_freq_flag = false;
#ifdef ENABLE_HOTPLUGGING
static unsigned int temp_up_threshold_hotplug_freq1 = 0;
static bool temp_up_threshold_hotplug_freq1_flag = false;
static unsigned int temp_down_threshold_hotplug_freq1 = 0;
static bool temp_down_threshold_hotplug_freq1_flag = false;
#if (MAX_CORES == 4 || MAX_CORES == 8)
static unsigned int temp_up_threshold_hotplug_freq2 = 0;
static bool temp_up_threshold_hotplug_freq2_flag = false;
static unsigned int temp_up_threshold_hotplug_freq3 = 0;
static bool temp_up_threshold_hotplug_freq3_flag = false;
static unsigned int temp_down_threshold_hotplug_freq2 = 0;
static bool temp_down_threshold_hotplug_freq2_flag = false;
static unsigned int temp_down_threshold_hotplug_freq3 = 0;
static bool temp_down_threshold_hotplug_freq3_flag = false;
#endif
#if (MAX_CORES == 8)
static unsigned int temp_up_threshold_hotplug_freq4 = 0;
static bool temp_up_threshold_hotplug_freq4_flag = false;
static unsigned int temp_up_threshold_hotplug_freq5 = 0;
static bool temp_up_threshold_hotplug_freq5_flag = false;
static unsigned int temp_up_threshold_hotplug_freq6 = 0;
static bool temp_up_threshold_hotplug_freq6_flag = false;
static unsigned int temp_up_threshold_hotplug_freq7 = 0;
static bool temp_up_threshold_hotplug_freq7_flag = false;
static unsigned int temp_down_threshold_hotplug_freq4 = 0;
static bool temp_down_threshold_hotplug_freq4_flag = false;
static unsigned int temp_down_threshold_hotplug_freq5 = 0;
static bool temp_down_threshold_hotplug_freq5_flag = false;
static unsigned int temp_down_threshold_hotplug_freq6 = 0;
static bool temp_down_threshold_hotplug_freq6_flag = false;
static unsigned int temp_down_threshold_hotplug_freq7 = 0;
static bool temp_down_threshold_hotplug_freq7_flag = false;
#endif

// ZZ: hotplug load thresholds array
static int hotplug_thresholds[2][8] = {
    { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
    { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 }
    };

// ZZ: hotplug frequencies thresholds array
static int hotplug_thresholds_freq[2][8] = {
    { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
    { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 }
    };
#endif

// ZZ: Early Suspend variables
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static unsigned int sampling_rate_awake;			// ZZ: for saving sampling rate awake value
static unsigned int up_threshold_awake;				// ZZ: for saving up threshold awake value
static unsigned int down_threshold_awake;			// ZZ: for saving down threshold awake value
static unsigned int smooth_up_awake;				// ZZ: for saving smooth up awake value
static unsigned int freq_limit_awake;				// ZZ: for saving frequency limit awake value
static unsigned int fast_scaling_up_awake;			// Yank: for saving fast scaling awake value for upscaling
static unsigned int fast_scaling_down_awake;			// Yank: for saving fast scaling awake value for downscaling
#ifdef ENABLE_HOTPLUGGING
static unsigned int disable_hotplug_awake;			// ZZ: for saving hotplug switch
static unsigned int hotplug1_awake;				// ZZ: for saving hotplug1 threshold awake value
#if (MAX_CORES == 4 || MAX_CORES == 8)
static unsigned int hotplug2_awake;				// ZZ: for saving hotplug2 threshold awake value
static unsigned int hotplug3_awake;				// ZZ: for saving hotplug3 threshold awake value
#endif
#if (MAX_CORES == 8)
static unsigned int hotplug4_awake;				// ZZ: for saving hotplug4 threshold awake value
static unsigned int hotplug5_awake;				// ZZ: for saving hotplug5 threshold awake value
static unsigned int hotplug6_awake;				// ZZ: for saving hotplug6 threshold awake value
static unsigned int hotplug7_awake;				// ZZ: for saving hotplug7 threshold awake value
#endif
#endif
static unsigned int sampling_rate_asleep;			// ZZ: for setting sampling rate value at early suspend
static unsigned int up_threshold_asleep;			// ZZ: for setting up threshold value at early suspend
static unsigned int down_threshold_asleep;			// ZZ: for setting down threshold value at early suspend
static unsigned int smooth_up_asleep;				// ZZ: for setting smooth scaling value at early suspend
static unsigned int freq_limit_asleep;				// ZZ: for setting frequency limit value at early suspend
static unsigned int fast_scaling_up_asleep;			// Yank: for setting fast scaling value at early suspend for upscaling
static unsigned int fast_scaling_down_asleep;			// Yank: for setting fast scaling value at early suspend for downscaling
#ifdef ENABLE_HOTPLUGGING
static unsigned int disable_hotplug_asleep;			// ZZ: for setting hotplug on/off at early suspend
#endif
#endif

#ifdef ENABLE_HOTPLUGGING
struct work_struct hotplug_offline_work;			// ZZ: hotplugging down work
struct work_struct hotplug_online_work;				// ZZ: hotplugging up work
#endif

static void do_dbs_timer(struct work_struct *work);

struct cpu_dbs_info_s {
	u64 time_in_idle;					// ZZ: for exit time handling
	u64 idle_exit_time;					// ZZ: for exit time handling
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	unsigned int down_skip;					// ZZ: Sampling Down reactivated
	unsigned int requested_freq;
	unsigned int rate_mult;					// ZZ: Sampling Down Momentum - sampling rate multiplier
	unsigned int momentum_adder;				// ZZ: Sampling Down Momentum - adder
	int cpu;
	unsigned int enable:1;
	unsigned int prev_load;					// ZZ: Early Demand - for previous load

	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};

static DEFINE_PER_CPU(struct cpu_dbs_info_s, cs_cpu_dbs_info);
static unsigned int dbs_enable;					// number of CPUs using this policy
static DEFINE_MUTEX(dbs_mutex);					// dbs_mutex protects dbs_enable in governor start/stop.
static struct workqueue_struct *dbs_wq;

static struct dbs_tuners {
	char profile[20];					// ZZ: profile tuneable
	unsigned int profile_number;				// ZZ: profile number tuneable
	unsigned int auto_adjust_freq_thresholds;		// ZZ: auto adjust freq thresholds tuneable
	unsigned int sampling_rate;				// ZZ: normal sampling rate tuneable
	unsigned int sampling_rate_current;			// ZZ: currently active sampling rate tuneable
	unsigned int sampling_rate_idle;			// ZZ: sampling rate at idle tuneable
	unsigned int sampling_rate_idle_threshold;		// ZZ: sampling rate switching threshold tuneable
	unsigned int sampling_rate_idle_delay;			// ZZ: sampling rate switching delay tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int sampling_rate_sleep_multiplier;		// ZZ: sampling rate sleep multiplier tuneable for early suspend
#endif
	unsigned int sampling_down_factor;			// ZZ: sampling down factor tuneable (reactivated)
	unsigned int sampling_down_momentum;			// ZZ: sampling down momentum tuneable
	unsigned int sampling_down_max_mom;			// ZZ: sampling down momentum max tuneable
	unsigned int sampling_down_mom_sens;			// ZZ: sampling down momentum sensitivity tuneable
	unsigned int up_threshold;				// ZZ: scaling up threshold tuneable
#ifdef ENABLE_HOTPLUGGING
	unsigned int up_threshold_hotplug1;			// ZZ: up threshold hotplug tuneable for core1
	unsigned int up_threshold_hotplug_freq1;		// Yank: up threshold hotplug freq tuneable for core1
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int up_threshold_hotplug2;			// ZZ: up threshold hotplug tuneable for core2
	unsigned int up_threshold_hotplug_freq2;		// Yank: up threshold hotplug freq tuneable for core2
	unsigned int up_threshold_hotplug3;			// ZZ: up threshold hotplug tuneable for core3
	unsigned int up_threshold_hotplug_freq3;		// Yank: up threshold hotplug freq tuneable for core3
#endif
#if (MAX_CORES == 8)
	unsigned int up_threshold_hotplug4;			// ZZ: up threshold hotplug tuneable for core4
	unsigned int up_threshold_hotplug_freq4;		// Yank: up threshold hotplug freq tuneable for core4
	unsigned int up_threshold_hotplug5;			// ZZ: up threshold hotplug tuneable for core5
	unsigned int up_threshold_hotplug_freq5;		// Yank: up threshold hotplug freq tuneable for core5
	unsigned int up_threshold_hotplug6;			// ZZ: up threshold hotplug tuneable for core6
	unsigned int up_threshold_hotplug_freq6;		// Yank: up threshold hotplug freq tuneable  for core6
	unsigned int up_threshold_hotplug7;			// ZZ: up threshold hotplug tuneable for core7
	unsigned int up_threshold_hotplug_freq7;		// Yank: up threshold hotplug freq tuneable for core7
#endif
#endif
	unsigned int up_threshold_sleep;			// ZZ: up threshold sleep tuneable for early suspend
	unsigned int down_threshold;				// ZZ: down threshold tuneable
#ifdef ENABLE_HOTPLUGGING
	unsigned int down_threshold_hotplug1;			// ZZ: down threshold hotplug tuneable for core1
	unsigned int down_threshold_hotplug_freq1;		// Yank: down threshold hotplug freq tuneable for core1
#if (MAX_CORES == 4 || MAX_CORES == 8)
	unsigned int down_threshold_hotplug2;			// ZZ: down threshold hotplug tuneable for core2
	unsigned int down_threshold_hotplug_freq2;		// Yank: down threshold hotplug freq tuneable for core2
	unsigned int down_threshold_hotplug3;			// ZZ: down threshold hotplug tuneable for core3
	unsigned int down_threshold_hotplug_freq3;		// Yank: down threshold hotplug freq tuneable for core3
#endif
#if (MAX_CORES == 8)
	unsigned int down_threshold_hotplug4;			// ZZ: down threshold hotplug tuneable for core4
	unsigned int down_threshold_hotplug_freq4;		// Yank: down threshold hotplug freq tuneable for core4
	unsigned int down_threshold_hotplug5;			// ZZ: down threshold hotplug tuneable for core5
	unsigned int down_threshold_hotplug_freq5;		// Yank: down threshold hotplug_freq tuneable for core5
	unsigned int down_threshold_hotplug6;			// ZZ: down threshold hotplug tuneable for core6
	unsigned int down_threshold_hotplug_freq6;		// Yank: down threshold hotplug freq tuneable for core6
	unsigned int down_threshold_hotplug7;			// ZZ: down threshold hotplug tuneable for core7
	unsigned int down_threshold_hotplug_freq7;		// Yank: down threshold hotplug freq tuneable for core7
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int down_threshold_sleep;			// ZZ: down threshold sleep tuneable for early suspend
#endif
	unsigned int ignore_nice;				// ZZ: ignore nice load tuneable
	unsigned int smooth_up;					// ZZ: smooth up tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int smooth_up_sleep;				// ZZ: smooth up sleep tuneable for early suspend
#ifdef ENABLE_HOTPLUGGING
	unsigned int hotplug_sleep;				// ZZ: hotplug sleep tuneable for early suspend
#endif
#endif
	unsigned int freq_limit;				// ZZ: freq limit tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int freq_limit_sleep;				// ZZ: freq limit sleep tuneable for early suspend
#endif
	unsigned int fast_scaling_up;				// Yank: fast scaling tuneable for upscaling
	unsigned int fast_scaling_down;				// Yank: fast scaling tuneable for downscaling
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int fast_scaling_sleep_up;			// Yank: fast scaling sleep tuneable for early suspend for upscaling
	unsigned int fast_scaling_sleep_down;			// Yank: fast scaling sleep tuneable for early suspend for downscaling
#endif
	unsigned int afs_threshold1;				// ZZ: auto fast scaling step one threshold
	unsigned int afs_threshold2;				// ZZ: auto fast scaling step two threshold
	unsigned int afs_threshold3;				// ZZ: auto fast scaling step three threshold
	unsigned int afs_threshold4;				// ZZ: auto fast scaling step four threshold
	unsigned int grad_up_threshold;				// ZZ: early demand grad up threshold tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int grad_up_threshold_sleep;			// ZZ: early demand grad up threshold tuneable for early suspend
#endif
	unsigned int early_demand;				// ZZ: early demand master switch tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int early_demand_sleep;			// ZZ: early demand master switch tuneable for early suspend
#endif
#ifdef ENABLE_HOTPLUGGING
	unsigned int disable_hotplug;				// ZZ: hotplug switch tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	unsigned int disable_hotplug_sleep;			// ZZ: hotplug switch for sleep tuneable for early suspend
#endif
	unsigned int hotplug_block_up_cycles;			// ZZ: hotplug up block cycles tuneable
	unsigned int hotplug_block_down_cycles;			// ZZ: hotplug down block cycles tuneable
	unsigned int hotplug_idle_threshold;			// ZZ: hotplug idle threshold tuneable
	unsigned int hotplug_idle_freq;				// ZZ: hotplug idle freq tuneable
	unsigned int hotplug_engage_freq;			// ZZ: frequency below which we run on only one core (ffolkes)
#endif
	unsigned int scaling_block_threshold;			// ZZ: scaling block threshold tuneable
	unsigned int scaling_block_cycles;			// ZZ: scaling block cycles tuneable
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	unsigned int scaling_block_temp;			// ZZ: scaling block temp tuneable
#endif
	unsigned int scaling_block_freq;			// ZZ: scaling block freq tuneable
	unsigned int scaling_block_force_down;			// ZZ: scaling block force down tuneable
	unsigned int scaling_fastdown_freq;			// ZZ: frequency beyond which we apply a different up threshold (ffolkes)
	unsigned int scaling_fastdown_up_threshold;		// ZZ: up threshold when scaling fastdown freq exceeded (ffolkes)
	unsigned int scaling_fastdown_down_threshold;		// ZZ: down threshold when scaling fastdown freq exceeded (ffolkes)
	unsigned int scaling_responsiveness_freq;		// ZZ: frequency below which we use a lower up threshold (ffolkes)
	unsigned int scaling_responsiveness_up_threshold;	// ZZ: up threshold we use when below scaling responsiveness freq (ffolkes)
	unsigned int scaling_proportional;			// ZZ: proportional to load scaling

// ZZ: set tuneable default values
} dbs_tuners_ins = {
	.profile = "none",
	.profile_number = DEF_PROFILE_NUMBER,
	.auto_adjust_freq_thresholds = DEF_AUTO_ADJUST_FREQ,
	.sampling_rate_idle = DEF_SAMPLING_RATE_IDLE,
	.sampling_rate_idle_threshold = DEF_SAMPLING_RATE_IDLE_THRESHOLD,
	.sampling_rate_idle_delay = DEF_SAMPLING_RATE_IDLE_DELAY,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.sampling_rate_sleep_multiplier = DEF_SAMPLING_RATE_SLEEP_MULTIPLIER,
#endif
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.sampling_down_momentum = DEF_SAMPLING_DOWN_MOMENTUM,
	.sampling_down_max_mom = DEF_SAMPLING_DOWN_MAX_MOMENTUM,
	.sampling_down_mom_sens = DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY,
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
#ifdef ENABLE_HOTPLUGGING
	.up_threshold_hotplug1 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq1 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
#if (MAX_CORES == 4 || MAX_CORES == 8)
	.up_threshold_hotplug2 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq2 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
	.up_threshold_hotplug3 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq3 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
#endif
#if (MAX_CORES == 8)
	.up_threshold_hotplug4 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq4 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
	.up_threshold_hotplug5 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq5 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
	.up_threshold_hotplug6 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq6 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
	.up_threshold_hotplug7 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG,
	.up_threshold_hotplug_freq7 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG_FREQ,
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.up_threshold_sleep = DEF_UP_THRESHOLD_SLEEP,
#endif
	.down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD,
#ifdef ENABLE_HOTPLUGGING
	.down_threshold_hotplug1 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq1 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
#if (MAX_CORES == 4 || MAX_CORES == 8)
	.down_threshold_hotplug2 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq2 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
	.down_threshold_hotplug3 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq3 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
#endif
#if (MAX_CORES == 8)
	.down_threshold_hotplug4 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq4 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
	.down_threshold_hotplug5 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq5 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
	.down_threshold_hotplug6 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq6 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
	.down_threshold_hotplug7 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG,
	.down_threshold_hotplug_freq7 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG_FREQ,
#endif
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.down_threshold_sleep = DEF_DOWN_THRESHOLD_SLEEP,
#endif
	.ignore_nice = DEF_IGNORE_NICE,
	.smooth_up = DEF_SMOOTH_UP,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.smooth_up_sleep = DEF_SMOOTH_UP_SLEEP,
#ifdef ENABLE_HOTPLUGGING
	.hotplug_sleep = DEF_HOTPLUG_SLEEP,
#endif
#endif
	.freq_limit = DEF_FREQ_LIMIT,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.freq_limit_sleep = DEF_FREQ_LIMIT_SLEEP,
#endif
	.fast_scaling_up = DEF_FAST_SCALING_UP,
	.fast_scaling_down = DEF_FAST_SCALING_DOWN,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.fast_scaling_sleep_up = DEF_FAST_SCALING_SLEEP_UP,
	.fast_scaling_sleep_down = DEF_FAST_SCALING_SLEEP_DOWN,
#endif
	.afs_threshold1 = DEF_AFS_THRESHOLD1,
	.afs_threshold2 = DEF_AFS_THRESHOLD2,
	.afs_threshold3 = DEF_AFS_THRESHOLD3,
	.afs_threshold4 = DEF_AFS_THRESHOLD4,
	.grad_up_threshold = DEF_GRAD_UP_THRESHOLD,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.grad_up_threshold_sleep = DEF_GRAD_UP_THRESHOLD_SLEEP,
#endif
	.early_demand = DEF_EARLY_DEMAND,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.early_demand_sleep = DEF_EARLY_DEMAND_SLEEP,
#endif
#ifdef ENABLE_HOTPLUGGING
	.disable_hotplug = DEF_DISABLE_HOTPLUG,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	.disable_hotplug_sleep = DEF_DISABLE_HOTPLUG_SLEEP,
#endif
	.hotplug_block_up_cycles = DEF_HOTPLUG_BLOCK_UP_CYCLES,
	.hotplug_block_down_cycles = DEF_HOTPLUG_BLOCK_DOWN_CYCLES,
	.hotplug_idle_threshold = DEF_HOTPLUG_IDLE_THRESHOLD,
	.hotplug_idle_freq = DEF_HOTPLUG_IDLE_FREQ,
	.hotplug_engage_freq = DEF_HOTPLUG_ENGAGE_FREQ,
#endif
	.scaling_block_threshold = DEF_SCALING_BLOCK_THRESHOLD,
	.scaling_block_cycles = DEF_SCALING_BLOCK_CYCLES,
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	.scaling_block_temp = DEF_SCALING_BLOCK_TEMP,
#endif
	.scaling_block_freq = DEF_SCALING_BLOCK_FREQ,
	.scaling_block_force_down = DEF_SCALING_BLOCK_FORCE_DOWN,
	.scaling_fastdown_freq = DEF_SCALING_FASTDOWN_FREQ,
	.scaling_fastdown_up_threshold = DEF_SCALING_FASTDOWN_UP_THRESHOLD,
	.scaling_fastdown_down_threshold = DEF_SCALING_FASTDOWN_DOWN_THRESHOLD,
	.scaling_responsiveness_freq = DEF_SCALING_RESPONSIVENESS_FREQ,
	.scaling_responsiveness_up_threshold = DEF_SCALING_RESPONSIVENESS_UP_THRESHOLD,
	.scaling_proportional = DEF_SCALING_PROPORTIONAL,
};

/**
 * ZZMoove Scaling by Zane Zaminsky and Yank555 2012/13/14 improved mainly
 * for Samsung I9300 devices but compatible with others too:
 *
 * ZZMoove v0.3		- table modified to reach overclocking frequencies
 *			  up to 1600mhz
 *
 * ZZMoove v0.4		- added fast scaling columns to frequency table
 *
 * ZZMoove v0.5		- removed fast scaling colums and use line jumps
 *			  instead. 4 steps and 2 modes (with/without fast
 *			  downscaling) possible now
 *			- table modified to reach overclocking frequencies
 *			  up to 1800mhz
 *			- fixed wrong frequency stepping
 *			- added search limit for more efficent frequency
 *			  searching and better hard/softlimit handling
 *
 * ZZMoove v0.5.1b	- combination of power and normal scaling table
 *			  to only one array (idea by Yank555)
 *			- scaling logic reworked and optimized by Yank555
 *
 * ZZMoove v0.6		- completely removed lookup tables and use the
 *			  system frequency table instead
 *                        modified scaling logic accordingly
 *			  (credits to Yank555)
 *
 * ZZMoove v0.6a	- added check if CPU freq. table is in ascending
 *			  or descending order and scale accordingly
 *			  (credits to Yank555)
 *
 * ZZMoove v0.7		- reindroduced the 'scaling lookup table way' in
 *			  form of the 'Legacy Mode'
 *
 * ZZMoove v0.7b	- readded forgotten frequency search optimisation
 *
 * ZZMoove v0.7c	- frequency search optimisation now fully compatible
 *			  with ascending ordered system frequency tables
 *
 * ZZMoove v0.8		- added scaling block cycles for a adjustable reduction
 *			  of high frequency overhead (in normal and legacy mode)
 *			- added auto fast scaling mode (aka 'insane' scaling mode)
 *			- added early demand sleep combined with fast scaling and
 *			  switching of sampling rate sleep multiplier
 *			- fixed stopping of up scaling at 100% load when up
 *			  threshold tuneable is set to the max value of 100
 *			- fixed smooth up not active at 100% load when smooth up
 *			  tuneable is set to the max value of 100
 *			- improved freq limit handling in tuneables and in
 *			  dbs_check_cpu function
 *			- moved code parts which are only necessary in legacy mode
 *			  to legacy macros and excluded them also during runtime
 *			- minor optimizations on multiple points in scaling code
 *
 * ZZMoove 0.9 beta1	- splitted fast_scaling into two separate tunables fast_scaling_up
 *			  and fast_scaling_down so each can be set individually to 0-4
 *			  (skip 0-4 frequency steps) or 5 to use autoscaling.
 *			- splitted fast_scaling_sleep into two separate tunables
 *			  fast_scaling_sleep_up and fast_scaling_sleep_down so each
 *			  can be set individually to 0-4
 *			  (skip 0-4 frequency steps) or 5 to use autoscaling.
 *			- removed legacy scaling mode (necessary to be able to
 *			  split fast_scaling tunable)
 *			- added auto fast scaling step tuneables
 *
 * ZZMoove 0.9 beta3	- added scaling fastdown to reduce spending time on
 *			  highest frequencies
 *			- added scaling responsiveness to help eliminate
 *			  lag when starting tasks
 *
 * ZZMoove 0.9 beta4	- added switchable calculation of load-proportional scaling
 *			  frequencies. if enabled always use the lowest frequency compared
 *			  between system table freq and proportional frequency
 *			- removed freq_step because it never had any function in this
 *			  governor
 *
 * ZZMoove 1.0 beta1	- removed unessesary calls of external cpufreq function and use a
 *			  static variable instead to hold the system freq table during runtime
 *			- fixed frequency stuck on max hard and soft frequency limit (under
 *			  some circumstances freq was out of scope for the main search loop)
 *			  and added precautions to avoid problems when for what ever reason
 *			  the freq table is 'messed' or even not available for the governor
 *			- fixed not properly working scaling with descend ordered frequency
 *			  table like it is for example on qualcomm platform
 *			- added additional propotional scaling mode (mode '1' as usual decide
 *			  and use the lowest freq, new mode '2' use only propotional
 *			  frequencies like ondemand governor does
 */

// Yank: return a valid value between min and max
static int validate_min_max(int val, int min, int max)
{
	return min(max(val, min), max);
}

// ZZ: system table scaling mode with freq search optimizations and proportional freq option
static int zz_get_next_freq(unsigned int curfreq, unsigned int updown, unsigned int load)
{
	int i = 0;
	unsigned int prop_target = 0, zz_target = 0;				// ZZ: proportional freq, system table freq
	int smooth_up_steps = 0;						// Yank: smooth up steps

	prop_target = pol_max * load / 100;					// ZZ: prepare proportional target freq

	if (dbs_tuners_ins.scaling_proportional == 2) {				// ZZ: if mode '2' use proportional target freq only
	    return prop_target;
	}

	if (load <= dbs_tuners_ins.smooth_up)					// Yank: consider smooth up
	    smooth_up_steps = 0;						// Yank: load not reached, move by one step
	else
	    smooth_up_steps = 1;						// Yank: load reached, move by two steps

	// ZZ: feq search loop with optimization
	if (freq_table_desc) {
	    for (i = limit_table_start; (likely(system_freq_table[i].frequency >= limit_table_end)); i++) {
		if (unlikely(curfreq == system_freq_table[i].frequency)) {	// Yank: we found where we currently are (i)
		    if (updown == 1) {						// Yank: scale up, but don't go above softlimit
			zz_target = min(system_freq_table[max_scaling_freq_soft].frequency,
		        system_freq_table[validate_min_max(i - 1 - smooth_up_steps - scaling_mode_up, 0, freq_table_size)].frequency);
			if (dbs_tuners_ins.scaling_proportional == 1)		// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);			// ZZ: check which freq is lower and return it
			else
			    return zz_target;					// ZZ: or return the found system table freq as usual
		    } else {							// Yank: scale down, but don't go below min. freq.
			zz_target = max(system_freq_table[min_scaling_freq].frequency,
		        system_freq_table[validate_min_max(i + 1 + scaling_mode_down, 0, freq_table_size)].frequency);
			if (dbs_tuners_ins.scaling_proportional == 1)		// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);			// ZZ: check which freq is lower and return it
			else
			    return zz_target;					// ZZ: or return the found system table freq as usual
		    }
		    return prop_target;						// ZZ: this shouldn't happen but if the freq is not found in system table
		}								//     fall back to proportional freq target to avoid stuck at current freq
	    }
	    return prop_target;							// ZZ: freq not found fallback to proportional freq target
	} else {
	    for (i = limit_table_start; (likely(system_freq_table[i].frequency <= limit_table_end)); i++) {
		if (unlikely(curfreq == system_freq_table[i].frequency)) {	// Yank: we found where we currently are (i)
		    if (updown == 1) {						// Yank: scale up, but don't go above softlimit
			zz_target = min(system_freq_table[max_scaling_freq_soft].frequency,
			system_freq_table[validate_min_max(i + 1 + smooth_up_steps + scaling_mode_up, 0, freq_table_size)].frequency);
			if (dbs_tuners_ins.scaling_proportional == 1)		// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);			// ZZ: check which freq is lower and return it
			else
			    return zz_target;					// ZZ: or return the found system table freq as usual
		    } else {							// Yank: scale down, but don't go below min. freq.
			zz_target = max(system_freq_table[min_scaling_freq].frequency,
			system_freq_table[validate_min_max(i - 1 - scaling_mode_down, 0, freq_table_size)].frequency);
			if (dbs_tuners_ins.scaling_proportional == 1)		// ZZ: if proportional scaling is enabled
			    return min(zz_target, prop_target);			// ZZ: check which freq is lower and return it
			else
			    return zz_target;					// ZZ: or return the found system table freq as usual
		    }
		    return prop_target;						// ZZ: this shouldn't happen but if the freq is not found in system table
		}								//     fall back to proportional freq target to avoid stuck at current freq
	    }
	    return prop_target;							// ZZ: freq not found fallback to proportional freq target
	}
}

#ifdef ENABLE_HOTPLUGGING
// ZZ: function for enabling cores from offline state
static inline void __cpuinit enable_offline_cores(void)
{
	int i = 0;

	for (i = 1; i < possible_cpus; i++) {					// ZZ: enable all offline cores
	    if (!cpu_online(i))
	    cpu_up(i);
	}
	enable_cores = false;							// ZZ: reset enable flag again
}
#endif

// ZZ: function for frequency table order detection and limit optimization
static inline void evaluate_scaling_order_limit_range(bool start, bool limit, bool suspend, unsigned int max_freq)
{
	int i = 0;
	int calc_index = 0;
	system_freq_table = cpufreq_frequency_get_table(0);			// ZZ: update static system frequency table

	/*
	 * ZZ: execute at start and at limit case and in combination with limit case 3 times
	 * to catch all scaling max/min changes at/after gov start
	 */
	if (start || (limit && freq_init_count <= 1)) {

	    // ZZ: initialisation of freq search in scaling table
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++) {
		if (unlikely(max_freq == system_freq_table[i].frequency)) {
		    max_scaling_freq_hard = max_scaling_freq_soft = i;		// ZZ: init soft and hard value
		    // Yank: continue looping until table end is reached, we need this to set the table size limit below
		}
	    }

	    freq_table_size = i - 1;						// Yank: upper index limit of freq. table

	    /*
	     * ZZ: we have to take care about where we are in the frequency table. when using kernel sources without OC capability
	     * it might be that index 0 and 1 contains no frequencies so a save index start point is needed.
	     */
	    calc_index = freq_table_size - max_scaling_freq_hard;		// ZZ: calculate the difference and use it as start point
	    if (calc_index == freq_table_size)					// ZZ: if we are at the end of the table
		calc_index = calc_index - 1;					// ZZ: shift in range for order calculation below

	    // Yank: assert if CPU freq. table is in ascending or descending order
	    if (system_freq_table[calc_index].frequency > system_freq_table[calc_index+1].frequency) {
		freq_table_desc = true;						// Yank: table is in descending order as expected, lowest freq at the bottom of the table
		min_scaling_freq = i - 1;					// Yank: last valid frequency step (lowest frequency)
		limit_table_start = max_scaling_freq_soft;			// ZZ: we should use the actual scaling soft limit value as search start point
	    } else {
		freq_table_desc = false;					// Yank: table is in ascending order, lowest freq at the top of the table
		min_scaling_freq = 0;						// Yank: first valid frequency step (lowest frequency)
		limit_table_start = 0;						// ZZ: start searching at lowest frequency
		limit_table_end = system_freq_table[freq_table_size].frequency;	// ZZ: end searching at highest frequency limit
	    }
	}

	// ZZ: execute at limit case but not at suspend and in combination with start case 3 times at/after gov start
	if ((limit && !suspend) || (limit && freq_init_count <= 1)) {

	    /*
	     * ZZ: obviously the 'limit case' will be executed multiple times at suspend for 'sanity' checks
	     * but we have already a early suspend code to handle scaling search limits so we have to differentiate
	     * to avoid double execution at suspend!
	     */
	    if (max_freq != system_freq_table[max_scaling_freq_hard].frequency) {	// Yank: if policy->max has changed...
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++) {
		    if (unlikely(max_freq == system_freq_table[i].frequency)) {
			max_scaling_freq_hard = i;					// ZZ: ...set new freq scaling index
			break;
		    }
		}
	    }

	    if (dbs_tuners_ins.freq_limit == 0 ||					// Yank: if there is no awake freq. limit
		dbs_tuners_ins.freq_limit > system_freq_table[max_scaling_freq_hard].frequency) {	// Yank: or it is higher than hard max frequency
		max_scaling_freq_soft = max_scaling_freq_hard;				// Yank: use hard max frequency
		if (freq_table_desc)							// ZZ: if descending ordered table is used
		    limit_table_start = max_scaling_freq_soft;				// ZZ: we should use the actual scaling soft limit value as search start point
		else
		    limit_table_end = system_freq_table[freq_table_size].frequency;	// ZZ: set search end point to max frequency when using ascending table
	    } else {
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++) {
		    if (unlikely(dbs_tuners_ins.freq_limit == system_freq_table[i].frequency)) {	// Yank: else lookup awake max. frequency index
			max_scaling_freq_soft = i;
			if (freq_table_desc)						// ZZ: if descending ordered table is used
			    limit_table_start = max_scaling_freq_soft;			// ZZ: we should use the actual scaling soft limit value as search start point
			else
			    limit_table_end = system_freq_table[i].frequency;		// ZZ: set search end point to soft freq limit when using ascending table
		    break;
		    }
		}
	    }
	    if (freq_init_count < 2)							// ZZ: execute start and limit part together 3 times to catch a possible setting of
	    freq_init_count++;								// ZZ: hard freq limit after gov start - after that skip 'start' part during
	}										// ZZ: normal operation and use only limit part to adjust limit optimizations

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	// ZZ: execute only at suspend but not at limit case
	if (suspend && !limit) {							// ZZ: only if we are at suspend
	    if (freq_limit_asleep == 0 ||						// Yank: if there is no sleep frequency limit
		freq_limit_asleep > system_freq_table[max_scaling_freq_hard].frequency) {	// Yank: or it is higher than hard max frequency
		max_scaling_freq_soft = max_scaling_freq_hard;				// Yank: use hard max frequency
		if (freq_table_desc)							// ZZ: if descending ordered table is used
		    limit_table_start = max_scaling_freq_soft;				// ZZ: we should use the actual scaling soft limit value as search start point
		else
		    limit_table_end = system_freq_table[freq_table_size].frequency;	// ZZ: set search end point to max freq when using ascending table
	    } else {
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++) {
		    if (unlikely(freq_limit_asleep == system_freq_table[i].frequency)) {	// Yank: else lookup sleep max. frequency index
			max_scaling_freq_soft = i;
			if (freq_table_desc)						// ZZ: if descending ordered table is used
			    limit_table_start = max_scaling_freq_soft;			// ZZ: we should use the actual scaling soft limit value as search start point
			else
			    limit_table_end = system_freq_table[i].frequency;		// ZZ: set search end point to max frequency when using ascending table
		    break;
		    }
		}
	    }
	}

	// ZZ: execute only at resume but not at limit or start case
	if (!suspend && !limit && !start) {						// ZZ: only if we are not at suspend
	    if (freq_limit_awake == 0 ||						// Yank: if there is no awake frequency limit
		freq_limit_awake > system_freq_table[max_scaling_freq_hard].frequency) {	// Yank: or it is higher than hard max frequency
		max_scaling_freq_soft = max_scaling_freq_hard;				// Yank: use hard max frequency
		if (freq_table_desc)							// ZZ: if descending ordered table is used
		    limit_table_start = max_scaling_freq_soft;				// ZZ: we should use the actual scaling soft limit value as search start point
		else
		    limit_table_end = system_freq_table[freq_table_size].frequency;	// ZZ: set search end point to max freq when using ascending table
	    } else {
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++) {
		    if (unlikely(freq_limit_awake == system_freq_table[i].frequency)) {		// Yank: else lookup awake max. frequency index
			max_scaling_freq_soft = i;
			if (freq_table_desc)						// ZZ: if descending ordered table is used
			    limit_table_start = max_scaling_freq_soft;			// ZZ: we should use the actual scaling soft limit value as search start point
			else
			    limit_table_end = system_freq_table[i].frequency;		// ZZ: set search end point to soft freq limit when using ascending table
		    break;
		    }
		}
	    }
	}
#endif
}

// ZZ: function for auto adjusting frequency thresholds if max policy has changed
static inline void adjust_freq_thresholds(unsigned int step)
{
	if (dbs_tuners_ins.auto_adjust_freq_thresholds != 0 && step != 0 && freq_init_count > 0) {	// ZZ: start adjusting if enabled and after freq search is ready initialized
#ifdef ENABLE_HOTPLUGGING
		// ZZ: adjust hotplug engage freq
		if (dbs_tuners_ins.hotplug_engage_freq != 0) {						// ZZ: adjust only if tuneable is set
		    if ((dbs_tuners_ins.hotplug_engage_freq + step < pol_min
			|| dbs_tuners_ins.hotplug_engage_freq + step > pol_max)
			&& !temp_hotplug_engage_freq_flag) {						// ZZ: check if we would go under/over limits
			temp_hotplug_engage_freq = dbs_tuners_ins.hotplug_engage_freq + step;		// ZZ: if so do it temporary but do not save tuneable yet
			temp_hotplug_engage_freq_flag = true;						// ZZ: set temp saving flag
		    } else if (temp_hotplug_engage_freq_flag) {						// ZZ: last time we were under/over limits
			if (temp_hotplug_engage_freq + step < pol_min
			    || temp_hotplug_engage_freq + step > pol_max) {				// ZZ: and if we are still there
			    temp_hotplug_engage_freq = temp_hotplug_engage_freq + step;			// ZZ: add step to temp var instead of tuneable var
			} else {
			    dbs_tuners_ins.hotplug_engage_freq = temp_hotplug_engage_freq + step;	// ZZ: else use it as offset for next step and finally save it in tuneable
			    temp_hotplug_engage_freq = 0;						// ZZ: reset temp var
			    temp_hotplug_engage_freq_flag = false;					// ZZ: reset temp flag
			}
		    } else {
			dbs_tuners_ins.hotplug_engage_freq += step;					// ZZ: or change it directly in the tuneable if we are in good range
		    }
		}

		// ZZ: adjust hotplug idle freq
		if (dbs_tuners_ins.hotplug_idle_freq != 0) {
		    if ((dbs_tuners_ins.hotplug_idle_freq + step < pol_min
			|| dbs_tuners_ins.hotplug_idle_freq + step > pol_max)
			&& !temp_hotplug_idle_freq_flag) {
			temp_hotplug_idle_freq = dbs_tuners_ins.hotplug_idle_freq + step;
			temp_hotplug_idle_freq_flag = true;
		    } else if (temp_hotplug_idle_freq_flag) {
			if (temp_hotplug_idle_freq + step < pol_min
			    || temp_hotplug_idle_freq + step > pol_max) {
			    temp_hotplug_idle_freq = temp_hotplug_idle_freq + step;
			} else {
			    dbs_tuners_ins.hotplug_idle_freq = temp_hotplug_idle_freq + step;
			    temp_hotplug_idle_freq = 0;
			    temp_hotplug_idle_freq_flag = false;
			}
		    } else {
			dbs_tuners_ins.hotplug_idle_freq += step;
		    }
		}
#endif
		// ZZ: adjust scaling block freq
		if (dbs_tuners_ins.scaling_block_freq != 0) {
		    if ((dbs_tuners_ins.scaling_block_freq + step < pol_min
			|| dbs_tuners_ins.scaling_block_freq + step > pol_max)
			&& !temp_scaling_block_freq_flag) {
			temp_scaling_block_freq = dbs_tuners_ins.scaling_block_freq + step;
			temp_scaling_block_freq_flag = true;
		    } else if (temp_scaling_block_freq_flag) {
			if (temp_scaling_block_freq + step < pol_min
			    || temp_scaling_block_freq + step > pol_max) {
			    temp_scaling_block_freq = temp_scaling_block_freq + step;
			} else {
			    dbs_tuners_ins.scaling_block_freq = temp_scaling_block_freq + step;
			    temp_scaling_block_freq = 0;
			    temp_scaling_block_freq_flag = false;
			}
		    } else {
			dbs_tuners_ins.scaling_block_freq += step;
		    }
		}

		// ZZ: adjust scaling fastdown freq
		if (dbs_tuners_ins.scaling_fastdown_freq != 0) {
		    if ((dbs_tuners_ins.scaling_fastdown_freq + step < pol_min
			|| dbs_tuners_ins.scaling_fastdown_freq + step > pol_max)
			&& !temp_scaling_fastdown_freq_flag) {
			temp_scaling_fastdown_freq = dbs_tuners_ins.scaling_fastdown_freq + step;
			temp_scaling_fastdown_freq_flag = true;
		    } else if (temp_scaling_fastdown_freq_flag) {
			if (temp_scaling_fastdown_freq + step < pol_min
			    || temp_scaling_fastdown_freq + step > pol_max) {
			    temp_scaling_fastdown_freq = temp_scaling_fastdown_freq + step;
			} else {
			    dbs_tuners_ins.scaling_fastdown_freq = temp_scaling_fastdown_freq + step;
			    temp_scaling_fastdown_freq = 0;
			    temp_scaling_fastdown_freq_flag = false;
			}
		    } else {
			dbs_tuners_ins.scaling_fastdown_freq += step;
		    }
		}

		// ZZ: adjust scaling responsiveness freq
		if (dbs_tuners_ins.scaling_responsiveness_freq != 0) {
		    if ((dbs_tuners_ins.scaling_responsiveness_freq + step < pol_min
			|| dbs_tuners_ins.scaling_responsiveness_freq + step > pol_max)
			&& !temp_scaling_responsiveness_freq_flag) {
			temp_scaling_responsiveness_freq = dbs_tuners_ins.scaling_responsiveness_freq + step;
			temp_scaling_responsiveness_freq_flag = true;
		    } else if (temp_scaling_responsiveness_freq_flag) {
			if (temp_scaling_responsiveness_freq + step < pol_min
			    || temp_scaling_responsiveness_freq + step > pol_max) {
			    temp_scaling_responsiveness_freq = temp_scaling_responsiveness_freq + step;
			} else {
			    dbs_tuners_ins.scaling_responsiveness_freq = temp_scaling_responsiveness_freq + step;
			    temp_scaling_responsiveness_freq = 0;
			    temp_scaling_responsiveness_freq_flag = false;
			}
		    } else {
			dbs_tuners_ins.scaling_responsiveness_freq += step;
		    }
		}
#ifdef ENABLE_HOTPLUGGING
		// ZZ: adjust up threshold hotplug freq1
		if (dbs_tuners_ins.up_threshold_hotplug_freq1 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq1 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq1 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq1_flag) {
			temp_up_threshold_hotplug_freq1 = dbs_tuners_ins.up_threshold_hotplug_freq1 + step;
			temp_up_threshold_hotplug_freq1_flag = true;
		    } else if (temp_up_threshold_hotplug_freq1_flag) {
			if (temp_up_threshold_hotplug_freq1 + step < pol_min
			    || temp_up_threshold_hotplug_freq1 + step > pol_max) {
			    temp_up_threshold_hotplug_freq1 = temp_up_threshold_hotplug_freq1 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq1 = temp_up_threshold_hotplug_freq1 + step;
			    temp_up_threshold_hotplug_freq1 = 0;
			    temp_up_threshold_hotplug_freq1_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq1 += step;
		    }
		}
#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: adjust up threshold hotplug freq2
		if (dbs_tuners_ins.up_threshold_hotplug_freq2 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq2 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq2 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq2_flag) {
			temp_up_threshold_hotplug_freq2 = dbs_tuners_ins.up_threshold_hotplug_freq2 + step;
			temp_up_threshold_hotplug_freq2_flag = true;
		    } else if (temp_up_threshold_hotplug_freq2_flag) {
			if (temp_up_threshold_hotplug_freq2 + step < pol_min
			    || temp_up_threshold_hotplug_freq2 + step > pol_max) {
			    temp_up_threshold_hotplug_freq2 = temp_up_threshold_hotplug_freq2 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq2 = temp_up_threshold_hotplug_freq2 + step;
			    temp_up_threshold_hotplug_freq2 = 0;
			    temp_up_threshold_hotplug_freq2_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq2 += step;
		    }
		}

		// ZZ: adjust up threshold hotplug freq3
		if (dbs_tuners_ins.up_threshold_hotplug_freq3 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq3 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq3 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq3_flag) {
			temp_up_threshold_hotplug_freq3 = dbs_tuners_ins.up_threshold_hotplug_freq3 + step;
			temp_up_threshold_hotplug_freq3_flag = true;
		    } else if (temp_up_threshold_hotplug_freq3_flag) {
			if (temp_up_threshold_hotplug_freq3 + step < pol_min
			    || temp_up_threshold_hotplug_freq3 + step > pol_max) {
			    temp_up_threshold_hotplug_freq3 = temp_up_threshold_hotplug_freq3 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq3 = temp_up_threshold_hotplug_freq3 + step;
			    temp_up_threshold_hotplug_freq3 = 0;
			    temp_up_threshold_hotplug_freq3_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq3 += step;
		    }
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: adjust up threshold hotplug freq4
		if (dbs_tuners_ins.up_threshold_hotplug_freq4 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq4 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq4 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq4_flag) {
			temp_up_threshold_hotplug_freq4 = dbs_tuners_ins.up_threshold_hotplug_freq4 + step;
			temp_up_threshold_hotplug_freq4_flag = true;
		    } else if (!temp_up_threshold_hotplug_freq4) {
			if (temp_up_threshold_hotplug_freq4 + step < pol_min
			    || temp_up_threshold_hotplug_freq4 + step > pol_max) {
			    temp_up_threshold_hotplug_freq4 = temp_up_threshold_hotplug_freq4 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq4 = temp_up_threshold_hotplug_freq4 + step;
			    temp_up_threshold_hotplug_freq4 = 0;
			    temp_up_threshold_hotplug_freq4_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq4 += step;
		    }
		}

		// ZZ: adjust up threshold hotplug freq5
		if (dbs_tuners_ins.up_threshold_hotplug_freq5 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq5 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq5 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq5_flag) {
			temp_up_threshold_hotplug_freq5 = dbs_tuners_ins.up_threshold_hotplug_freq5 + step;
			temp_up_threshold_hotplug_freq5_flag = true;
		    } else if (temp_up_threshold_hotplug_freq5_flag) {
			if (temp_up_threshold_hotplug_freq5 + step < pol_min
			    || temp_up_threshold_hotplug_freq5 + step > pol_max) {
			    temp_up_threshold_hotplug_freq5 = temp_up_threshold_hotplug_freq5 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq5 = temp_up_threshold_hotplug_freq5 + step;
			    temp_up_threshold_hotplug_freq5 = 0;
			    temp_up_threshold_hotplug_freq5_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq5 += step;
		    }
		}

		// ZZ: adjust up threshold hotplug freq6
		if (dbs_tuners_ins.up_threshold_hotplug_freq6 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq6 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq6 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq6_flag) {
			temp_up_threshold_hotplug_freq6 = dbs_tuners_ins.up_threshold_hotplug_freq6 + step;
			temp_up_threshold_hotplug_freq6_flag = true;
		    } else if (temp_up_threshold_hotplug_freq6_flag) {
			if (temp_up_threshold_hotplug_freq6 + step < pol_min
			    || temp_up_threshold_hotplug_freq6 + step > pol_max) {
			    temp_up_threshold_hotplug_freq6 = temp_up_threshold_hotplug_freq6 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq6 = temp_up_threshold_hotplug_freq6 + step;
			    temp_up_threshold_hotplug_freq6 = 0;
			    temp_up_threshold_hotplug_freq6_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq6 += step;
		    }
		}

		// ZZ: adjust up threshold hotplug freq7
		if (dbs_tuners_ins.up_threshold_hotplug_freq7 != 0) {
		    if ((dbs_tuners_ins.up_threshold_hotplug_freq7 + step < pol_min
			|| dbs_tuners_ins.up_threshold_hotplug_freq7 + step > pol_max)
			&& !temp_up_threshold_hotplug_freq7_flag) {
			temp_up_threshold_hotplug_freq7 = dbs_tuners_ins.up_threshold_hotplug_freq7 + step;
			temp_up_threshold_hotplug_freq7_flag = true;
		    } else if (temp_up_threshold_hotplug_freq7_flag) {
			if (temp_up_threshold_hotplug_freq7 + step < pol_min
			    || temp_up_threshold_hotplug_freq7 + step > pol_max) {
			    temp_up_threshold_hotplug_freq7 = temp_up_threshold_hotplug_freq7 + step;
			} else {
			    dbs_tuners_ins.up_threshold_hotplug_freq7 = temp_up_threshold_hotplug_freq7 + step;
			    temp_up_threshold_hotplug_freq7 = 0;
			    temp_up_threshold_hotplug_freq7_flag = false;
			}
		    } else {
			dbs_tuners_ins.up_threshold_hotplug_freq7 += step;
		    }
		}
#endif
		// ZZ: adjust down threshold hotplug freq1
		if (dbs_tuners_ins.down_threshold_hotplug_freq1 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq1 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq1 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq1_flag) {
			temp_down_threshold_hotplug_freq1 = dbs_tuners_ins.down_threshold_hotplug_freq1 + step;
			temp_down_threshold_hotplug_freq1_flag = true;
		    } else if (temp_down_threshold_hotplug_freq1_flag) {
			if (temp_down_threshold_hotplug_freq1 + step < pol_min
			    || temp_down_threshold_hotplug_freq1 + step > pol_max) {
			    temp_down_threshold_hotplug_freq1 = temp_down_threshold_hotplug_freq1 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq1 = temp_down_threshold_hotplug_freq1 + step;
			    temp_down_threshold_hotplug_freq1 = 0;
			    temp_down_threshold_hotplug_freq1_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq1 += step;
		    }
		}
#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: adjust down threshold hotplug freq2
		if (dbs_tuners_ins.down_threshold_hotplug_freq2 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq2 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq2 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq2_flag) {
			temp_down_threshold_hotplug_freq2 = dbs_tuners_ins.down_threshold_hotplug_freq2 + step;
			temp_down_threshold_hotplug_freq2_flag = true;
		    } else if (temp_down_threshold_hotplug_freq2_flag) {
			if (temp_down_threshold_hotplug_freq2 + step < pol_min
			    || temp_down_threshold_hotplug_freq2 + step > pol_max) {
			    temp_down_threshold_hotplug_freq2 = temp_down_threshold_hotplug_freq2 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq2 = temp_down_threshold_hotplug_freq2 + step;
			    temp_down_threshold_hotplug_freq2 = 0;
			    temp_down_threshold_hotplug_freq2_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq2 += step;
		    }
		}

		// ZZ: adjust down threshold hotplug freq3
		if (dbs_tuners_ins.down_threshold_hotplug_freq3 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq3 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq3 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq3_flag) {
			temp_down_threshold_hotplug_freq3 = dbs_tuners_ins.down_threshold_hotplug_freq3 + step;
			temp_down_threshold_hotplug_freq3_flag = true;
		    } else if (temp_down_threshold_hotplug_freq3_flag) {
			if (temp_down_threshold_hotplug_freq3 + step < pol_min
			    || temp_down_threshold_hotplug_freq3 + step > pol_max) {
			    temp_down_threshold_hotplug_freq3 = temp_down_threshold_hotplug_freq3 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq3 = temp_down_threshold_hotplug_freq3 + step;
			    temp_down_threshold_hotplug_freq3 = 0;
			    temp_down_threshold_hotplug_freq3_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq3 += step;
		    }
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: adjust down threshold hotplug freq4
		if (dbs_tuners_ins.down_threshold_hotplug_freq4 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq4 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq4 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq4_flag) {
			temp_down_threshold_hotplug_freq4 = dbs_tuners_ins.down_threshold_hotplug_freq4 + step;
			temp_down_threshold_hotplug_freq4_flag = true;
		    } else if (temp_down_threshold_hotplug_freq4_flag) {
			if (temp_down_threshold_hotplug_freq4 + step < pol_min
			    || temp_down_threshold_hotplug_freq4 + step > pol_max) {
			    temp_down_threshold_hotplug_freq4 = temp_down_threshold_hotplug_freq4 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq4 = temp_down_threshold_hotplug_freq4 + step;
			    temp_down_threshold_hotplug_freq4 = 0;
			    temp_down_threshold_hotplug_freq4_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq4 += step;
		    }
		}

		// ZZ: adjust down threshold hotplug freq5
		if (dbs_tuners_ins.down_threshold_hotplug_freq5 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq5 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq5 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq5_flag) {
			temp_down_threshold_hotplug_freq5 = dbs_tuners_ins.down_threshold_hotplug_freq5 + step;
			temp_down_threshold_hotplug_freq5_flag = true;
		    } else if (temp_down_threshold_hotplug_freq5_flag) {
			if (temp_down_threshold_hotplug_freq5 + step < pol_min
			    || temp_down_threshold_hotplug_freq5 + step > pol_max) {
			    temp_down_threshold_hotplug_freq5 = temp_down_threshold_hotplug_freq5 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq5 = temp_down_threshold_hotplug_freq5 + step;
			    temp_down_threshold_hotplug_freq5 = 0;
			    temp_down_threshold_hotplug_freq5_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq5 += step;
		    }
		}

		// ZZ: adjust down threshold hotplug freq6
		if (dbs_tuners_ins.down_threshold_hotplug_freq6 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq6 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq6 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq6_flag) {
			temp_down_threshold_hotplug_freq6 = dbs_tuners_ins.down_threshold_hotplug_freq6 + step;
			temp_down_threshold_hotplug_freq6_flag = true;
		    } else if (temp_down_threshold_hotplug_freq6_flag) {
			if (temp_down_threshold_hotplug_freq6 + step < pol_min
			    || temp_down_threshold_hotplug_freq6 + step > pol_max) {
			    temp_down_threshold_hotplug_freq6 = temp_down_threshold_hotplug_freq6 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq6 = temp_down_threshold_hotplug_freq6 + step;
			    temp_down_threshold_hotplug_freq6 = 0;
			    temp_down_threshold_hotplug_freq6_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq6 += step;
		    }
		}

		// ZZ: adjust down threshold hotplug freq7
		if (dbs_tuners_ins.down_threshold_hotplug_freq7 != 0) {
		    if ((dbs_tuners_ins.down_threshold_hotplug_freq7 + step < pol_min
			|| dbs_tuners_ins.down_threshold_hotplug_freq7 + step > pol_max)
			&& !temp_down_threshold_hotplug_freq7_flag) {
			temp_down_threshold_hotplug_freq7 = dbs_tuners_ins.down_threshold_hotplug_freq7 + step;
			temp_down_threshold_hotplug_freq7_flag = true;
		    } else if (temp_down_threshold_hotplug_freq7_flag) {
			if (temp_down_threshold_hotplug_freq7 + step < pol_min
			    || temp_down_threshold_hotplug_freq7 + step > pol_max) {
			    temp_down_threshold_hotplug_freq7 = temp_down_threshold_hotplug_freq7 + step;
			} else {
			    dbs_tuners_ins.down_threshold_hotplug_freq7 = temp_down_threshold_hotplug_freq7 + step;
			    temp_down_threshold_hotplug_freq7 = 0;
			    temp_down_threshold_hotplug_freq7_flag = false;
			}
		    } else {
			dbs_tuners_ins.down_threshold_hotplug_freq7 += step;
		    }
		}
#endif
#endif /* ENABLE_HOTPLUGGING */
	}
#ifdef ENABLE_HOTPLUGGING
		/*
		 * ZZ: check if maximal freq is lower than any hotplug freq thresholds,
		 * if so overwrite all freq thresholds and therefore fall back
		 * to load thresholds - this keeps hotplugging working properly
		 */
		if (unlikely(pol_max < dbs_tuners_ins.up_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq2
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq3
#endif
#if (MAX_CORES == 8)
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq4
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq5
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq6
		    || pol_max < dbs_tuners_ins.up_threshold_hotplug_freq7
#endif
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq1
#if (MAX_CORES == 4 || MAX_CORES == 8)
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq2
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq3
#endif
#if (MAX_CORES == 8)
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq4
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq5
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq6
		    || pol_max < dbs_tuners_ins.down_threshold_hotplug_freq7
#endif
		    ))
		    max_freq_too_low = true;
		else
		    max_freq_too_low = false;
#endif /* ENABLE_HOTPLUGGING */
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
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
#else
static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu, cputime64_t *wall)
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
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0) /* function has been moved to cpufreq.c in kernel version 3.10 */
#ifndef CPU_IDLE_TIME_IN_CPUFREQ		 /* overrule for sources with backported cpufreq implementation */
static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}
#endif
#endif

// keep track of frequency transitions
static int dbs_cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data)
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
	if (unlikely(this_dbs_info->requested_freq > policy->max
	    || this_dbs_info->requested_freq < policy->min))
		this_dbs_info->requested_freq = freq->new;
	return 0;
}

static struct notifier_block dbs_cpufreq_notifier_block = {
	.notifier_call = dbs_cpufreq_notifier
};

/************************** sysfs interface **************************/
static ssize_t show_sampling_rate_min(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

// cpufreq_zzmoove governor tunables
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(profile_number, profile_number);						// ZZ: profile number tuneable
show_one(auto_adjust_freq_thresholds, auto_adjust_freq_thresholds);			// ZZ: auto adjust freq thresholds tuneable
show_one(sampling_rate, sampling_rate);							// ZZ: normal sampling rate tuneable
show_one(sampling_rate_current, sampling_rate_current);					// ZZ: tuneable for showing the actual sampling rate
show_one(sampling_rate_idle_threshold, sampling_rate_idle_threshold);			// ZZ: sampling rate idle threshold tuneable
show_one(sampling_rate_idle, sampling_rate_idle);					// ZZ: tuneable for sampling rate at idle
show_one(sampling_rate_idle_delay, sampling_rate_idle_delay);				// ZZ: DSR switching delay tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(sampling_rate_sleep_multiplier, sampling_rate_sleep_multiplier);		// ZZ: sampling rate multiplier tuneable for early suspend
#endif
show_one(sampling_down_factor, sampling_down_factor);					// ZZ: sampling down factor tuneable
show_one(sampling_down_max_momentum, sampling_down_max_mom);				// ZZ: sampling down momentum tuneable
show_one(sampling_down_momentum_sensitivity, sampling_down_mom_sens);			// ZZ: sampling down momentum sensitivity tuneable
show_one(up_threshold, up_threshold);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(up_threshold_sleep, up_threshold_sleep);					// ZZ: up threshold sleep tuneable for early suspend
#endif
#ifdef ENABLE_HOTPLUGGING
show_one(up_threshold_hotplug1, up_threshold_hotplug1);					// ZZ: up threshold hotplug tuneable for core1
show_one(up_threshold_hotplug_freq1, up_threshold_hotplug_freq1);			// Yank: up threshold hotplug freq tuneable for core1
#if (MAX_CORES == 4 || MAX_CORES == 8)
show_one(up_threshold_hotplug2, up_threshold_hotplug2);					// ZZ: up threshold hotplug tuneable for core2
show_one(up_threshold_hotplug_freq2, up_threshold_hotplug_freq2);			// Yank: up threshold hotplug freq tuneable for core2
show_one(up_threshold_hotplug3, up_threshold_hotplug3);					// ZZ: up threshold hotplug tuneable for core3
show_one(up_threshold_hotplug_freq3, up_threshold_hotplug_freq3);			// Yank: up threshold hotplug freq tuneable for core3
#endif
#if (MAX_CORES == 8)
show_one(up_threshold_hotplug4, up_threshold_hotplug4);					// ZZ: up threshold hotplug tuneable for core4
show_one(up_threshold_hotplug_freq4, up_threshold_hotplug_freq4);			// Yank: up threshold hotplug freq tuneable for core4
show_one(up_threshold_hotplug5, up_threshold_hotplug5);					// ZZ: up threshold hotplug tuneable for core5
show_one(up_threshold_hotplug_freq5, up_threshold_hotplug_freq5);			// Yank: up threshold hotplug freq tuneable for core5
show_one(up_threshold_hotplug6, up_threshold_hotplug6);					// ZZ: up threshold hotplug tuneable for core6
show_one(up_threshold_hotplug_freq6, up_threshold_hotplug_freq6);			// Yank: up threshold hotplug freq tuneable for core6
show_one(up_threshold_hotplug7, up_threshold_hotplug7);					// ZZ: up threshold hotplug tuneable for core7
show_one(up_threshold_hotplug_freq7, up_threshold_hotplug_freq7);			// Yank: up threshold hotplug freq tuneable for core7
#endif
#endif
show_one(down_threshold, down_threshold);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(down_threshold_sleep, down_threshold_sleep);					// ZZ: down threshold sleep tuneable for early suspend
#endif
#ifdef ENABLE_HOTPLUGGING
show_one(down_threshold_hotplug1, down_threshold_hotplug1);				// ZZ: down threshold hotplug tuneable for core1
show_one(down_threshold_hotplug_freq1, down_threshold_hotplug_freq1);			// Yank: down threshold hotplug freq tuneable for core1
#if (MAX_CORES == 4 || MAX_CORES == 8)
show_one(down_threshold_hotplug2, down_threshold_hotplug2);				// ZZ: down threshold hotplug tuneable for core2
show_one(down_threshold_hotplug_freq2, down_threshold_hotplug_freq2);			// Yank: down threshold hotplug freq tuneable for core2
show_one(down_threshold_hotplug3, down_threshold_hotplug3);				// ZZ: down threshold hotplug tuneable for core3
show_one(down_threshold_hotplug_freq3, down_threshold_hotplug_freq3);			// Yank: down threshold hotplug freq tuneable for core3
#endif
#if (MAX_CORES == 8)
show_one(down_threshold_hotplug4, down_threshold_hotplug4);				// ZZ: down threshold hotplug tuneable for core4
show_one(down_threshold_hotplug_freq4, down_threshold_hotplug_freq4);			// Yank: down threshold hotplug freq tuneable for core4
show_one(down_threshold_hotplug5, down_threshold_hotplug5);				// ZZ: down threshold hotplug tuneable for core5
show_one(down_threshold_hotplug_freq5, down_threshold_hotplug_freq5);			// Yank: down threshold hotplug freq tuneable for core5
show_one(down_threshold_hotplug6, down_threshold_hotplug6);				// ZZ: down threshold hotplug tuneable for core6
show_one(down_threshold_hotplug_freq6, down_threshold_hotplug_freq6);			// Yank: down threshold hotplug freq tuneable for core6
show_one(down_threshold_hotplug7, down_threshold_hotplug7);				// ZZ: down threshold hotplug  tuneable for core7
show_one(down_threshold_hotplug_freq7, down_threshold_hotplug_freq7);			// Yank: down threshold hotplug freq tuneable for core7
#endif
#endif
show_one(ignore_nice_load, ignore_nice);						// ZZ: ignore nice load tuneable
show_one(smooth_up, smooth_up);								// ZZ: smooth up tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(smooth_up_sleep, smooth_up_sleep);						// ZZ: smooth up sleep tuneable for early suspend
#ifdef ENABLE_HOTPLUGGING
show_one(hotplug_sleep, hotplug_sleep);							// ZZ: hotplug sleep tuneable for early suspend
#endif
#endif
show_one(freq_limit, freq_limit);							// ZZ: freq limit tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(freq_limit_sleep, freq_limit_sleep);						// ZZ: freq limit sleep tuneable for early suspend
#endif
show_one(fast_scaling_up, fast_scaling_up);						// Yank: fast scaling tuneable for upscaling
show_one(fast_scaling_down, fast_scaling_down);						// Yank: fast scaling tuneable for downscaling
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(fast_scaling_sleep_up, fast_scaling_sleep_up);					// Yank: fast scaling sleep tuneable for early suspend for upscaling
show_one(fast_scaling_sleep_down, fast_scaling_sleep_down);				// Yank: fast scaling sleep tuneable for early suspend for downscaling
#endif
show_one(afs_threshold1, afs_threshold1);						// ZZ: auto fast scaling step one threshold
show_one(afs_threshold2, afs_threshold2);						// ZZ: auto fast scaling step two threshold
show_one(afs_threshold3, afs_threshold3);						// ZZ: auto fast scaling step three threshold
show_one(afs_threshold4, afs_threshold4);						// ZZ: auto fast scaling step four threshold
show_one(grad_up_threshold, grad_up_threshold);						// ZZ: early demand tuneable grad up threshold
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(grad_up_threshold_sleep, grad_up_threshold_sleep);				// ZZ: early demand sleep tuneable grad up threshold
#endif
show_one(early_demand, early_demand);							// ZZ: early demand tuneable master switch
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(early_demand_sleep, early_demand_sleep);					// ZZ: early demand sleep tuneable master switch
#endif
#ifdef ENABLE_HOTPLUGGING
show_one(disable_hotplug, disable_hotplug);						// ZZ: hotplug switch tuneable
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
show_one(disable_hotplug_sleep, disable_hotplug_sleep);					// ZZ: hotplug switch tuneable for sleep
#endif
show_one(hotplug_block_up_cycles, hotplug_block_up_cycles);				// ZZ: hotplug up block cycles tuneable
show_one(hotplug_block_down_cycles, hotplug_block_down_cycles);				// ZZ: hotplug down block cycles tuneable
show_one(hotplug_idle_threshold, hotplug_idle_threshold);				// ZZ: hotplug idle threshold tuneable
show_one(hotplug_idle_freq, hotplug_idle_freq);						// ZZ: hotplug idle freq tuneable
show_one(hotplug_engage_freq, hotplug_engage_freq);					// ZZ: hotplug engage freq tuneable (ffolkes)
#endif
show_one(scaling_block_threshold, scaling_block_threshold);				// ZZ: scaling block threshold tuneable
show_one(scaling_block_cycles, scaling_block_cycles);					// ZZ: scaling block cycles tuneable
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
show_one(scaling_block_temp, scaling_block_temp);					// ZZ: scaling block temp tuneable
#endif
show_one(scaling_block_freq, scaling_block_freq);					// ZZ: scaling block freq tuneable
show_one(scaling_block_force_down, scaling_block_force_down);				// ZZ: scaling block force down tuneable
show_one(scaling_fastdown_freq, scaling_fastdown_freq);					// ZZ: scaling fastdown freq tuneable (ffolkes)
show_one(scaling_fastdown_up_threshold, scaling_fastdown_up_threshold);			// ZZ: scaling fastdown up threshold tuneable (ffolkes)
show_one(scaling_fastdown_down_threshold, scaling_fastdown_down_threshold);		// ZZ: scaling fastdown down threshold tuneable (ffolkes-ZaneZam)
show_one(scaling_responsiveness_freq, scaling_responsiveness_freq);			// ZZ: scaling responsiveness freq tuneable (ffolkes)
show_one(scaling_responsiveness_up_threshold, scaling_responsiveness_up_threshold);	// ZZ: scaling responsiveness up threshold tuneable (ffolkes)
show_one(scaling_proportional, scaling_proportional);					// ZZ: scaling proportional tuneable

// ZZ: tuneable for showing the currently active governor settings profile
static ssize_t show_profile(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", dbs_tuners_ins.profile);
}

// ZZ: tuneable -> possible values: 0 (disable) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 0
static ssize_t store_sampling_down_max_momentum(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR -
	    dbs_tuners_ins.sampling_down_factor || input < 0 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_down_max_mom = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;

	// ZZ: reset sampling down factor to default if momentum was disabled
	if (dbs_tuners_ins.sampling_down_max_mom == 0)
	    dbs_tuners_ins.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;

	// ZZ: reset momentum_adder and reset down sampling multiplier in case momentum was disabled
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	    if (dbs_tuners_ins.sampling_down_max_mom == 0)
		dbs_info->rate_mult = 1;
	}
	return count;
}

// ZZ: tuneable -> possible values: 1 to MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY, if not set default is 50
static ssize_t store_sampling_down_momentum_sensitivity(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY
	    || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_down_mom_sens = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	// ZZ: reset momentum_adder
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	}
	return count;
}
/*
 * ZZ: tunable for sampling down factor (reactivated function) added reset loop for momentum functionality
 * -> possible values: 1 (disabled) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 1
 */
static ssize_t store_sampling_down_factor(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR
	    || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_down_factor = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	// ZZ: reset down sampling multiplier in case it was active
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_rate = dbs_tuners_ins.sampling_rate_current
	= max(input, min_sampling_rate); // ZZ: set it to new value

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

/*
 * ZZ: tuneable -> possible values: 0 disable whole functionality and same as 'sampling_rate' any value
 * above min_sampling_rate, if not set default is 180000
 */
static ssize_t store_sampling_rate_idle(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0)
	    dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate_idle
	    = dbs_tuners_ins.sampling_rate;	// ZZ: set current and idle rate to normal = disable feature
	else
	    dbs_tuners_ins.sampling_rate_idle = max(input, min_sampling_rate);	// ZZ: set idle rate to new value

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: 0 disable threshold, any value under 100, if not set default is 0
static ssize_t store_sampling_rate_idle_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_rate_idle_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_sampling_rate_idle_delay(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0)
	    sampling_rate_step_up_delay = 0;
	    sampling_rate_step_down_delay = 0;

	dbs_tuners_ins.sampling_rate_idle_delay = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: 1 to 8, if not set default is 2
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_sampling_rate_sleep_multiplier(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_RATE_SLEEP_MULTIPLIER || input < 1
	    || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.sampling_rate_sleep_multiplier = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100
	    || input <= dbs_tuners_ins.down_threshold || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.up_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: range from above down_threshold_sleep value up to 100, if not set default is 90
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_up_threshold_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100
	    || input <= dbs_tuners_ins.down_threshold_sleep || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.up_threshold_sleep = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

#ifdef ENABLE_HOTPLUGGING
// Yank: also use definitions for other hotplug tunables
#define store_up_threshold_hotplug(name,core)							\
static ssize_t store_up_threshold_hotplug##name							\
(struct kobject *a, struct attribute *b, const char *buf, size_t count)				\
{												\
	unsigned int input;									\
	int ret;										\
	ret = sscanf(buf, "%u", &input);							\
												\
	    if (ret != 1 || input < 0 || input > 100 || set_profile_active == true)		\
		return -EINVAL;									\
												\
	    dbs_tuners_ins.up_threshold_hotplug##name = input;					\
	    hotplug_thresholds[0][core] = input;						\
												\
	    if (dbs_tuners_ins.profile_number != 0) {						\
		dbs_tuners_ins.profile_number = 0;						\
		strncpy(dbs_tuners_ins.profile, custom_profile,					\
		sizeof(dbs_tuners_ins.profile));						\
	    }											\
												\
	return count;										\
}

#define store_down_threshold_hotplug(name,core)							\
static ssize_t store_down_threshold_hotplug##name						\
(struct kobject *a, struct attribute *b, const char *buf, size_t count)				\
{												\
	unsigned int input;									\
	int ret;										\
	ret = sscanf(buf, "%u", &input);							\
												\
	    if (ret != 1 || input < 1 || input > 100 || set_profile_active == true)		\
		return -EINVAL;									\
												\
	    dbs_tuners_ins.down_threshold_hotplug##name = input;				\
	    hotplug_thresholds[1][core] = input;						\
												\
	    if (dbs_tuners_ins.profile_number != 0) {						\
		dbs_tuners_ins.profile_number = 0;						\
		strncpy(dbs_tuners_ins.profile, custom_profile,					\
		sizeof(dbs_tuners_ins.profile));						\
	    }											\
												\
	return count;										\
}

/*
 * ZZ: tuneables -> possible values: 0 to disable core (only in up thresholds), range from appropriate
 * down threshold value up to 100, if not set default for up threshold is 68 and for down threshold is 55
 */
store_up_threshold_hotplug(1,0);
store_down_threshold_hotplug(1,0);
#if (MAX_CORES == 4 || MAX_CORES == 8)
store_up_threshold_hotplug(2,1);
store_down_threshold_hotplug(2,1);
store_up_threshold_hotplug(3,2);
store_down_threshold_hotplug(3,2);
#endif
#if (MAX_CORES == 8)
store_up_threshold_hotplug(4,3);
store_down_threshold_hotplug(4,3);
store_up_threshold_hotplug(5,4);
store_down_threshold_hotplug(5,4);
store_up_threshold_hotplug(6,5);
store_down_threshold_hotplug(6,5);
store_up_threshold_hotplug(7,6);
store_down_threshold_hotplug(7,6);
#endif
#endif

static ssize_t store_down_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	// ZZ: cannot be lower than 11 otherwise freq will not fall (conservative governor)
	if (ret != 1 || input < 11 || input > 100 || set_profile_active == true)
	   return -EINVAL;

	// ZZ: instead of failing when set too high set it to the highest it can safely go (ffolkes)
	if (dbs_tuners_ins.up_threshold != 0 && input >= dbs_tuners_ins.up_threshold) {
	    input = dbs_tuners_ins.up_threshold - 1;
	}

	dbs_tuners_ins.down_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: range from 11 to up_threshold_sleep but not up_threshold_sleep, if not set default is 44
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_down_threshold_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	// ZZ: cannot be lower than 11 otherwise freq will not fall (conservative governor)
	if (ret != 1 || input < 11 || input > 100 || set_profile_active == true)
	    return -EINVAL;

	// ZZ: instead of failing when set too high set it to the highest it can safely go (ffolkes)
	if (dbs_tuners_ins.up_threshold != 0 && input >= dbs_tuners_ins.up_threshold) {
	    input = dbs_tuners_ins.up_threshold - 1;
	}

	dbs_tuners_ins.down_threshold_sleep = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input > 1)
	    input = 1;

	if (input == dbs_tuners_ins.ignore_nice) {		// ZZ: nothing to do
	    return count;
	}

	dbs_tuners_ins.ignore_nice = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	// ZZ: we need to re-evaluate prev_cpu_idle
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	for_each_online_cpu(j) {
		 struct cpu_dbs_info_s *dbs_info;
		 dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		 dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
		 &dbs_info->prev_cpu_wall, 0);
#else
		 &dbs_info->prev_cpu_wall);
#endif
#else
		 &dbs_info->prev_cpu_wall, 0);
#endif
		 if (dbs_tuners_ins.ignore_nice)
		     dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
	}
	return count;
#else
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
		 &dbs_info->prev_cpu_wall, 0);
#else
		 &dbs_info->prev_cpu_wall);
#endif
#else
		 &dbs_info->prev_cpu_wall, 0);
#endif
		if (dbs_tuners_ins.ignore_nice)
		    dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;

	}
	return count;
#endif
}

static ssize_t store_smooth_up(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.smooth_up = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable -> possible values: range from 1 to 100, if not set default is 100
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_smooth_up_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.smooth_up_sleep = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

/*
 * ZZ: tuneable -> possible values: 0 do not touch the hotplug values on early suspend,
 * input value 1 to MAX_CORES -> value equals cores to run at early suspend, if not set default is 0 (= all cores enabled)
 */
#ifdef ENABLE_HOTPLUGGING
static ssize_t store_hotplug_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input >= possible_cpus || (input < 0 && input != 0)
	    || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.hotplug_sleep = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif
#endif

/*
 * ZZ: tuneable -> possible values: 0 disable, system table freq->min to freq->max in khz -> freqency soft-limit, if not set default is 0
 * Yank: updated : possible values now depend on the system frequency table only
 */
static ssize_t store_freq_limit(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    max_scaling_freq_soft = max_scaling_freq_hard;
	    if (freq_table_desc)					// ZZ: if descending ordered table is used
		limit_table_start = max_scaling_freq_soft;		// ZZ: we should use the actual scaling soft limit value as search start point
	    else
		limit_table_end = system_freq_table[freq_table_size].frequency;	// ZZ: set search end point to max freq when using ascending table
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		freq_limit_awake = dbs_tuners_ins.freq_limit = input;
#else
		dbs_tuners_ins.freq_limit = input;
#endif
		// ZZ: set profile number to 0 and profile name to custom mode
		if (dbs_tuners_ins.profile_number != 0) {
		    dbs_tuners_ins.profile_number = 0;
		    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		}
		return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max limit
	    return -EINVAL;
	} else {
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		    if (unlikely(system_freq_table[i].frequency == input)) {
			max_scaling_freq_soft = i;
			if (freq_table_desc)				// ZZ: if descending ordered table is used
			    limit_table_start = max_scaling_freq_soft;	// ZZ: we should use the actual scaling soft limit value as search start point
			else
			    limit_table_end = system_freq_table[i].frequency;	// ZZ: set search end point to max soft freq limit when using ascenting table
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
			freq_limit_awake = dbs_tuners_ins.freq_limit = input;
#else
			dbs_tuners_ins.freq_limit = input;
#endif
			// ZZ: set profile number to 0 and profile name to custom mode
			if (dbs_tuners_ins.profile_number != 0) {
			    dbs_tuners_ins.profile_number = 0;
			    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
			}
			return count;
		}
	}
	return -EINVAL;
}

/*
 * ZZ: tuneable -> possible values: 0 disable, system table freq->min to freq->max in khz -> freqency soft-limit,
 * if not set default is 0
 * Yank: updated : possible values now depend on the system frequency table only
 */
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_freq_limit_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep = input;

		// ZZ: set profile number to 0 and profile name to custom mode
		if (dbs_tuners_ins.profile_number != 0) {
		    dbs_tuners_ins.profile_number = 0;
		    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		}
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {	// Yank: allow only frequencies below or equal to hard max
	    return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
		        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		return count;
		}
	}
	return -EINVAL;
}
#endif

// Yank: tuneable -> possible values 1-4 to enable fast scaling and 5 for auto fast scaling (insane scaling)
static ssize_t store_fast_scaling_up(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 5 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.fast_scaling_up = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	if (input > 4)				// ZZ: auto fast scaling mode
	    return count;

	scaling_mode_up = input;		// Yank: fast scaling up only

	return count;
}

// Yank: tuneable -> possible values 1-4 to enable fast scaling and 5 for auto fast scaling (insane scaling)
static ssize_t store_fast_scaling_down(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 5 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.fast_scaling_down = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	if (input > 4)				// ZZ: auto fast scaling mode
	    return count;

	scaling_mode_down = input;		// Yank: fast scaling up only

	return count;
}

// Yank: tuneable -> possible values 1-4 to enable fast scaling and 5 for auto fast scaling (insane scaling) in early suspend
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_fast_scaling_sleep_up(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 5 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.fast_scaling_sleep_up = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	return count;
}
#endif

// Yank: tuneable -> possible values 1-4 to enable fast scaling and 5 for auto fast scaling (insane scaling) in early suspend
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_fast_scaling_sleep_down(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 5 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.fast_scaling_sleep_down = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}

	return count;
}
#endif

// ZZ: tuneable -> possible values from 0 to 100
#define store_afs_threshold(name)								\
static ssize_t store_afs_threshold##name(struct kobject *a, struct attribute *b,		\
				  const char *buf, size_t count)				\
{												\
	unsigned int input;									\
	int ret;										\
	ret = sscanf(buf, "%u", &input);							\
												\
	if (ret != 1 || input > 100 || input < 0 || set_profile_active == true)			\
		return -EINVAL;									\
												\
	dbs_tuners_ins.afs_threshold##name = input;						\
												\
	if (dbs_tuners_ins.profile_number != 0) {						\
	    dbs_tuners_ins.profile_number = 0;							\
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));	\
	}											\
	return count;										\
}												\

store_afs_threshold(1);
store_afs_threshold(2);
store_afs_threshold(3);
store_afs_threshold(4);

// ZZ: Early demand - tuneable grad up threshold -> possible values: from 1 to 100, if not set default is 50
static ssize_t store_grad_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.grad_up_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: Early demand - tuneable grad up threshold sleep -> possible values: from 1 to 100, if not set default is 50
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_grad_up_threshold_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.grad_up_threshold_sleep = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

// ZZ: Early demand - tuneable master switch -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_early_demand(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.early_demand = !!input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: Early demand sleep - tuneable master switch -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_early_demand_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	    dbs_tuners_ins.early_demand_sleep = !!input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

#ifdef ENABLE_HOTPLUGGING
// ZZ: tuneable hotplug switch -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_disable_hotplug(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input > 0) {
	    dbs_tuners_ins.disable_hotplug = true;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
		dbs_tuners_ins.profile_number = 0;
		strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }

	        enable_cores = true;
		queue_work_on(0, dbs_wq, &hotplug_online_work);

	} else {
	    dbs_tuners_ins.disable_hotplug = false;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
		dbs_tuners_ins.profile_number = 0;
		strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	}
	return count;
}
#endif

// ZZ: tuneable hotplug switch for early supend -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
#ifdef ENABLE_HOTPLUGGING
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
static ssize_t store_disable_hotplug_sleep(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input > 0) {
	    dbs_tuners_ins.disable_hotplug_sleep = true;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
		dbs_tuners_ins.profile_number = 0;
		strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	} else {
	    dbs_tuners_ins.disable_hotplug_sleep = false;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
		dbs_tuners_ins.profile_number = 0;
		strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	}
	return count;
}
#endif

// ZZ: tuneable hotplug up block cycles -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_hotplug_block_up_cycles(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0)
	    hplg_up_block_cycles = 0;

	dbs_tuners_ins.hotplug_block_up_cycles = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable hotplug down block cycles -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_hotplug_block_down_cycles(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0)
	    hplg_down_block_cycles = 0;

	dbs_tuners_ins.hotplug_block_down_cycles = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable hotplug idle threshold -> possible values: range from 0 disabled to 100, if not set default is 0
static ssize_t store_hotplug_idle_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (((ret != 1 || input < 0 || input > 100) && input != 0) || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.hotplug_idle_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable hotplug idle frequency -> frequency from where the hotplug idle should begin. possible values: all valid system frequencies
static ssize_t store_hotplug_idle_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    dbs_tuners_ins.hotplug_idle_freq = input;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
	        dbs_tuners_ins.profile_number = 0;
	        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max
		   return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    dbs_tuners_ins.hotplug_idle_freq = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
			strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		    return count;
		}
	}
	return -EINVAL;
}

// ZZ: tuneable -> possible values: range from 0 (disabled) to policy->max, if not set default is 0 (ffolkes)
static ssize_t store_hotplug_engage_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    dbs_tuners_ins.hotplug_engage_freq = input;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
	        dbs_tuners_ins.profile_number = 0;
	        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max
		   return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    dbs_tuners_ins.hotplug_engage_freq = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
			strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		    return count;
		}
	}
	return -EINVAL;
}
#endif /* ENABLE_HOTPLUGGING */

// ZZ: tuneable -> possible values: range from 0 (disabled) to policy->max, if not set default is 0 (ffolkes)
static ssize_t store_scaling_responsiveness_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    dbs_tuners_ins.scaling_responsiveness_freq = input;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
	        dbs_tuners_ins.profile_number = 0;
	        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max
		   return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    dbs_tuners_ins.scaling_responsiveness_freq = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
			strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		    return count;
		}
	}
	return -EINVAL;
}

// ZZ: tuneable -> possible values: range from 11 to 100, if not set default is 30 (ffolkes)
static ssize_t store_scaling_responsiveness_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{

	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 11 || set_profile_active == true)
		return -EINVAL;

	dbs_tuners_ins.scaling_responsiveness_up_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable scaling idle threshold -> possible values: range from 0 disabled to 100, if not set default is 0
static ssize_t store_scaling_block_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (((ret != 1 || input < 0 || input > 100) && input != 0) || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.scaling_block_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable scaling block cycles -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_scaling_block_cycles(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0)
	    scaling_block_cycles_count = 0;

	dbs_tuners_ins.scaling_block_cycles = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
// ZZ: tuneable scaling block temp -> possible values: 0 to disable, values from 30°C to 80°C, if not set default is 0
static ssize_t store_scaling_block_temp(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || (input < 30 && input != 0) || input > 80 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.scaling_block_temp = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}
#endif

// ZZ: tuneable scaling up idle frequency -> frequency from where the scaling up idle should begin. possible values all valid system frequenies
static ssize_t store_scaling_block_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    dbs_tuners_ins.scaling_block_freq = input;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
	        dbs_tuners_ins.profile_number = 0;
	        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max
		   return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    dbs_tuners_ins.scaling_block_freq = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
			strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		    return count;
		}
	}
	return -EINVAL;
}

// ZZ: tuneable scaling block force down -> possible values: 0 to disable, 2 or any value above 2 to enable, if not set default is 2
static ssize_t store_scaling_block_force_down(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || input == 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.scaling_block_force_down = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable scaling_fastdown_freq -> possible values: range from 0 (disabled) to policy->max, if not set default is 0 (ffolkes)
static ssize_t store_scaling_fastdown_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i = 0;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	if (input == 0) {
	    dbs_tuners_ins.scaling_fastdown_freq = input;
	    // ZZ: set profile number to 0 and profile name to custom mode
	    if (dbs_tuners_ins.profile_number != 0) {
	        dbs_tuners_ins.profile_number = 0;
	        strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	    }
	return count;
	}

	if (input > system_freq_table[max_scaling_freq_hard].frequency) {		// Yank: allow only frequencies below or equal to hard max
		   return -EINVAL;
	} else {
	    for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)
		if (unlikely(system_freq_table[i].frequency == input)) {
		    dbs_tuners_ins.scaling_fastdown_freq = input;
		    // ZZ: set profile number to 0 and profile name to custom mode
		    if (dbs_tuners_ins.profile_number != 0) {
			dbs_tuners_ins.profile_number = 0;
			strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
		    }
		    return count;
		}
	}
	return -EINVAL;
}

// ZZ: tuneable scaling_fastdown_up_threshold -> possible values: range from above fastdown up threshold to 100, if not set default is 95 (ffolkes)
static ssize_t store_scaling_fastdown_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input <= dbs_tuners_ins.scaling_fastdown_down_threshold || set_profile_active == true)
		return -EINVAL;

	dbs_tuners_ins.scaling_fastdown_up_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable scaling_fastdown_down_threshold -> possible values: range from 11 to 100, if not set default is 90 (ffolkes)
static ssize_t store_scaling_fastdown_down_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if ((ret != 1 || input > 100 || (input < 11 && input >= dbs_tuners_ins.scaling_fastdown_up_threshold)) || set_profile_active == true)
		return -EINVAL;

	dbs_tuners_ins.scaling_fastdown_down_threshold = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: tuneable scaling proportinal -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_scaling_proportional(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input < 0 || input > 2 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.scaling_proportional = input;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// ZZ: function for switching a settings profile either at governor start by macro 'DEF_PROFILE_NUMBER' or later by tuneable 'profile_number'
static inline int set_profile(int profile_num)
{
	int i = 0;					// ZZ: for main profile loop
	int t = 0;					// ZZ: for sub-loop
	unsigned int j;					// ZZ: for update routines

	set_profile_active = true;			// ZZ: avoid additional setting of tuneables during following loop

	for (i = 0; (unlikely(zzmoove_profiles[i].profile_number != PROFILE_TABLE_END)); i++) {
	    if (unlikely(zzmoove_profiles[i].profile_number == profile_num)) {

#ifdef ENABLE_HOTPLUGGING
		// ZZ: set disable_hotplug value
		if (zzmoove_profiles[i].disable_hotplug > 0) {
		    dbs_tuners_ins.disable_hotplug = true;
		    enable_cores = true;
		    queue_work_on(0, dbs_wq, &hotplug_online_work);
		} else {
		    dbs_tuners_ins.disable_hotplug = false;
		}

		// ZZ: set disable_hotplug_sleep value
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		if (zzmoove_profiles[i].disable_hotplug_sleep > 0)
		    dbs_tuners_ins.disable_hotplug_sleep = true;
		else
		    dbs_tuners_ins.disable_hotplug_sleep = false;

		// ZZ: set hotplug_sleep value
		if (zzmoove_profiles[i].hotplug_sleep <= possible_cpus && zzmoove_profiles[i].hotplug_sleep >= 0)
		    dbs_tuners_ins.hotplug_sleep = zzmoove_profiles[i].hotplug_sleep;
#endif
#endif
		// ZZ: set down_threshold value
		if (zzmoove_profiles[i].down_threshold > 11 && zzmoove_profiles[i].down_threshold <= 100
		    && zzmoove_profiles[i].down_threshold < zzmoove_profiles[i].up_threshold)
		    dbs_tuners_ins.down_threshold = zzmoove_profiles[i].down_threshold;

#ifdef ENABLE_HOTPLUGGING
		// ZZ: set down_threshold_hotplug1 value
		if ((zzmoove_profiles[i].down_threshold_hotplug1 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug1 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug1 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug1 = zzmoove_profiles[i].down_threshold_hotplug1;
		    hotplug_thresholds[0][0] = zzmoove_profiles[i].down_threshold_hotplug1;
		}
#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: set down_threshold_hotplug2 value
		if ((zzmoove_profiles[i].down_threshold_hotplug2 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug2 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug2 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug2 = zzmoove_profiles[i].down_threshold_hotplug2;
		    hotplug_thresholds[0][1] = zzmoove_profiles[i].down_threshold_hotplug2;
		}

		// ZZ: set down_threshold_hotplug3 value
		if ((zzmoove_profiles[i].down_threshold_hotplug3 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug3 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug3 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug3 = zzmoove_profiles[i].down_threshold_hotplug3;
		    hotplug_thresholds[0][2] = zzmoove_profiles[i].down_threshold_hotplug3;
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: set down_threshold_hotplug4 value
		if ((zzmoove_profiles[i].down_threshold_hotplug4 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug4 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug4 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug4 = zzmoove_profiles[i].down_threshold_hotplug4;
		    hotplug_thresholds[0][3] = zzmoove_profiles[i].down_threshold_hotplug4;
		}

		// ZZ: set down_threshold_hotplug5 value
		if ((zzmoove_profiles[i].down_threshold_hotplug5 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug5 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug5 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug5 = zzmoove_profiles[i].down_threshold_hotplug5;
		    hotplug_thresholds[0][4] = zzmoove_profiles[i].down_threshold_hotplug5;
		}

		// ZZ: set down_threshold_hotplug6 value
		if ((zzmoove_profiles[i].down_threshold_hotplug6 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug6 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug6 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug6 = zzmoove_profiles[i].down_threshold_hotplug6;
		    hotplug_thresholds[0][5] = zzmoove_profiles[i].down_threshold_hotplug6;
		}

		// ZZ: set down_threshold_hotplug7 value
		if ((zzmoove_profiles[i].down_threshold_hotplug7 <= 100
		    && zzmoove_profiles[i].down_threshold_hotplug7 >= 1)
		    || zzmoove_profiles[i].down_threshold_hotplug7 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug7 = zzmoove_profiles[i].down_threshold_hotplug7;
		    hotplug_thresholds[0][6] = zzmoove_profiles[i].down_threshold_hotplug7;
		}
#endif
		// ZZ: set down_threshold_hotplug_freq1 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq1 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq1 = zzmoove_profiles[i].down_threshold_hotplug_freq1;
		    hotplug_thresholds_freq[1][0] = zzmoove_profiles[i].down_threshold_hotplug_freq1;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq1 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq1) {
			    dbs_tuners_ins.down_threshold_hotplug_freq1 = zzmoove_profiles[i].down_threshold_hotplug_freq1;
			    hotplug_thresholds_freq[1][0] = zzmoove_profiles[i].down_threshold_hotplug_freq1;
			}
		    }
		}
#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: set down_threshold_hotplug_freq2 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq2 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq2 = zzmoove_profiles[i].down_threshold_hotplug_freq2;
		    hotplug_thresholds_freq[1][1] = zzmoove_profiles[i].down_threshold_hotplug_freq2;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq2 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq2) {
			    dbs_tuners_ins.down_threshold_hotplug_freq2 = zzmoove_profiles[i].down_threshold_hotplug_freq2;
			    hotplug_thresholds_freq[1][1] = zzmoove_profiles[i].down_threshold_hotplug_freq2;
			}
		    }
		}

		// ZZ: set down_threshold_hotplug_freq3 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq3 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq3 = zzmoove_profiles[i].down_threshold_hotplug_freq3;
		    hotplug_thresholds_freq[1][2] = zzmoove_profiles[i].down_threshold_hotplug_freq3;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq3 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq3) {
			    dbs_tuners_ins.down_threshold_hotplug_freq3 = zzmoove_profiles[i].down_threshold_hotplug_freq3;
			    hotplug_thresholds_freq[1][2] = zzmoove_profiles[i].down_threshold_hotplug_freq3;
			}
		    }
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: set down_threshold_hotplug_freq4 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq4 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq4 = zzmoove_profiles[i].down_threshold_hotplug_freq4;
		    hotplug_thresholds_freq[1][3] = zzmoove_profiles[i].down_threshold_hotplug_freq4;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq4 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq4) {
			    dbs_tuners_ins.down_threshold_hotplug_freq4 = zzmoove_profiles[i].down_threshold_hotplug_freq4;
			    hotplug_thresholds_freq[1][3] = zzmoove_profiles[i].down_threshold_hotplug_freq4;
			}
		    }
		}

		// ZZ: set down_threshold_hotplug_freq5 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq5 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq5 = zzmoove_profiles[i].down_threshold_hotplug_freq5;
		    hotplug_thresholds_freq[1][4] = zzmoove_profiles[i].down_threshold_hotplug_freq5;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq5 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq5) {
			    dbs_tuners_ins.down_threshold_hotplug_freq5 = zzmoove_profiles[i].down_threshold_hotplug_freq5;
			    hotplug_thresholds_freq[1][4] = zzmoove_profiles[i].down_threshold_hotplug_freq5;
			}
		    }
		}

		// ZZ: set down_threshold_hotplug_freq6 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq6 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq6 = zzmoove_profiles[i].down_threshold_hotplug_freq6;
		    hotplug_thresholds_freq[1][5] = zzmoove_profiles[i].down_threshold_hotplug_freq6;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq6 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq6) {
			    dbs_tuners_ins.down_threshold_hotplug_freq6 = zzmoove_profiles[i].down_threshold_hotplug_freq6;
			    hotplug_thresholds_freq[1][5] = zzmoove_profiles[i].down_threshold_hotplug_freq6;
			}
		    }
		}

		// ZZ: set down_threshold_hotplug_freq7 value
		if (zzmoove_profiles[i].down_threshold_hotplug_freq7 == 0) {
		    dbs_tuners_ins.down_threshold_hotplug_freq7 = zzmoove_profiles[i].down_threshold_hotplug_freq7;
		    hotplug_thresholds_freq[1][6] = zzmoove_profiles[i].down_threshold_hotplug_freq7;
		}

		if (system_freq_table && zzmoove_profiles[i].down_threshold_hotplug_freq7 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].down_threshold_hotplug_freq7) {
			    dbs_tuners_ins.down_threshold_hotplug_freq7 = zzmoove_profiles[i].down_threshold_hotplug_freq7;
			    hotplug_thresholds_freq[1][6] = zzmoove_profiles[i].down_threshold_hotplug_freq7;
			}
		    }
		}
#endif
#endif /* ENABLE_HOTPLUGGING */
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set down_threshold_sleep value
		if (zzmoove_profiles[i].down_threshold_sleep > 11 && zzmoove_profiles[i].down_threshold_sleep <= 100
		    && zzmoove_profiles[i].down_threshold_sleep < dbs_tuners_ins.up_threshold_sleep)
		    dbs_tuners_ins.down_threshold_sleep = zzmoove_profiles[i].down_threshold_sleep;
#endif
		// ZZ: set early_demand value
		dbs_tuners_ins.early_demand = !!zzmoove_profiles[i].early_demand;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		dbs_tuners_ins.early_demand_sleep = !!zzmoove_profiles[i].early_demand_sleep;
#endif
		// Yank: set fast_scaling value
		if (zzmoove_profiles[i].fast_scaling_up <= 5 && zzmoove_profiles[i].fast_scaling_up >= 0) {
			dbs_tuners_ins.fast_scaling_up = zzmoove_profiles[i].fast_scaling_up;
			if (zzmoove_profiles[i].fast_scaling_up > 4)
				scaling_mode_up = 0;
			else
				scaling_mode_up = zzmoove_profiles[i].fast_scaling_up;
		}

		if (zzmoove_profiles[i].fast_scaling_down <= 5 && zzmoove_profiles[i].fast_scaling_down >= 0) {
			dbs_tuners_ins.fast_scaling_down = zzmoove_profiles[i].fast_scaling_down;
			if (zzmoove_profiles[i].fast_scaling_down > 4)
				scaling_mode_down = 0;
			else
				scaling_mode_down = zzmoove_profiles[i].fast_scaling_down;
		}
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set fast_scaling_sleep value
		if (zzmoove_profiles[i].fast_scaling_sleep_up <= 5 && zzmoove_profiles[i].fast_scaling_sleep_up >= 0)
			dbs_tuners_ins.fast_scaling_sleep_up = zzmoove_profiles[i].fast_scaling_sleep_up;

		if (zzmoove_profiles[i].fast_scaling_sleep_down <= 5 && zzmoove_profiles[i].fast_scaling_sleep_down >= 0)
			dbs_tuners_ins.fast_scaling_sleep_down = zzmoove_profiles[i].fast_scaling_sleep_down;
#endif
		// ZZ: set afs_threshold1 value
		if (zzmoove_profiles[i].afs_threshold1 <= 100)
		    dbs_tuners_ins.afs_threshold1 = zzmoove_profiles[i].afs_threshold1;

		// ZZ: set afs_threshold2 value
		if (zzmoove_profiles[i].afs_threshold2 <= 100)
		    dbs_tuners_ins.afs_threshold2 = zzmoove_profiles[i].afs_threshold2;

		// ZZ: set afs_threshold3 value
		if (zzmoove_profiles[i].afs_threshold3 <= 100)
		    dbs_tuners_ins.afs_threshold3 = zzmoove_profiles[i].afs_threshold3;

		// ZZ: set afs_threshold4 value
		if (zzmoove_profiles[i].afs_threshold4 <= 100)
		    dbs_tuners_ins.afs_threshold4 = zzmoove_profiles[i].afs_threshold4;

		// ZZ: set freq_limit value
		if (system_freq_table && zzmoove_profiles[i].freq_limit == 0) {
		    max_scaling_freq_soft = max_scaling_freq_hard;

		    if (freq_table_desc)
			limit_table_start = max_scaling_freq_soft;
		    else
			limit_table_end = system_freq_table[freq_table_size].frequency;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		    freq_limit_awake = dbs_tuners_ins.freq_limit = zzmoove_profiles[i].freq_limit;
#else
		    dbs_tuners_ins.freq_limit = zzmoove_profiles[i].freq_limit;
#endif
		} else if (system_freq_table && zzmoove_profiles[i].freq_limit <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].freq_limit) {
			    max_scaling_freq_soft = t;
			    if (freq_table_desc)
				limit_table_start = max_scaling_freq_soft;
			    else
				limit_table_end = system_freq_table[t].frequency;
			}
		    }
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		    freq_limit_awake = dbs_tuners_ins.freq_limit = zzmoove_profiles[i].freq_limit;
#else
		    dbs_tuners_ins.freq_limit = zzmoove_profiles[i].freq_limit;
#endif
		}

		// ZZ: set freq_limit_sleep value
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		if (system_freq_table && zzmoove_profiles[i].freq_limit_sleep == 0) {
		    freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep = zzmoove_profiles[i].freq_limit_sleep;

		} else if (system_freq_table && zzmoove_profiles[i].freq_limit_sleep <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].freq_limit_sleep)
			    freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep = zzmoove_profiles[i].freq_limit_sleep;
		    }
		}
#endif
		// ZZ: set grad_up_threshold value
		if (zzmoove_profiles[i].grad_up_threshold < 100 && zzmoove_profiles[i].grad_up_threshold > 1)
		    dbs_tuners_ins.grad_up_threshold = zzmoove_profiles[i].grad_up_threshold;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set grad_up_threshold sleep value
		if (zzmoove_profiles[i].grad_up_threshold_sleep < 100 && zzmoove_profiles[i].grad_up_threshold_sleep > 1)
		    dbs_tuners_ins.grad_up_threshold_sleep = zzmoove_profiles[i].grad_up_threshold_sleep;
#endif
#ifdef ENABLE_HOTPLUGGING
		// ZZ: set hotplug_block_up_cycles value
		if (zzmoove_profiles[i].hotplug_block_up_cycles >= 0)
		    dbs_tuners_ins.hotplug_block_up_cycles = zzmoove_profiles[i].hotplug_block_up_cycles;

		// ZZ: set hotplug_block_down_cycles value
		if (zzmoove_profiles[i].hotplug_block_down_cycles >= 0)
		    dbs_tuners_ins.hotplug_block_down_cycles = zzmoove_profiles[i].hotplug_block_down_cycles;

		// ZZ: set hotplug_idle_threshold value
		if (zzmoove_profiles[i].hotplug_idle_threshold >= 0 && zzmoove_profiles[i].hotplug_idle_threshold < 100)
		    dbs_tuners_ins.hotplug_idle_threshold = zzmoove_profiles[i].hotplug_idle_threshold;

		// ZZ: set hotplug_idle_freq value
		if (zzmoove_profiles[i].hotplug_idle_freq == 0) {
		    dbs_tuners_ins.hotplug_idle_freq = zzmoove_profiles[i].hotplug_idle_freq;

		} else if (system_freq_table && zzmoove_profiles[i].hotplug_idle_freq <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].hotplug_idle_freq) {
			    dbs_tuners_ins.hotplug_idle_freq = zzmoove_profiles[i].hotplug_idle_freq;
			}
		    }
		}

		// ZZ: set hotplug_engage_freq value
		if (zzmoove_profiles[i].hotplug_engage_freq == 0) {
		    dbs_tuners_ins.hotplug_engage_freq = zzmoove_profiles[i].hotplug_engage_freq;

		} else if (system_freq_table && zzmoove_profiles[i].hotplug_engage_freq <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].hotplug_engage_freq) {
			    dbs_tuners_ins.hotplug_engage_freq = zzmoove_profiles[i].hotplug_engage_freq;
			}
		    }
		}
#endif
		// ZZ: set scaling_responsiveness_freq value
		if (zzmoove_profiles[i].scaling_responsiveness_freq == 0) {
		    dbs_tuners_ins.scaling_responsiveness_freq = zzmoove_profiles[i].scaling_responsiveness_freq;

		} else if (system_freq_table && zzmoove_profiles[i].scaling_responsiveness_freq <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].scaling_responsiveness_freq) {
			    dbs_tuners_ins.scaling_responsiveness_freq = zzmoove_profiles[i].scaling_responsiveness_freq;
			}
		    }
		}

		// ZZ: set scaling_proportional value
		if (zzmoove_profiles[i].scaling_proportional > 2)
		    zzmoove_profiles[i].scaling_proportional = 2;

		dbs_tuners_ins.scaling_proportional = zzmoove_profiles[i].scaling_proportional;

		// ZZ: set scaling_responsiveness_up_threshold value
		if (zzmoove_profiles[i].scaling_responsiveness_up_threshold <= 100 && zzmoove_profiles[i].scaling_responsiveness_up_threshold >= 11)
		    dbs_tuners_ins.scaling_responsiveness_up_threshold = zzmoove_profiles[i].scaling_responsiveness_up_threshold;

		// ZZ: set ignore_nice_load value
		if (zzmoove_profiles[i].ignore_nice_load > 1)
		    zzmoove_profiles[i].ignore_nice_load = 1;

		dbs_tuners_ins.ignore_nice = zzmoove_profiles[i].ignore_nice_load;

		// we need to re-evaluate prev_cpu_idle
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		for_each_online_cpu(j) {
		     struct cpu_dbs_info_s *dbs_info;
		     dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		     dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
		     &dbs_info->prev_cpu_wall, 0);
#else
		     &dbs_info->prev_cpu_wall);
#endif
#else
		     &dbs_info->prev_cpu_wall, 0);
#endif
		 if (dbs_tuners_ins.ignore_nice)
		     dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		 }
#else
		for_each_online_cpu(j) {
		    struct cpu_dbs_info_s *dbs_info;
		    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		    dbs_info->prev_cpu_idle = get_cpu_idle_time(j, 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
		     &dbs_info->prev_cpu_wall, 0);
#else
		     &dbs_info->prev_cpu_wall);
#endif
#else
		     &dbs_info->prev_cpu_wall, 0);
#endif
		if (dbs_tuners_ins.ignore_nice)
		    dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
		}
#endif
		// ZZ: set sampling_down_factor value
		if (zzmoove_profiles[i].sampling_down_factor <= MAX_SAMPLING_DOWN_FACTOR
		    && zzmoove_profiles[i].sampling_down_factor >= 1)
		    dbs_tuners_ins.sampling_down_factor = zzmoove_profiles[i].sampling_down_factor;

		    // ZZ: Reset down sampling multiplier in case it was active
		    for_each_online_cpu(j) {
			struct cpu_dbs_info_s *dbs_info;
			dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			dbs_info->rate_mult = 1;
		    }

		// ZZ: set sampling_down_max_momentum value
		if (zzmoove_profiles[i].sampling_down_max_momentum <= MAX_SAMPLING_DOWN_FACTOR - dbs_tuners_ins.sampling_down_factor
		    && zzmoove_profiles[i].sampling_down_max_momentum >= 0) {
		    dbs_tuners_ins.sampling_down_max_mom = zzmoove_profiles[i].sampling_down_max_momentum;
		    orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;
		}

		// ZZ: Reset sampling down factor to default if momentum was disabled
		if (dbs_tuners_ins.sampling_down_max_mom == 0)
		    dbs_tuners_ins.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;

		    // ZZ: Reset momentum_adder and reset down sampling multiplier in case momentum was disabled
		    for_each_online_cpu(j) {
			struct cpu_dbs_info_s *dbs_info;
			dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			dbs_info->momentum_adder = 0;
			if (dbs_tuners_ins.sampling_down_max_mom == 0)
			dbs_info->rate_mult = 1;
		    }

		// ZZ: set sampling_down_momentum_sensitivity value
		if (zzmoove_profiles[i].sampling_down_momentum_sensitivity <= MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY
		    && zzmoove_profiles[i].sampling_down_momentum_sensitivity >= 1) {
		    dbs_tuners_ins.sampling_down_mom_sens = zzmoove_profiles[i].sampling_down_momentum_sensitivity;

		    // ZZ: Reset momentum_adder
		    for_each_online_cpu(j) {
			struct cpu_dbs_info_s *dbs_info;
			dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			dbs_info->momentum_adder = 0;
		    }

		// ZZ: set sampling_rate value
		dbs_tuners_ins.sampling_rate = dbs_tuners_ins.sampling_rate_current
		= max(zzmoove_profiles[i].sampling_rate, min_sampling_rate);

		// ZZ: set sampling_rate_idle value
		if (zzmoove_profiles[i].sampling_rate_idle == 0) {
		    dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate
		    = dbs_tuners_ins.sampling_rate_idle;
		} else {
		    dbs_tuners_ins.sampling_rate_idle = max(zzmoove_profiles[i].sampling_rate_idle, min_sampling_rate);
		}

		// ZZ: set sampling_rate_idle_delay value
		if (zzmoove_profiles[i].sampling_rate_idle_delay >= 0) {
		    sampling_rate_step_up_delay = 0;
		    sampling_rate_step_down_delay = 0;
		    dbs_tuners_ins.sampling_rate_idle_delay = zzmoove_profiles[i].sampling_rate_idle_delay;
		}

		// ZZ: set sampling_rate_idle_threshold value
		if (zzmoove_profiles[i].sampling_rate_idle_threshold <= 100)
		    dbs_tuners_ins.sampling_rate_idle_threshold = zzmoove_profiles[i].sampling_rate_idle_threshold;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set sampling_rate_sleep_multiplier value
		if (zzmoove_profiles[i].sampling_rate_sleep_multiplier <= MAX_SAMPLING_RATE_SLEEP_MULTIPLIER
		    && zzmoove_profiles[i].sampling_rate_sleep_multiplier >= 1)
		    dbs_tuners_ins.sampling_rate_sleep_multiplier = zzmoove_profiles[i].sampling_rate_sleep_multiplier;
#endif
		// ZZ: set scaling_block_cycles value
		if (zzmoove_profiles[i].scaling_block_cycles >= 0) {
		    dbs_tuners_ins.scaling_block_cycles = zzmoove_profiles[i].scaling_block_cycles;
		    if (zzmoove_profiles[i].scaling_block_cycles == 0)
			scaling_block_cycles_count = 0;
		}
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		// ZZ: set scaling_block_temp value
		if ((zzmoove_profiles[i].scaling_block_temp >= 30 && zzmoove_profiles[i].scaling_block_temp <= 80)
		    || zzmoove_profiles[i].scaling_block_temp == 0) {
		    dbs_tuners_ins.scaling_block_temp = zzmoove_profiles[i].scaling_block_temp;
		}
#endif
		// ZZ: set scaling_block_freq value
		if (zzmoove_profiles[i].scaling_block_freq == 0) {
		    dbs_tuners_ins.scaling_block_freq = zzmoove_profiles[i].scaling_block_freq;

		} else if (system_freq_table && zzmoove_profiles[i].scaling_block_freq <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].scaling_block_freq) {
			    dbs_tuners_ins.scaling_block_freq = zzmoove_profiles[i].scaling_block_freq;
			}
		    }
		}

		// ZZ: set scaling_block_threshold value
		if (zzmoove_profiles[i].scaling_block_threshold >= 0
		    && zzmoove_profiles[i].scaling_block_threshold <= 100)
		    dbs_tuners_ins.scaling_block_threshold = zzmoove_profiles[i].scaling_block_threshold;

		// ZZ: set scaling_block_force_down value
		if (zzmoove_profiles[i].scaling_block_force_down >= 0
		    && zzmoove_profiles[i].scaling_block_force_down != 1)
		    dbs_tuners_ins.scaling_block_force_down = zzmoove_profiles[i].scaling_block_force_down;

		// ZZ: set scaling_fastdown_freq value
		if (zzmoove_profiles[i].scaling_fastdown_freq == 0) {
		    dbs_tuners_ins.scaling_fastdown_freq = zzmoove_profiles[i].scaling_fastdown_freq;

		} else if (system_freq_table && zzmoove_profiles[i].scaling_fastdown_freq <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].scaling_fastdown_freq) {
			    dbs_tuners_ins.scaling_fastdown_freq = zzmoove_profiles[i].scaling_fastdown_freq;
			}
		    }
		}

		// ZZ: set scaling_fastdown_up_threshold value
		if (zzmoove_profiles[i].scaling_fastdown_up_threshold <= 100 && zzmoove_profiles[i].scaling_fastdown_up_threshold
		    > zzmoove_profiles[i].scaling_fastdown_down_threshold)
		    dbs_tuners_ins.scaling_fastdown_up_threshold = zzmoove_profiles[i].scaling_fastdown_up_threshold;

		// ZZ: set scaling_fastdown_down_threshold value
		if (zzmoove_profiles[i].scaling_fastdown_down_threshold < zzmoove_profiles[i].scaling_fastdown_up_threshold
		    && zzmoove_profiles[i].scaling_fastdown_down_threshold > 11)
		    dbs_tuners_ins.scaling_fastdown_down_threshold = zzmoove_profiles[i].scaling_fastdown_down_threshold;

		// ZZ: set smooth_up value
		if (zzmoove_profiles[i].smooth_up <= 100 && zzmoove_profiles[i].smooth_up >= 1)
		    dbs_tuners_ins.smooth_up = zzmoove_profiles[i].smooth_up;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set smooth_up_sleep value
		if (zzmoove_profiles[i].smooth_up_sleep <= 100 && zzmoove_profiles[i].smooth_up_sleep >= 1)
		    dbs_tuners_ins.smooth_up_sleep = zzmoove_profiles[i].smooth_up_sleep;
#endif
		// ZZ: set up_threshold value
		if (zzmoove_profiles[i].up_threshold <= 100 && zzmoove_profiles[i].up_threshold
		    >= zzmoove_profiles[i].down_threshold)
		    dbs_tuners_ins.up_threshold = zzmoove_profiles[i].up_threshold;
#ifdef ENABLE_HOTPLUGGING
		// ZZ: set up_threshold_hotplug1 value
		if (zzmoove_profiles[i].up_threshold_hotplug1 >= 0 && zzmoove_profiles[i].up_threshold_hotplug1 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug1 = zzmoove_profiles[i].up_threshold_hotplug1;
		    hotplug_thresholds[0][0] = zzmoove_profiles[i].up_threshold_hotplug1;
		}
#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: set up_threshold_hotplug2 value
		if (zzmoove_profiles[i].up_threshold_hotplug2 >= 0 && zzmoove_profiles[i].up_threshold_hotplug2 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug2 = zzmoove_profiles[i].up_threshold_hotplug2;
		    hotplug_thresholds[0][1] = zzmoove_profiles[i].up_threshold_hotplug2;
		}

		// ZZ: set up_threshold_hotplug3 value
		if (zzmoove_profiles[i].up_threshold_hotplug3 >= 0 && zzmoove_profiles[i].up_threshold_hotplug3 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug3 = zzmoove_profiles[i].up_threshold_hotplug3;
		    hotplug_thresholds[0][2] = zzmoove_profiles[i].up_threshold_hotplug3;
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: set up_threshold_hotplug4 value
		if (zzmoove_profiles[i].up_threshold_hotplug4 >= 0 && zzmoove_profiles[i].up_threshold_hotplug4 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug4 = zzmoove_profiles[i].up_threshold_hotplug4;
		    hotplug_thresholds[0][3] = zzmoove_profiles[i].up_threshold_hotplug4;
		}

		// ZZ: set up_threshold_hotplug5 value
		if (zzmoove_profiles[i].up_threshold_hotplug5 >= 0 && zzmoove_profiles[i].up_threshold_hotplug5 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug5 = zzmoove_profiles[i].up_threshold_hotplug5;
		    hotplug_thresholds[0][4] = zzmoove_profiles[i].up_threshold_hotplug5;
		}

		// ZZ: set up_threshold_hotplug6 value
		if (zzmoove_profiles[i].up_threshold_hotplug6 >= 0 && zzmoove_profiles[i].up_threshold_hotplug6 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug6 = zzmoove_profiles[i].up_threshold_hotplug6;
		    hotplug_thresholds[0][5] = zzmoove_profiles[i].up_threshold_hotplug6;
		}

		// ZZ: set up_threshold_hotplug7 value
		if (zzmoove_profiles[i].up_threshold_hotplug7 >= 0 && zzmoove_profiles[i].up_threshold_hotplug7 <= 100) {
		    dbs_tuners_ins.up_threshold_hotplug7 = zzmoove_profiles[i].up_threshold_hotplug7;
		    hotplug_thresholds[0][6] = zzmoove_profiles[i].up_threshold_hotplug7;
		}
#endif
		// ZZ: set up_threshold_hotplug_freq1 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq1 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq1 = zzmoove_profiles[i].up_threshold_hotplug_freq1;
		    hotplug_thresholds_freq[0][0] = zzmoove_profiles[i].up_threshold_hotplug_freq1;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq1 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
		        if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq1) {
			    dbs_tuners_ins.up_threshold_hotplug_freq1 = zzmoove_profiles[i].up_threshold_hotplug_freq1;
			    hotplug_thresholds_freq[0][0] = zzmoove_profiles[i].up_threshold_hotplug_freq1;
		        }
		    }
		}

#if (MAX_CORES == 4 || MAX_CORES == 8)
		// ZZ: set up_threshold_hotplug_freq2 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq2 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq2 = zzmoove_profiles[i].up_threshold_hotplug_freq2;
		    hotplug_thresholds_freq[0][1] = zzmoove_profiles[i].up_threshold_hotplug_freq2;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq2 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq2) {
			    dbs_tuners_ins.up_threshold_hotplug_freq2 = zzmoove_profiles[i].up_threshold_hotplug_freq2;
			    hotplug_thresholds_freq[0][1] = zzmoove_profiles[i].up_threshold_hotplug_freq2;
			}
		    }
		}

		// ZZ: set up_threshold_hotplug_freq3 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq3 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq3 = zzmoove_profiles[i].up_threshold_hotplug_freq3;
		    hotplug_thresholds_freq[0][2] = zzmoove_profiles[i].up_threshold_hotplug_freq3;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq3 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq3) {
			    dbs_tuners_ins.up_threshold_hotplug_freq3 = zzmoove_profiles[i].up_threshold_hotplug_freq3;
			    hotplug_thresholds_freq[0][2] = zzmoove_profiles[i].up_threshold_hotplug_freq3;
			}
		    }
		}
#endif
#if (MAX_CORES == 8)
		// ZZ: set up_threshold_hotplug_freq4 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq4 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq4 = zzmoove_profiles[i].up_threshold_hotplug_freq4;
		    hotplug_thresholds_freq[0][3] = zzmoove_profiles[i].up_threshold_hotplug_freq4;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq4 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq4) {
			    dbs_tuners_ins.up_threshold_hotplug_freq4 = zzmoove_profiles[i].up_threshold_hotplug_freq4;
			    hotplug_thresholds_freq[0][3] = zzmoove_profiles[i].up_threshold_hotplug_freq4;
			}
		    }
		}

		// ZZ: set up_threshold_hotplug_freq5 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq5 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq5 = zzmoove_profiles[i].up_threshold_hotplug_freq5;
		    hotplug_thresholds_freq[0][4] = zzmoove_profiles[i].up_threshold_hotplug_freq5;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq5 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq5) {
			    dbs_tuners_ins.up_threshold_hotplug_freq5 = zzmoove_profiles[i].up_threshold_hotplug_freq5;
			    hotplug_thresholds_freq[0][4] = zzmoove_profiles[i].up_threshold_hotplug_freq5;
			}
		    }
		}

		// ZZ: set up_threshold_hotplug_freq6 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq6 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq6 = zzmoove_profiles[i].up_threshold_hotplug_freq6;
		    hotplug_thresholds_freq[0][5] = zzmoove_profiles[i].up_threshold_hotplug_freq6;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq6 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq6) {
			    dbs_tuners_ins.up_threshold_hotplug_freq6 = zzmoove_profiles[i].up_threshold_hotplug_freq6;
			    hotplug_thresholds_freq[0][5] = zzmoove_profiles[i].up_threshold_hotplug_freq6;
			}
		    }
		}

		// ZZ: set up_threshold_hotplug_freq7 value
		if (zzmoove_profiles[i].up_threshold_hotplug_freq7 == 0) {
		    dbs_tuners_ins.up_threshold_hotplug_freq7 = zzmoove_profiles[i].up_threshold_hotplug_freq7;
		    hotplug_thresholds_freq[0][6] = zzmoove_profiles[i].up_threshold_hotplug_freq7;
		}

		if (system_freq_table && zzmoove_profiles[i].up_threshold_hotplug_freq7 <= system_freq_table[max_scaling_freq_hard].frequency) {
		    for (t = 0; (system_freq_table[t].frequency != system_table_end); t++) {
			if (system_freq_table[t].frequency == zzmoove_profiles[i].up_threshold_hotplug_freq7) {
			    dbs_tuners_ins.up_threshold_hotplug_freq7 = zzmoove_profiles[i].up_threshold_hotplug_freq7;
			    hotplug_thresholds_freq[0][6] = zzmoove_profiles[i].up_threshold_hotplug_freq7;
			}
		    }
		}
#endif
#endif /* ENABLE_HOTPLUGGING */
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		// ZZ: set up_threshold_sleep value
		if (zzmoove_profiles[i].up_threshold_sleep <= 100 && zzmoove_profiles[i].up_threshold_sleep
		    > dbs_tuners_ins.down_threshold_sleep)
		    dbs_tuners_ins.up_threshold_sleep = zzmoove_profiles[i].up_threshold_sleep;
#endif
		// ZZ: set auto_adjust_freq_thresholds value
		if (zzmoove_profiles[i].auto_adjust_freq_thresholds > 1) {
		    zzmoove_profiles[i].auto_adjust_freq_thresholds = 1;
		    dbs_tuners_ins.auto_adjust_freq_thresholds = zzmoove_profiles[i].auto_adjust_freq_thresholds;
		} else {
		    dbs_tuners_ins.auto_adjust_freq_thresholds = zzmoove_profiles[i].auto_adjust_freq_thresholds;
		    pol_step = 0;
		}

		// ZZ: set current profile number
		dbs_tuners_ins.profile_number = profile_num;

		// ZZ: set current profile name
		strncpy(dbs_tuners_ins.profile, zzmoove_profiles[i].profile_name, sizeof(dbs_tuners_ins.profile));
		set_profile_active = false; // ZZ: profile found - allow setting of tuneables again
		return 1;
	    }
	}
    }
// ZZ: profile not found - allow setting of tuneables again
set_profile_active = false;
return 0;
}

// ZZ: tunable profile number -> for switching settings profiles, check zzmoove_profiles.h file for possible values
static ssize_t store_profile_number(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;				// ZZ: regular input handling of this tuneable
	int ret_profile;				// ZZ: return value for set_profile function
	int ret;					// ZZ: regular input handling of this tuneable

	ret = sscanf(buf, "%u", &input);		// ZZ: regular input handling of this tuneable

	if (ret != 1)
	    return -EINVAL;

	// ZZ: if input is 0 set profile to custom mode
	if (input == 0) {
	    dbs_tuners_ins.profile_number = input;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	return count;
	}

	// ZZ: set profile and check result
	ret_profile = set_profile(input);

	if (ret_profile != 1)
	    return -EINVAL; // ZZ: given profile not available
	else
	    return count; // ZZ: profile found return as normal
}

// ZZ: tunable auto adjust freq thresholds -> for a automatic adjustment of all freq thresholds.
static ssize_t store_auto_adjust_freq_thresholds(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || set_profile_active == true)
	    return -EINVAL;

	dbs_tuners_ins.auto_adjust_freq_thresholds = !!input;

	if (input == 0)
	    pol_step = 0;

	// ZZ: set profile number to 0 and profile name to custom mode
	if (dbs_tuners_ins.profile_number != 0) {
	    dbs_tuners_ins.profile_number = 0;
	    strncpy(dbs_tuners_ins.profile, custom_profile, sizeof(dbs_tuners_ins.profile));
	}
	return count;
}

// Yank: add hotplug up/down threshold sysfs store interface
#ifdef ENABLE_HOTPLUGGING
#define store_up_threshold_hotplug_freq(name,core)						\
static ssize_t store_up_threshold_hotplug_freq##name						\
(struct kobject *a, struct attribute *b, const char *buf, size_t count)				\
{												\
	unsigned int input;									\
	int ret;										\
	int i = 0;										\
												\
	ret = sscanf(buf, "%u", &input);							\
	if (ret != 1 || set_profile_active == true)						\
	    return -EINVAL;									\
												\
	if (input == 0) {									\
	    dbs_tuners_ins.up_threshold_hotplug_freq##name = input;				\
	    hotplug_thresholds_freq[0][core] = input;						\
	    if (dbs_tuners_ins.profile_number != 0) {						\
		dbs_tuners_ins.profile_number = 0;						\
		strncpy(dbs_tuners_ins.profile, custom_profile,					\
		sizeof(dbs_tuners_ins.profile));						\
	    }											\
	return count;										\
	}											\
												\
												\
	if (input > system_freq_table[max_scaling_freq_hard].frequency) {			\
	    return -EINVAL;									\
	} else {										\
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)	\
		    if (unlikely(system_freq_table[i].frequency == input)) {			\
			dbs_tuners_ins.up_threshold_hotplug_freq##name = input;			\
			hotplug_thresholds_freq[0][core] = input;				\
			if (dbs_tuners_ins.profile_number != 0) {				\
			    dbs_tuners_ins.profile_number = 0;					\
			    strncpy(dbs_tuners_ins.profile, custom_profile,			\
			    sizeof(dbs_tuners_ins.profile));					\
			}									\
			return count;								\
		    }										\
	}											\
	return -EINVAL;										\
}

#define store_down_threshold_hotplug_freq(name,core)						\
static ssize_t store_down_threshold_hotplug_freq##name						\
(struct kobject *a, struct attribute *b, const char *buf, size_t count)				\
{												\
	unsigned int input;									\
	int ret;										\
	int i = 0;										\
												\
	ret = sscanf(buf, "%u", &input);							\
	if (ret != 1 || set_profile_active == true)						\
	    return -EINVAL;									\
												\
	if (input == 0) {									\
	    dbs_tuners_ins.down_threshold_hotplug_freq##name = input;				\
	    hotplug_thresholds_freq[1][core] = input;						\
	    if (dbs_tuners_ins.profile_number != 0) {						\
		dbs_tuners_ins.profile_number = 0;						\
		strncpy(dbs_tuners_ins.profile, custom_profile,					\
		sizeof(dbs_tuners_ins.profile));						\
	    }											\
	return count;										\
	}											\
												\
												\
	if (input > system_freq_table[max_scaling_freq_hard].frequency) {			\
	    return -EINVAL;									\
	} else {										\
		for (i = 0; (likely(system_freq_table[i].frequency != system_table_end)); i++)	\
		    if (unlikely(system_freq_table[i].frequency == input)) {			\
			dbs_tuners_ins.down_threshold_hotplug_freq##name = input;		\
			hotplug_thresholds_freq[1][core] = input;				\
			if (dbs_tuners_ins.profile_number != 0) {				\
			    dbs_tuners_ins.profile_number = 0;					\
			    strncpy(dbs_tuners_ins.profile, custom_profile,			\
			    sizeof(dbs_tuners_ins.profile));					\
			}									\
		    return count;								\
		    }										\
	}											\
	return -EINVAL;										\
}

/*
 * ZZ: tuneables -> possible values: 0 to disable core (only in up thresholds), range from above
 * appropriate down thresholds up to scaling max frequency, if not set default for up and down
 * thresholds is 0
 */
store_up_threshold_hotplug_freq(1,0);
store_down_threshold_hotplug_freq(1,0);
#if (MAX_CORES == 4 || MAX_CORES == 8)
store_up_threshold_hotplug_freq(2,1);
store_down_threshold_hotplug_freq(2,1);
store_up_threshold_hotplug_freq(3,2);
store_down_threshold_hotplug_freq(3,2);
#endif
#if (MAX_CORES == 8)
store_up_threshold_hotplug_freq(4,3);
store_down_threshold_hotplug_freq(4,3);
store_up_threshold_hotplug_freq(5,4);
store_down_threshold_hotplug_freq(5,4);
store_up_threshold_hotplug_freq(6,5);
store_down_threshold_hotplug_freq(6,5);
store_up_threshold_hotplug_freq(7,6);
store_down_threshold_hotplug_freq(7,6);
#endif
#endif /* ENABLE_HOTPLUGGING */

define_one_global_rw(profile_number);
define_one_global_ro(profile);
define_one_global_rw(auto_adjust_freq_thresholds);
define_one_global_ro(sampling_rate_current);
define_one_global_rw(sampling_rate);
define_one_global_rw(sampling_rate_idle_threshold);
define_one_global_rw(sampling_rate_idle);
define_one_global_rw(sampling_rate_idle_delay);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(sampling_rate_sleep_multiplier);
#endif
define_one_global_rw(sampling_down_factor);
define_one_global_rw(sampling_down_max_momentum);
define_one_global_rw(sampling_down_momentum_sensitivity);
define_one_global_rw(up_threshold);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(up_threshold_sleep);
#endif
#ifdef ENABLE_HOTPLUGGING
define_one_global_rw(up_threshold_hotplug1);
define_one_global_rw(up_threshold_hotplug_freq1);
#if (MAX_CORES == 4 || MAX_CORES == 8)
define_one_global_rw(up_threshold_hotplug2);
define_one_global_rw(up_threshold_hotplug_freq2);
define_one_global_rw(up_threshold_hotplug3);
define_one_global_rw(up_threshold_hotplug_freq3);
#endif
#if (MAX_CORES == 8)
define_one_global_rw(up_threshold_hotplug4);
define_one_global_rw(up_threshold_hotplug_freq4);
define_one_global_rw(up_threshold_hotplug5);
define_one_global_rw(up_threshold_hotplug_freq5);
define_one_global_rw(up_threshold_hotplug6);
define_one_global_rw(up_threshold_hotplug_freq6);
define_one_global_rw(up_threshold_hotplug7);
define_one_global_rw(up_threshold_hotplug_freq7);
#endif
#endif
define_one_global_rw(down_threshold);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(down_threshold_sleep);
#endif
#ifdef ENABLE_HOTPLUGGING
define_one_global_rw(down_threshold_hotplug1);
define_one_global_rw(down_threshold_hotplug_freq1);
#if (MAX_CORES == 4 || MAX_CORES == 8)
define_one_global_rw(down_threshold_hotplug2);
define_one_global_rw(down_threshold_hotplug_freq2);
define_one_global_rw(down_threshold_hotplug3);
define_one_global_rw(down_threshold_hotplug_freq3);
#endif
#if (MAX_CORES == 8)
define_one_global_rw(down_threshold_hotplug4);
define_one_global_rw(down_threshold_hotplug_freq4);
define_one_global_rw(down_threshold_hotplug5);
define_one_global_rw(down_threshold_hotplug_freq5);
define_one_global_rw(down_threshold_hotplug6);
define_one_global_rw(down_threshold_hotplug_freq6);
define_one_global_rw(down_threshold_hotplug7);
define_one_global_rw(down_threshold_hotplug_freq7);
#endif
#endif
define_one_global_rw(ignore_nice_load);
define_one_global_rw(smooth_up);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(smooth_up_sleep);
#ifdef ENABLE_HOTPLUGGING
define_one_global_rw(hotplug_sleep);
#endif
#endif
define_one_global_rw(freq_limit);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(freq_limit_sleep);
#endif
define_one_global_rw(fast_scaling_up);
define_one_global_rw(fast_scaling_down);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(fast_scaling_sleep_up);
define_one_global_rw(fast_scaling_sleep_down);
#endif
define_one_global_rw(afs_threshold1);
define_one_global_rw(afs_threshold2);
define_one_global_rw(afs_threshold3);
define_one_global_rw(afs_threshold4);
define_one_global_rw(grad_up_threshold);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(grad_up_threshold_sleep);
#endif
define_one_global_rw(early_demand);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(early_demand_sleep);
#endif
#ifdef ENABLE_HOTPLUGGING
define_one_global_rw(disable_hotplug);
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
define_one_global_rw(disable_hotplug_sleep);
#endif
define_one_global_rw(hotplug_block_up_cycles);
define_one_global_rw(hotplug_block_down_cycles);
define_one_global_rw(hotplug_idle_threshold);
define_one_global_rw(hotplug_idle_freq);
define_one_global_rw(hotplug_engage_freq);
#endif
define_one_global_rw(scaling_block_threshold);
define_one_global_rw(scaling_block_cycles);
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
define_one_global_rw(scaling_block_temp);
#endif
define_one_global_rw(scaling_block_freq);
define_one_global_rw(scaling_block_force_down);
define_one_global_rw(scaling_fastdown_freq);
define_one_global_rw(scaling_fastdown_up_threshold);
define_one_global_rw(scaling_fastdown_down_threshold);
define_one_global_rw(scaling_responsiveness_freq);
define_one_global_rw(scaling_responsiveness_up_threshold);
define_one_global_rw(scaling_proportional);

// Yank: version info tunable
static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", ZZMOOVE_VERSION);
}

static DEVICE_ATTR(version, S_IRUGO , show_version, NULL);

// ZZ: profiles version info tunable
static ssize_t show_version_profiles(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", profiles_file_version);
}

static DEVICE_ATTR(version_profiles, S_IRUGO , show_version_profiles, NULL);

// ZZ: print out all available profiles
static ssize_t show_profile_list(struct device *dev, struct device_attribute *attr, char *buf)
{
    int i = 0, c = 0;
    char profiles[256];

    for (i = 0; (zzmoove_profiles[i].profile_number != PROFILE_TABLE_END); i++) {
	c += sprintf(profiles+c, "profile: %d " "name: %s\n", zzmoove_profiles[i].profile_number,
	zzmoove_profiles[i].profile_name);
    }
    return sprintf(buf, profiles);
}

static DEVICE_ATTR(profile_list, S_IRUGO , show_profile_list, NULL);

#ifdef ZZMOOVE_DEBUG
// Yank: debug info
static ssize_t show_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
#ifdef ENABLE_HOTPLUGGING
    return sprintf(buf, "available cores                : %d\n"
#else
    return sprintf(buf, "core 0 online                  : %d\n"
#endif
#ifdef ENABLE_HOTPLUGGING
			"core 0 online                  : %d\n"
#endif
			"core 1 online                  : %d\n"
			"core 2 online                  : %d\n"
			"core 3 online                  : %d\n"
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
			"current cpu temp               : %d\n"
#endif
			"max freq                       : %d\n"
			"min freq                       : %d\n"
			"auto adjust step               : %d\n"
			"current load                   : %d\n"
			"current frequency              : %d\n"
			"current sampling rate          : %u\n"
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
			"freq limit awake               : %u\n"
			"freq limit asleep              : %u\n"
#endif
			"freq table in desc order       : %d\n"
			"freq table size                : %u\n"
			"limit table start              : %u\n"
			"max scaling freq hard          : %u\n"
			"max scaling freq soft          : %u\n"
			"freq init count                : %u\n"
			"scaling boost                  : %d\n"
			"scaling cancel up              : %d\n"
			"scaling mode up                : %d\n"
			"scaling force down             : %d\n"
			"scaling mode down              : %d\n"
			"scaling up threshold           : %d\n"
#ifdef ENABLE_HOTPLUGGING
			"scaling down threshold         : %d\n"
			"hotplug up threshold1          : %d\n"
			"hotplug up threshold2          : %d\n"
			"hotplug up threshold3          : %d\n"
			"hotplug up threshold1 freq     : %d\n"
			"hotplug up threshold2 freq     : %d\n"
			"hotplug up threshold3 freq     : %d\n"
			"hotplug down threshold1        : %d\n"
			"hotplug down threshold2        : %d\n"
			"hotplug down threshold3        : %d\n"
			"hotplug down threshold1 freq   : %d\n"
			"hotplug down threshold2 freq   : %d\n"
			"hotplug down threshold3 freq   : %d\n",
#else
			"scaling down threshold         : %d\n",
#endif
#ifdef ENABLE_HOTPLUGGING
			possible_cpus,
#endif
			cpu_online(0),
			cpu_online(1),
			cpu_online(2),
			cpu_online(3),
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
			cpu_temp,
#endif
			pol_max,
			pol_min,
			pol_step,
			cur_load,
			cur_freq,
			dbs_tuners_ins.sampling_rate_current,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
			freq_limit_awake,
			freq_limit_asleep,
#endif
			freq_table_desc,
			freq_table_size,
			limit_table_start,
			max_scaling_freq_hard,
			max_scaling_freq_soft,
			freq_init_count,
			boost_freq,
			cancel_up_scaling,
			scaling_mode_up,
			force_down_scaling,
			scaling_mode_down,
			scaling_up_threshold,
#ifdef ENABLE_HOTPLUGGING
			scaling_down_threshold,
			hotplug_thresholds[0][0],
			hotplug_thresholds[0][1],
			hotplug_thresholds[0][2],
			hotplug_thresholds_freq[0][0],
			hotplug_thresholds_freq[0][1],
			hotplug_thresholds_freq[0][2],
			hotplug_thresholds[1][0],
			hotplug_thresholds[1][1],
			hotplug_thresholds[1][2],
			hotplug_thresholds_freq[1][0],
			hotplug_thresholds_freq[1][1],
			hotplug_thresholds_freq[1][2]);
#else
			scaling_down_threshold);
#endif
}

static DEVICE_ATTR(debug, S_IRUGO , show_debug, NULL);
#endif

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&sampling_rate_current.attr,
	&sampling_rate_idle_threshold.attr,
	&sampling_rate_idle.attr,
	&sampling_rate_idle_delay.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&sampling_rate_sleep_multiplier.attr,
#endif
	&sampling_down_factor.attr,
	&sampling_down_max_momentum.attr,
	&sampling_down_momentum_sensitivity.attr,
	&up_threshold.attr,
	&down_threshold.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&up_threshold_sleep.attr,
	&down_threshold_sleep.attr,
#endif
	&ignore_nice_load.attr,
	&smooth_up.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&smooth_up_sleep.attr,
#endif
	&freq_limit.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&freq_limit_sleep.attr,
#endif
	&fast_scaling_up.attr,
	&fast_scaling_down.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&fast_scaling_sleep_up.attr,
	&fast_scaling_sleep_down.attr,
#endif
	&afs_threshold1.attr,
	&afs_threshold2.attr,
	&afs_threshold3.attr,
	&afs_threshold4.attr,
	&grad_up_threshold.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&grad_up_threshold_sleep.attr,
#endif
	&early_demand.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&early_demand_sleep.attr,
#ifdef ENABLE_HOTPLUGGING
	&hotplug_sleep.attr,
#endif
#endif
#ifdef ENABLE_HOTPLUGGING
	&disable_hotplug.attr,
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
	&disable_hotplug_sleep.attr,
#endif
	&hotplug_block_up_cycles.attr,
	&hotplug_block_down_cycles.attr,
	&hotplug_idle_threshold.attr,
	&hotplug_idle_freq.attr,
	&hotplug_engage_freq.attr,
#endif
	&scaling_block_threshold.attr,
	&scaling_block_cycles.attr,
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	&scaling_block_temp.attr,
#endif
	&scaling_block_freq.attr,
	&scaling_block_force_down.attr,
	&scaling_fastdown_freq.attr,
	&scaling_fastdown_up_threshold.attr,
	&scaling_fastdown_down_threshold.attr,
	&scaling_responsiveness_freq.attr,
	&scaling_responsiveness_up_threshold.attr,
	&scaling_proportional.attr,
#ifdef ENABLE_HOTPLUGGING
	&up_threshold_hotplug1.attr,
	&up_threshold_hotplug_freq1.attr,
	&down_threshold_hotplug1.attr,
	&down_threshold_hotplug_freq1.attr,
#if (MAX_CORES == 4 || MAX_CORES == 8)
	&up_threshold_hotplug2.attr,
	&up_threshold_hotplug_freq2.attr,
	&down_threshold_hotplug2.attr,
	&down_threshold_hotplug_freq2.attr,
	&up_threshold_hotplug3.attr,
	&up_threshold_hotplug_freq3.attr,
	&down_threshold_hotplug3.attr,
	&down_threshold_hotplug_freq3.attr,
#endif
#if (MAX_CORES == 8)
	&up_threshold_hotplug4.attr,
	&up_threshold_hotplug_freq4.attr,
	&down_threshold_hotplug4.attr,
	&down_threshold_hotplug_freq4.attr,
	&up_threshold_hotplug5.attr,
	&up_threshold_hotplug_freq5.attr,
	&down_threshold_hotplug5.attr,
	&down_threshold_hotplug_freq5.attr,
	&up_threshold_hotplug6.attr,
	&up_threshold_hotplug_freq6.attr,
	&down_threshold_hotplug6.attr,
	&down_threshold_hotplug_freq6.attr,
	&up_threshold_hotplug7.attr,
	&up_threshold_hotplug_freq7.attr,
	&down_threshold_hotplug7.attr,
	&down_threshold_hotplug_freq7.attr,
#endif
#endif /* ENABLE_HOTPLUGGING */
	&dev_attr_version.attr,
	&dev_attr_version_profiles.attr,
	&dev_attr_profile_list.attr,
	&profile.attr,
	&profile_number.attr,
	&auto_adjust_freq_thresholds.attr,
#ifdef ZZMOOVE_DEBUG
	&dev_attr_debug.attr,
#endif
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "zzmoove",
};

/************************** sysfs end **************************/

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int j, load = 0, max_load = 0;
	struct cpufreq_policy *policy;

	boost_freq = false;					// ZZ: reset early demand boost freq flag
#ifdef ENABLE_HOTPLUGGING
	boost_hotplug = false;					// ZZ: reset early demand boost hotplug flag
#endif
	force_down_scaling = false;				// ZZ: reset force down scaling flag
	cancel_up_scaling = false;				// ZZ: reset cancel up scaling flag

	policy = this_dbs_info->cur_policy;
	cur_freq = policy->cur;			// Yank: store current frequency for hotplugging frequency thresholds

	/*
	 * Every sampling_rate, we check, if current idle time is less than 20%
	 * (default), then we try to increase frequency. Every sampling_rate *
	 * sampling_down_factor, we check, if current idle time is more than 80%
	 * (default), then we try to decrease frequency.
	 */

	/*
	 * ZZ: Get absolute load and make calcualtions for early demand, auto fast
	 * scaling and scaling block functionality
	 */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
		     &cur_wall_time, 0);
#else
		     &cur_wall_time);
#endif
#else
		     &cur_wall_time, 0);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		wall_time = (unsigned int)
				(cur_wall_time - j_dbs_info->prev_cpu_wall);
#else
		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
#endif
		j_dbs_info->prev_cpu_wall = cur_wall_time;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		idle_time = (unsigned int)

		(cur_idle_time - j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;
#else
		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;
#endif
		if (dbs_tuners_ins.ignore_nice) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		    u64 cur_nice;
#else
		    cputime64_t cur_nice;
#endif
		    unsigned long cur_nice_jiffies;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		    cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
				 j_dbs_info->prev_cpu_nice;
#else
		    cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
				 j_dbs_info->prev_cpu_nice);
#endif
		    /*
		     * Assumption: nice time between sampling periods will
		     * be less than 2^32 jiffies for 32 bit sys
		     */
		    cur_nice_jiffies = (unsigned long)
		    cputime64_to_jiffies64(cur_nice);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		    j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
#else
		    j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
#endif
		    idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		if (unlikely(!wall_time || wall_time < idle_time))
		    continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		if (load > max_load)
		    cur_load = max_load = load;		// ZZ: added static cur_load for hotplugging functions

		/*
		 * ZZ: Early demand by Stratosk
		 * Calculate the gradient of load. If it is too steep we assume
		 * that the load will go over up_threshold in next iteration(s) and
		 * we increase the frequency immediately
		 *
		 * At suspend:
		 * Seperate early demand for suspend to be able to adjust scaling behaving at screen off and therefore to be
		 * able to react problems which can occur because of too strictly suspend settings
		 * So this will: boost freq and switch to fast scaling mode 2 at the same time if load is steep enough
		 * (the value in grad_up_threshold_sleep) and in addition will lower the sleep multiplier to 2
		 * (if it was set higher) when load goes above the value in grad_up_threshold_sleep
		 */

		if (dbs_tuners_ins.early_demand && !suspend_flag) {

		    // ZZ: early demand at awake
		    if (max_load > this_dbs_info->prev_load && max_load - this_dbs_info->prev_load
			> dbs_tuners_ins.grad_up_threshold)
			boost_freq = true;
#ifdef ENABLE_HOTPLUGGING
			boost_hotplug = true;
#endif
		// ZZ: early demand at suspend
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		} else if (dbs_tuners_ins.early_demand_sleep && suspend_flag) {

			    // ZZ: check if we are over sleep threshold
			    if (max_load > dbs_tuners_ins.grad_up_threshold_sleep
				&& dbs_tuners_ins.sampling_rate_sleep_multiplier > 2)
				dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate_idle * 2;	// ZZ: lower sleep multiplier
			    else
			        dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate_idle
			        * dbs_tuners_ins.sampling_rate_sleep_multiplier;				// ZZ: restore sleep multiplier

			    // ZZ: if load is steep enough enable freq boost and fast up scaling
			    if (max_load > this_dbs_info->prev_load && max_load - this_dbs_info->prev_load
			        > dbs_tuners_ins.grad_up_threshold_sleep) {
			        boost_freq = true;								// ZZ: boost frequency
			        scaling_mode_up = 2;								// ZZ: enable fast scaling up mode 2
			    } else {
			        scaling_mode_up = 0;								// ZZ: disable fast scaling again
			    }
		}
#else
		}
#endif

		/*
		 * ZZ/Yank: Auto fast scaling mode
		 * Switch to all 4 fast scaling modes depending on load gradient
		 * the mode will start switching at given afs threshold load changes in both directions
		 */
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		if ((dbs_tuners_ins.fast_scaling_up       > 4 && !suspend_flag) ||
		    (dbs_tuners_ins.fast_scaling_sleep_up > 4 &&  suspend_flag)    ) {
#else
		if (dbs_tuners_ins.fast_scaling_up       > 4 && !suspend_flag) {
#endif
		    if (max_load > this_dbs_info->prev_load && max_load - this_dbs_info->prev_load <= dbs_tuners_ins.afs_threshold1) {
				scaling_mode_up = 0;
		    } else if (max_load - this_dbs_info->prev_load <= dbs_tuners_ins.afs_threshold2) {
				scaling_mode_up = 1;
		    } else if (max_load - this_dbs_info->prev_load <= dbs_tuners_ins.afs_threshold3) {
				scaling_mode_up = 2;
		    } else if (max_load - this_dbs_info->prev_load <= dbs_tuners_ins.afs_threshold4) {
				scaling_mode_up = 3;
		    } else {
				scaling_mode_up = 4;
		    }
		}

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		if ((dbs_tuners_ins.fast_scaling_down       > 4 && !suspend_flag) ||
		    (dbs_tuners_ins.fast_scaling_sleep_down > 4 &&  suspend_flag)    ) {
#else
		if (dbs_tuners_ins.fast_scaling_down       > 4 && !suspend_flag) {
#endif
		  if (max_load < this_dbs_info->prev_load && this_dbs_info->prev_load - max_load <= dbs_tuners_ins.afs_threshold1) {
				scaling_mode_down = 0;
		    } else if (this_dbs_info->prev_load - max_load <= dbs_tuners_ins.afs_threshold2) {
				scaling_mode_down = 1;
		    } else if (this_dbs_info->prev_load - max_load <= dbs_tuners_ins.afs_threshold3) {
				scaling_mode_down = 2;
		    } else if (this_dbs_info->prev_load - max_load <= dbs_tuners_ins.afs_threshold4) {
				scaling_mode_down = 3;
		    } else {
				scaling_mode_down = 4;
		    }
		}

		/*
		 * ZZ: Scaling block for reducing up scaling 'overhead'
		 *
		 * If the given freq threshold is reached do following:
		 * Calculate the gradient of load in both directions count them every time they are under the load threshold
		 * and block up scaling during that time. If max count of cycles (and therefore threshold hits) are reached
		 * switch to 'force down mode' which lowers the freq the next given block cycles. By all that we can avoid
		 * 'sticking' on max or relatively high frequency (caused by the very fast scaling behaving of zzmoove)
		 * when load is constantly on mid to higher load during a 'longer' peroid.
		 *
		 * Or if exynos4 CPU temperature reading is enabled below do following:
		 * Use current CPU temperature as a blocking threshold to lower the frequency and therefore keep the CPU cooler.
		 * so in particular this will lower the frequency to the frequency set in 'scaling_block_freq' and hold it
		 * there till the temperature goes under the temperature threshold again.
		 *
		 * u can choose here to use either fixed blocking cycles or the temperature threshold. using fixed blocking cycles disables
		 * temperature depending blocking. in case of temperature depending blocks u must set a target freq in scaling_block_freq
		 * tuneable. fixed block cycle feature can still be used optional without a frequency as 'starting threshold' like before
		 */

#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
		if (dbs_tuners_ins.scaling_block_cycles == 0 && dbs_tuners_ins.scaling_block_freq != 0
		    && cpu_temp >= dbs_tuners_ins.scaling_block_temp && !suspend_flag) {
		    if (policy->cur == dbs_tuners_ins.scaling_block_freq) {
			cancel_up_scaling = true;
#ifdef ENABLE_HOTPLUGGING
			hotplug_up_temp_block = true;
#endif
		    }
		    if (policy->cur > dbs_tuners_ins.scaling_block_freq || policy->cur == policy->max) {
			scaling_mode_down = 0;	// ZZ: if fast scaling down was enabled disable it to be sure that block freq will be met
			force_down_scaling = true;
		    }
		}
#endif
		// ZZ: start blocking if we are not at suspend freq threshold is reached and max load is not at maximum
		if (dbs_tuners_ins.scaling_block_cycles != 0 && policy->cur >= dbs_tuners_ins.scaling_block_freq
		    && !suspend_flag && max_load != 100) {

		    // ZZ: depending on load threshold count the gradients and block up scaling till max cycles are reached
		    if ((scaling_block_cycles_count <= dbs_tuners_ins.scaling_block_cycles && max_load > this_dbs_info->prev_load
			&& max_load - this_dbs_info->prev_load >= dbs_tuners_ins.scaling_block_threshold) ||
			(scaling_block_cycles_count <= dbs_tuners_ins.scaling_block_cycles && max_load < this_dbs_info->prev_load
			&& this_dbs_info->prev_load - max_load >= dbs_tuners_ins.scaling_block_threshold) ||
			dbs_tuners_ins.scaling_block_threshold == 0) {
			scaling_block_cycles_count++;							// ZZ: count gradients
			cancel_up_scaling = true;							// ZZ: block scaling up at the same time
		    }

		    // ZZ: then switch to 'force down mode'
		    if (scaling_block_cycles_count == dbs_tuners_ins.scaling_block_cycles) {		// ZZ: amount of cycles is reached
			if (dbs_tuners_ins.scaling_block_force_down != 0)
			    scaling_block_cycles_count = dbs_tuners_ins.scaling_block_cycles		// ZZ: switch to force down mode if enabled
			    * dbs_tuners_ins.scaling_block_force_down;
		        else
			    scaling_block_cycles_count = 0;						// ZZ: down force disabled start from scratch
		    }

		    // ZZ: and force down scaling during next given bock cycles
		    if (scaling_block_cycles_count > dbs_tuners_ins.scaling_block_cycles) {
			if (unlikely(--scaling_block_cycles_count > dbs_tuners_ins.scaling_block_cycles))
			    force_down_scaling = true;							// ZZ: force down scaling
			else
			    scaling_block_cycles_count = 0;						// ZZ: done -> reset counter
		    }

		}

		// ZZ: used for gradient load calculation in fast scaling, scaling block and early demand
		if (dbs_tuners_ins.early_demand || dbs_tuners_ins.scaling_block_cycles != 0
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
		  || dbs_tuners_ins.fast_scaling_up > 4 || dbs_tuners_ins.fast_scaling_down > 4 || (dbs_tuners_ins.early_demand_sleep && !suspend_flag))
		    this_dbs_info->prev_load = max_load;
#else
		  || dbs_tuners_ins.fast_scaling_up > 4 || dbs_tuners_ins.fast_scaling_down > 4)
		    this_dbs_info->prev_load = max_load;
#endif
	}

#ifdef ENABLE_HOTPLUGGING
	// ZZ: if hotplug idle threshold is reached and cpu frequency is at its minimum disable hotplug
	if (policy->cur < dbs_tuners_ins.hotplug_idle_freq && max_load < dbs_tuners_ins.hotplug_idle_threshold
	    && dbs_tuners_ins.hotplug_idle_threshold != 0 && !suspend_flag)
	    hotplug_idle_flag = true;
	else
	    hotplug_idle_flag = false;

	// ZZ: block cycles to be able to slow down hotplugging - added hotplug enagage freq (ffolkes)
	if (((!dbs_tuners_ins.disable_hotplug && num_online_cpus() != possible_cpus) || hotplug_idle_flag)
	    && (!dbs_tuners_ins.hotplug_engage_freq || policy->cur >= dbs_tuners_ins.hotplug_engage_freq)) {
	    if (hplg_up_block_cycles > dbs_tuners_ins.hotplug_block_up_cycles
		|| (!hotplug_up_in_progress && dbs_tuners_ins.hotplug_block_up_cycles == 0)) {
		    queue_work_on(policy->cpu, dbs_wq, &hotplug_online_work);
		if (dbs_tuners_ins.hotplug_block_up_cycles != 0)
		    hplg_up_block_cycles = 0;
	    }
	    if (dbs_tuners_ins.hotplug_block_up_cycles != 0)
		hplg_up_block_cycles++;
	}
#endif
	// ZZ: Sampling rate idle
	if (dbs_tuners_ins.sampling_rate_idle != dbs_tuners_ins.sampling_rate
	    && max_load > dbs_tuners_ins.sampling_rate_idle_threshold
	    && !suspend_flag && dbs_tuners_ins.sampling_rate_current != dbs_tuners_ins.sampling_rate) {
	    if (sampling_rate_step_up_delay >= dbs_tuners_ins.sampling_rate_idle_delay) {
	        dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate;
	        if (dbs_tuners_ins.sampling_rate_idle_delay != 0)
		    sampling_rate_step_up_delay = 0;
	    }
	    if (dbs_tuners_ins.sampling_rate_idle_delay != 0)
	        sampling_rate_step_up_delay++;
	}

	// ZZ: Scaling fastdown and responsiveness thresholds (ffolkes)
	if (!suspend_flag && dbs_tuners_ins.scaling_fastdown_freq && policy->cur > dbs_tuners_ins.scaling_fastdown_freq) {
	    scaling_up_threshold = dbs_tuners_ins.scaling_fastdown_up_threshold;
	} else if (!suspend_flag && dbs_tuners_ins.scaling_responsiveness_freq && policy->cur < dbs_tuners_ins.scaling_responsiveness_freq) {
	    scaling_up_threshold = dbs_tuners_ins.scaling_responsiveness_up_threshold;
	} else {
	    scaling_up_threshold = dbs_tuners_ins.up_threshold;
	}

	// Check for frequency increase
	if ((max_load >= scaling_up_threshold || boost_freq)		// ZZ: boost switch for early demand and scaling block switches added
	    && !cancel_up_scaling && !force_down_scaling) {

	    // ZZ: Sampling down momentum - if momentum is inactive switch to 'down_skip' method
	    if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1)
		this_dbs_info->down_skip = 0;

	    // ZZ: Frequency Limit: if we are at freq_limit break out early
	    if (dbs_tuners_ins.freq_limit != 0 && policy->cur == dbs_tuners_ins.freq_limit)
		return;

	    // if we are already at full speed then break out early
	    if (policy->cur == policy->max)				// ZZ: changed check from reqested_freq to current freq (DerTeufel1980)
		return;

	    // ZZ: Sampling down momentum - if momentum is active and we are switching to max speed, apply sampling_down_factor
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && policy->cur < policy->max)
		this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;

		this_dbs_info->requested_freq = zz_get_next_freq(policy->cur, 1, max_load);

	    if (dbs_tuners_ins.freq_limit != 0 && this_dbs_info->requested_freq
		> dbs_tuners_ins.freq_limit)
		this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			    CPUFREQ_RELATION_H);

	    // ZZ: Sampling down momentum - calculate momentum and update sampling down factor
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder
		< dbs_tuners_ins.sampling_down_mom_sens) {
		this_dbs_info->momentum_adder++;
		dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder
		* dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
		dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor
		+ dbs_tuners_ins.sampling_down_momentum;
	    }
	    return;
	}

#ifdef ENABLE_HOTPLUGGING
	// ZZ: block cycles to be able to slow down hotplugging
	if (!dbs_tuners_ins.disable_hotplug && num_online_cpus() != 1 && !hotplug_idle_flag) {
	    if (unlikely(hplg_down_block_cycles > dbs_tuners_ins.hotplug_block_down_cycles)
		|| (!hotplug_down_in_progress && dbs_tuners_ins.hotplug_block_down_cycles == 0)) {
		    queue_work_on(policy->cpu, dbs_wq, &hotplug_offline_work);
		if (dbs_tuners_ins.hotplug_block_down_cycles != 0)
		    hplg_down_block_cycles = 0;
	    }
	    if (dbs_tuners_ins.hotplug_block_down_cycles != 0)
		hplg_down_block_cycles++;
	}
#endif

	// ZZ: Sampling down momentum - if momentum is inactive switch to down skip method and if sampling_down_factor is active break out early
	if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1) {
	    if (++this_dbs_info->down_skip < dbs_tuners_ins.sampling_down_factor)
		return;
	    this_dbs_info->down_skip = 0;
	}

	// ZZ: Sampling down momentum - calculate momentum and update sampling down factor
	if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder > 1) {
	    this_dbs_info->momentum_adder -= 2;
	    dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder
	    * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
	    dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor
	    + dbs_tuners_ins.sampling_down_momentum;
	}

	// ZZ: Sampling rate idle
	if (dbs_tuners_ins.sampling_rate_idle != dbs_tuners_ins.sampling_rate
	    && max_load < dbs_tuners_ins.sampling_rate_idle_threshold && !suspend_flag
	    && dbs_tuners_ins.sampling_rate_current != dbs_tuners_ins.sampling_rate_idle) {
	    if (sampling_rate_step_down_delay >= dbs_tuners_ins.sampling_rate_idle_delay) {
		dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate_idle;
		if (dbs_tuners_ins.sampling_rate_idle_delay != 0)
		    sampling_rate_step_down_delay = 0;
	    }
	    if (dbs_tuners_ins.sampling_rate_idle_delay != 0)
		sampling_rate_step_down_delay++;
	}

	// ZZ: Scaling fastdown threshold (ffolkes)
	if (!suspend_flag && dbs_tuners_ins.scaling_fastdown_freq != 0 && policy->cur > dbs_tuners_ins.scaling_fastdown_freq)
	    scaling_down_threshold = dbs_tuners_ins.scaling_fastdown_down_threshold;
	else
	    scaling_down_threshold = dbs_tuners_ins.down_threshold;

	// Check for frequency decrease
	if (max_load < scaling_down_threshold || force_down_scaling) {				// ZZ: added force down switch

		// ZZ: Sampling down momentum - no longer fully busy, reset rate_mult
		this_dbs_info->rate_mult = 1;

		// if we cannot reduce the frequency anymore, break out early
		if (policy->cur == policy->min)
		    return;

		this_dbs_info->requested_freq = zz_get_next_freq(policy->cur, 2, max_load);

		if (dbs_tuners_ins.freq_limit != 0 && this_dbs_info->requested_freq
		    > dbs_tuners_ins.freq_limit)
		    this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

		    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			    CPUFREQ_RELATION_L);							// ZZ: changed to relation low
		return;
	}
}

/**
 * ZZMoove hotplugging by Zane Zaminsky 2012/13/14 improved mainly
 * for Samsung I9300 devices but compatible with others too:
 *
 * ZZMoove v0.1		- Modification by ZaneZam November 2012
 *			  Check for frequency increase is greater than hotplug up threshold value and wake up cores accordingly
 *			  Following will bring up 3 cores in a row (cpu0 stays always on!)
 *
 * ZZMoove v0.2		- changed hotplug logic to be able to tune up threshold per core and to be able to set
 *			  cores offline manually via sysfs
 *
 * ZZMoove v0.5		- fixed non switching cores at 0+2 and 0+3 situations
 *			- optimized hotplug logic by removing locks and skipping hotplugging if not needed
 *			- try to avoid deadlocks at critical events by using a flag if we are in the middle of hotplug decision
 *
 * ZZMoove v0.5.1b	- optimised hotplug logic by reducing code and concentrating only on essential parts
 *			- preperation for automatic core detection
 *
 * ZZMoove v0.6		- reduced hotplug loop to a minimum and use seperate functions out of dbs_check_cpu for hotplug work (credits to ktoonsez)
 *
 * ZZMoove v0.7		- added legacy mode for enabling the 'old way of hotplugging' from versions 0.4/0.5
 *                      - added hotplug idle threshold for automatic disabling of hotplugging when CPU idles
 *                        (balanced cpu load might bring cooler cpu at that state - inspired by JustArchis observations, thx!)
 *                      - added hotplug block cycles to reduce hotplug overhead (credits to ktoonesz)
 *                      - added hotplug frequency thresholds (credits to Yank555)
 *
 * ZZMoove v0.7a	- fixed a glitch in hotplug freq threshold tuneables
 *
 * ZZMoove v0.7d	- fixed hotplug up threshold tuneables to be able again to disable cores manually via sysfs by setting them to 0
 *			- fixed the problem caused by a 'wrong' tuneable apply order of non sticking values in hotplug down threshold tuneables
 *			- fixed a typo in hotplug threshold tuneable macros (would have been only a issue in 8-core mode)
 *			- fixed unwanted disabling of cores when setting hotplug threshold tuneables to lowest or highest possible value
 *
 * ZZMoove v0.8		- fixed not working hotplugging when freq->max undercuts one ore more hotplug freq thresholds
 *			- removed all no longer required hotplug skip flags
 *			- added freq thresholds to legacy hotplugging (same usage as in normal hotlpugging mode)
 *			- improved hotplugging by avoiding calls to hotplug functions if hotlpug is currently active
 *			  and by using different function in hotplug up loop and removing external function calls in down loop
 *
 * ZZMoove v0.9 beta3	- added hotplug_engage_freq to reduce unnecessary cores online at low loads (credits to ffolkes)
 *
 * ZZMoove v0.9 beta4	- added avoiding of hotplug online work during scaling temp blocking if exynos4 CPU temperature reading support
 *			  is enabled in code.
 *
 * ZZMoove v1.0 beta1	- added macros to exclude hotplugging functionality (default in this version is disabled=commented for
 *			  opo devices because of mpdecision in general and unmotivated kernel crashes when using it without mpdecision)
 *			  in addition disabled hopplugging per default if hotplugging code is used to avoid accetential usage of mpdecision
 *			  and buildin governor hotplugging at the same time
 */

// ZZ: function for hotplug down work
#ifdef ENABLE_HOTPLUGGING
static void __cpuinit hotplug_offline_work_fn(struct work_struct *work)
{
	int cpu;	// ZZ: for hotplug down loop

	hotplug_down_in_progress = true;

	    // Yank: added frequency thresholds
	    for_each_online_cpu(cpu) {
	    if (likely(cpu_online(cpu) && (cpu)) && cpu != 0
		&& cur_load <= hotplug_thresholds[1][cpu-1]
		&& (hotplug_thresholds_freq[1][cpu-1] == 0
		|| cur_freq <= hotplug_thresholds_freq[1][cpu-1]
		|| max_freq_too_low))
		cpu_down(cpu);
	    }

	hotplug_down_in_progress = false;
}

// ZZ: function for hotplug up work
static void __cpuinit hotplug_online_work_fn(struct work_struct *work)
{
	int i = 0;	// ZZ: for hotplug up loop

	hotplug_up_in_progress = true;

#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	if (hotplug_up_temp_block) {
	    hotplug_up_temp_block = false;
	    hotplug_up_in_progress = false;
	    return;
	}
#endif

	/*
	 * ZZ: hotplug idle flag to enable offline cores on idle to avoid higher/achieve balanced cpu load at idle
	 * and enable cores flag to enable offline cores on governor stop and at late resume
	 */
	if (unlikely(hotplug_idle_flag || enable_cores)){
	    enable_offline_cores();
	    hotplug_up_in_progress = false;
	    return;
	}

	    // Yank: added frequency thresholds
	    for (i = 1; likely(i < possible_cpus); i++) {
		if (!cpu_online(i) && hotplug_thresholds[0][i-1] != 0 && cur_load >= hotplug_thresholds[0][i-1]
		    && (hotplug_thresholds_freq[0][i-1] == 0 || cur_freq >= hotplug_thresholds_freq[0][i-1]
		    || boost_hotplug || max_freq_too_low))
		    cpu_up(i);
	    }

	hotplug_up_in_progress = false;
}
#endif

#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
// ZZ: function for CPU temperature reading
static void tmu_read_temperature(struct work_struct * tmu_read_work)
{
	cpu_temp = get_exynos4_temperature();
	return;
}
#endif

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	// We want all CPUs to do sampling nearly on same jiffy
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate_current * dbs_info->rate_mult); // ZZ: Sampling down momentum - added multiplier

#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	// ZZ: start reading of temperature from exynos4 thermal management driver but disable it at suspend
	if (dbs_tuners_ins.scaling_block_temp != 0) {						// ZZ: only if it is enabled and we are not at suspend
	    if (!suspend_flag) {
		schedule_delayed_work(&tmu_read_work, msecs_to_jiffies(DEF_TMU_READ_DELAY));	// ZZ: start work
		temp_reading_started = true;							// ZZ: set work started flag
		cancel_temp_reading = false;							// ZZ: reset cancel flag
	    } else {
		cancel_temp_reading = true;							// ZZ: else set cancel flag
	    }

	    if (temp_reading_started && cancel_temp_reading) {					// ZZ: if work was started and cancel flag was set
		cancel_delayed_work(&tmu_read_work);						// ZZ: cancel work
		cancel_temp_reading = false;							// ZZ: reset cancel flag
		temp_reading_started = false;							// ZZ: reset started flag
	    }
	}

	if (dbs_tuners_ins.scaling_block_temp == 0 && temp_reading_started)			// ZZ: if temp reading was disabled via sysfs and work was started
	    cancel_delayed_work(&tmu_read_work);						// ZZ: cancel work
#endif
	delay -= jiffies % delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);

	queue_delayed_work_on(cpu, dbs_wq, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	// We want all CPUs to do sampling nearly on same jiffy
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate_current);
	delay -= jiffies % delay;

	dbs_info->enable = 1;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	queue_delayed_work_on(dbs_info->cpu, dbs_wq, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	dbs_info->enable = 0;
	cancel_delayed_work_sync(&dbs_info->work);
#ifdef CONFIG_EXYNOS4_EXPORT_TEMP
	cancel_delayed_work(&tmu_read_work);		// ZZ: cancel cpu temperature reading
#endif
}

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
// raise sampling rate to SR*multiplier and adjust sampling rate/thresholds/hotplug/scaling/freq limit/freq step on blank screen
#if defined(CONFIG_HAS_EARLYSUSPEND)
static void __cpuinit powersave_early_suspend(struct early_suspend *handler)
#elif defined(CONFIG_POWERSUSPEND)
static void __cpuinit powersave_suspend(struct power_suspend *handler)
#elif defined(CONFIG_BACKLIGHT_EXT_CONTROL)
extern void zzmoove_suspend(void)
#endif
{
	suspend_flag = true;				// ZZ: we want to know if we are at suspend because of things that shouldn't be executed at suspend
	sampling_rate_awake = dbs_tuners_ins.sampling_rate_current;		// ZZ: save current sampling rate for restore on awake
	up_threshold_awake = dbs_tuners_ins.up_threshold;			// ZZ: save up threshold for restore on awake
	down_threshold_awake = dbs_tuners_ins.down_threshold;			// ZZ: save down threhold for restore on awake
	dbs_tuners_ins.sampling_down_max_mom = 0;				// ZZ: sampling down momentum - disabled at suspend
	smooth_up_awake = dbs_tuners_ins.smooth_up;				// ZZ: save smooth up value for restore on awake
	fast_scaling_up_awake = dbs_tuners_ins.fast_scaling_up;			// Yank: save scaling setting for restore on awake for upscaling
	fast_scaling_down_awake = dbs_tuners_ins.fast_scaling_down;		// Yank: save scaling setting for restore on awake for downscaling
#ifdef ENABLE_HOTPLUGGING
	disable_hotplug_awake = dbs_tuners_ins.disable_hotplug;			// ZZ: save hotplug switch state for restore on awake

	if (likely(dbs_tuners_ins.hotplug_sleep != 0)) {			// ZZ: if set to 0 do not touch hotplugging values
	    hotplug1_awake = dbs_tuners_ins.up_threshold_hotplug1;		// ZZ: save hotplug1 value for restore on awake
#if (MAX_CORES == 4 || MAX_CORES == 8)
	    hotplug2_awake = dbs_tuners_ins.up_threshold_hotplug2;		// ZZ: save hotplug2 value for restore on awake
	    hotplug3_awake = dbs_tuners_ins.up_threshold_hotplug3;		// ZZ: save hotplug3 value for restore on awake
#endif
#if (MAX_CORES == 8)
	    hotplug4_awake = dbs_tuners_ins.up_threshold_hotplug4;		// ZZ: save hotplug4 value for restore on awake
	    hotplug5_awake = dbs_tuners_ins.up_threshold_hotplug5;		// ZZ: save hotplug5 value for restore on awake
	    hotplug6_awake = dbs_tuners_ins.up_threshold_hotplug6;		// ZZ: save hotplug6 value for restore on awake
	    hotplug7_awake = dbs_tuners_ins.up_threshold_hotplug7;		// ZZ: save hotplug7 value for restore on awake
#endif
	}
#endif
	sampling_rate_asleep = dbs_tuners_ins.sampling_rate_sleep_multiplier;	// ZZ: save sleep multiplier for sleep
	up_threshold_asleep = dbs_tuners_ins.up_threshold_sleep;		// ZZ: save up threshold for sleep
	down_threshold_asleep = dbs_tuners_ins.down_threshold_sleep;		// ZZ: save down threshold for sleep
	smooth_up_asleep = dbs_tuners_ins.smooth_up_sleep;			// ZZ: save smooth up for sleep
	fast_scaling_up_asleep = dbs_tuners_ins.fast_scaling_sleep_up;		// Yank: save fast scaling for sleep for upscaling
	fast_scaling_down_asleep = dbs_tuners_ins.fast_scaling_sleep_down;	// Yank: save fast scaling for sleep for downscaling
#ifdef ENABLE_HOTPLUGGING
	disable_hotplug_asleep = dbs_tuners_ins.disable_hotplug_sleep;		// ZZ: save disable hotplug switch for sleep
#endif
	dbs_tuners_ins.sampling_rate_current = dbs_tuners_ins.sampling_rate_idle
	* sampling_rate_asleep;							// ZZ: set sampling rate for sleep
	dbs_tuners_ins.up_threshold = up_threshold_asleep;			// ZZ: set up threshold for sleep
	dbs_tuners_ins.down_threshold = down_threshold_asleep;			// ZZ: set down threshold for sleep
	dbs_tuners_ins.smooth_up = smooth_up_asleep;				// ZZ: set smooth up for for sleep
	dbs_tuners_ins.freq_limit = freq_limit_asleep;				// ZZ: set freqency limit for sleep
	dbs_tuners_ins.fast_scaling_up = fast_scaling_up_asleep;		// Yank: set fast scaling for sleep for upscaling
	dbs_tuners_ins.fast_scaling_down = fast_scaling_down_asleep;		// Yank: set fast scaling for sleep for downscaling
#ifdef ENABLE_HOTPLUGGING
	dbs_tuners_ins.disable_hotplug = disable_hotplug_asleep;		// ZZ: set hotplug switch for sleep
#endif
	evaluate_scaling_order_limit_range(0, 0, suspend_flag, 0);		// ZZ: table order detection and limit optimizations
#ifdef ENABLE_HOTPLUGGING
	if (dbs_tuners_ins.disable_hotplug_sleep) {				// ZZ: enable all cores at suspend if disable hotplug sleep is set
	    enable_cores = true;
	    queue_work_on(0, dbs_wq, &hotplug_online_work);
	}
#endif
	if (dbs_tuners_ins.fast_scaling_up > 4)					// Yank: set scaling mode
	    scaling_mode_up   = 0;						// ZZ: auto fast scaling
	else
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling_up;			// Yank: fast scaling up only

	if (dbs_tuners_ins.fast_scaling_down > 4)				// Yank: set scaling mode
	    scaling_mode_down = 0;						// ZZ: auto fast scaling
	else
	    scaling_mode_down = dbs_tuners_ins.fast_scaling_down;		// Yank: fast scaling up only
#ifdef ENABLE_HOTPLUGGING
	if (likely(dbs_tuners_ins.hotplug_sleep != 0)) {			// ZZ: if set to 0 do not touch hotplugging values
	    if (dbs_tuners_ins.hotplug_sleep == 1) {
		dbs_tuners_ins.up_threshold_hotplug1 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][0] = 0;
#if (MAX_CORES == 4 || MAX_CORES == 8)
		dbs_tuners_ins.up_threshold_hotplug2 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][1] = 0;
		dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][2] = 0;
#endif
#if (MAX_CORES == 8)
		dbs_tuners_ins.up_threshold_hotplug4 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][3] = 0;
		dbs_tuners_ins.up_threshold_hotplug5 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][4] = 0;
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to one core
		hotplug_thresholds[0][6] = 0;
#endif
	    }
#if (MAX_CORES == 4 || MAX_CORES == 8)
	    if (dbs_tuners_ins.hotplug_sleep == 2) {
		dbs_tuners_ins.up_threshold_hotplug2 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][1] = 0;
		dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][2] = 0;
#endif
#if (MAX_CORES == 8)
		dbs_tuners_ins.up_threshold_hotplug4 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][3] = 0;
		dbs_tuners_ins.up_threshold_hotplug5 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][4] = 0;
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to two cores
		hotplug_thresholds[0][6] = 0;
#endif
#if (MAX_CORES == 4 || MAX_CORES == 8)
	    }
	    if (dbs_tuners_ins.hotplug_sleep == 3) {
		dbs_tuners_ins.up_threshold_hotplug3 = 0;			// ZZ: set to three cores
	        hotplug_thresholds[0][2] = 0;
#endif
#if (MAX_CORES == 8)
		dbs_tuners_ins.up_threshold_hotplug4 = 0;			// ZZ: set to three cores
		hotplug_thresholds[0][3] = 0;
		dbs_tuners_ins.up_threshold_hotplug5 = 0;			// ZZ: set to three cores
		hotplug_thresholds[0][4] = 0;
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to three cores
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to three cores
		hotplug_thresholds[0][6] = 0;
#endif
#if (MAX_CORES == 4 || MAX_CORES == 8)
	    }
#endif
#if (MAX_CORES == 8)
	    if (dbs_tuners_ins.hotplug_sleep == 4) {
		dbs_tuners_ins.up_threshold_hotplug4 = 0;			// ZZ: set to four cores
		hotplug_thresholds[0][3] = 0;
		dbs_tuners_ins.up_threshold_hotplug5 = 0;			// ZZ: set to four cores
		hotplug_thresholds[0][4] = 0;
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to four cores
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to four cores
		hotplug_thresholds[0][6] = 0;
	    }
	    if (dbs_tuners_ins.hotplug_sleep == 5) {
		dbs_tuners_ins.up_threshold_hotplug5 = 0;			// ZZ: set to five cores
		hotplug_thresholds[0][4] = 0;
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to five cores
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to five cores
		hotplug_thresholds[0][6] = 0;
	    }
	    if (dbs_tuners_ins.hotplug_sleep == 6) {
		dbs_tuners_ins.up_threshold_hotplug6 = 0;			// ZZ: set to six cores
		hotplug_thresholds[0][5] = 0;
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to six cores
		hotplug_thresholds[0][6] = 0;
	    }
	    if (dbs_tuners_ins.hotplug_sleep == 7) {
		dbs_tuners_ins.up_threshold_hotplug7 = 0;			// ZZ: set to seven cores
		hotplug_thresholds[0][6] = 0;
	    }
#endif
	}
#endif /* ENABLE_HOTPLUGGING */
}
#if defined(CONFIG_BACKLIGHT_EXT_CONTROL)
EXPORT_SYMBOL(zzmoove_suspend);
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void __cpuinit powersave_late_resume(struct early_suspend *handler)
#elif defined(CONFIG_POWERSUSPEND)
static void __cpuinit powersave_resume(struct power_suspend *handler)
#elif defined(CONFIG_BACKLIGHT_EXT_CONTROL)
extern void zzmoove_resume(void)
#endif
{
	suspend_flag = false;					// ZZ: we are resuming so reset supend flag
	scaling_mode_up = 4;					// ZZ: scale up as fast as possibe
	boost_freq = true;					// ZZ: and boost freq in addition

#ifdef ENABLE_HOTPLUGGING
	if (likely(!dbs_tuners_ins.disable_hotplug_sleep)) {
	    enable_cores = true;
	    queue_work_on(0, dbs_wq, &hotplug_online_work);	// ZZ: enable offline cores to avoid stuttering after resume if hotplugging limit was active
	}

	if (likely(dbs_tuners_ins.hotplug_sleep != 0)) {
	    dbs_tuners_ins.up_threshold_hotplug1 = hotplug1_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][0] = hotplug1_awake;
#if (MAX_CORES == 4 || MAX_CORES == 8)
	    dbs_tuners_ins.up_threshold_hotplug2 = hotplug2_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][1] = hotplug2_awake;
	    dbs_tuners_ins.up_threshold_hotplug3 = hotplug3_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][2] = hotplug3_awake;
#endif
#if (MAX_CORES == 8)
	    dbs_tuners_ins.up_threshold_hotplug4 = hotplug4_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][3] = hotplug4_awake;
	    dbs_tuners_ins.up_threshold_hotplug5 = hotplug5_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][4] = hotplug5_awake;
	    dbs_tuners_ins.up_threshold_hotplug6 = hotplug6_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][5] = hotplug6_awake;
	    dbs_tuners_ins.up_threshold_hotplug7 = hotplug7_awake;		// ZZ: restore previous settings
	    hotplug_thresholds[0][6] = hotplug7_awake;
#endif
	}
#endif
	dbs_tuners_ins.sampling_down_max_mom = orig_sampling_down_max_mom;	// ZZ: Sampling down momentum - restore max value
	dbs_tuners_ins.sampling_rate_current = sampling_rate_awake;		// ZZ: restore previous settings
	dbs_tuners_ins.up_threshold = up_threshold_awake;			// ZZ: restore previous settings
	dbs_tuners_ins.down_threshold = down_threshold_awake;			// ZZ: restore previous settings
	dbs_tuners_ins.smooth_up = smooth_up_awake;				// ZZ: restore previous settings
	dbs_tuners_ins.freq_limit = freq_limit_awake;				// ZZ: restore previous settings
	dbs_tuners_ins.fast_scaling_up   = fast_scaling_up_awake;		// Yank: restore previous settings for upscaling
	dbs_tuners_ins.fast_scaling_down = fast_scaling_down_awake;		// Yank: restore previous settings for downscaling
#ifdef ENABLE_HOTPLUGGING
	dbs_tuners_ins.disable_hotplug = disable_hotplug_awake;			// ZZ: restore previous settings
#endif
	evaluate_scaling_order_limit_range(0, 0, suspend_flag, 0);		// ZZ: table order detection and limit optimizations

	if (dbs_tuners_ins.fast_scaling_up > 4)					// Yank: set scaling mode
	    scaling_mode_up   = 0;						// ZZ: auto fast scaling
	else
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling_up;			// Yank: fast scaling up only

	if (dbs_tuners_ins.fast_scaling_down > 4)				// Yank: set scaling mode
	    scaling_mode_down = 0;						// ZZ: auto fast scaling
	else
	    scaling_mode_down = dbs_tuners_ins.fast_scaling_down;		// Yank: fast scaling up only
}
#if defined(CONFIG_BACKLIGHT_EXT_CONTROL)
EXPORT_SYMBOL(zzmoove_resume);
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
static struct early_suspend __refdata _powersave_early_suspend = {
  .suspend = powersave_early_suspend,
  .resume = powersave_late_resume,
  .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};
#elif defined(CONFIG_POWERSUSPEND)
static struct power_suspend __refdata powersave_powersuspend = {
  .suspend = powersave_suspend,
  .resume = powersave_resume,
};
#endif
#endif

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;
#ifdef ENABLE_HOTPLUGGING
	int i = 0;
#endif
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
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
#ifdef CPU_IDLE_TIME_IN_CPUFREQ			/* overrule for sources with backported cpufreq implementation */
			&j_dbs_info->prev_cpu_wall, 0);
#else
			&j_dbs_info->prev_cpu_wall);
#endif
#else
			&j_dbs_info->prev_cpu_wall, 0);
#endif
			if (dbs_tuners_ins.ignore_nice) {
			    j_dbs_info->prev_cpu_nice =
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
			    kcpustat_cpu(j).cpustat[CPUTIME_NICE];
#else
			    kstat_cpu(j).cpustat.nice;
#endif
			}
			j_dbs_info->time_in_idle = get_cpu_idle_time_us(cpu, &j_dbs_info->idle_exit_time); // ZZ: idle exit time handling
		}
		this_dbs_info->cpu = cpu;					// ZZ: initialise the cpu field during governor start
		this_dbs_info->rate_mult = 1;					// ZZ: sampling down momentum - reset multiplier
		this_dbs_info->momentum_adder = 0;				// ZZ: sampling down momentum - reset momentum adder
		this_dbs_info->down_skip = 0;					// ZZ: sampling down - reset down_skip
		this_dbs_info->requested_freq = policy->cur;

		// ZZ: get freq table available cpus for hotplugging and optimize/detect scaling range
#ifdef ENABLE_HOTPLUGGING
		possible_cpus = num_possible_cpus();
#endif
		freq_init_count = 0;						// ZZ: reset init flag for governor reload
		system_freq_table = cpufreq_frequency_get_table(0);		// ZZ: update static system frequency table
		evaluate_scaling_order_limit_range(1, 0, 0, policy->max);	// ZZ: table order detection and limit optimizations
#ifdef ENABLE_HOTPLUGGING
		// ZZ: save default values in threshold array
		for (i = 0; i < possible_cpus; i++) {
		    hotplug_thresholds[0][i] = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG;
		    hotplug_thresholds[1][i] = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG;
		}
#endif
		mutex_init(&this_dbs_info->timer_mutex);
		dbs_enable++;

		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
		    unsigned int latency;
		    // policy latency is in nS. Convert it to uS first
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
			// Bring kernel and HW constraints together
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate_current =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
#if (DEF_PROFILE_NUMBER > 0)
			set_profile(DEF_PROFILE_NUMBER);
#endif
			// ZZ: Sampling down momentum - set down factor and max momentum
			orig_sampling_down_factor = dbs_tuners_ins.sampling_down_factor;
			orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
			sampling_rate_awake = dbs_tuners_ins.sampling_rate
			= dbs_tuners_ins.sampling_rate_current;
#else
			dbs_tuners_ins.sampling_rate
			= dbs_tuners_ins.sampling_rate_current;
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_POWERSUSPEND) || defined(CONFIG_BACKLIGHT_EXT_CONTROL)
			up_threshold_awake = dbs_tuners_ins.up_threshold;
			down_threshold_awake = dbs_tuners_ins.down_threshold;
			smooth_up_awake = dbs_tuners_ins.smooth_up;
#endif
			// ZZ: switch to proportional scaling if we didn't get system freq table
			if (!system_freq_table)
			    printk(KERN_ERR "[zzmoove] Failed to get system freq table! falling back to proportional scaling!\n");
			    dbs_tuners_ins.scaling_proportional = 2;

			cpufreq_register_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
		}
		mutex_unlock(&dbs_mutex);
		dbs_timer_init(this_dbs_info);
#if defined(CONFIG_HAS_EARLYSUSPEND)
		register_early_suspend(&_powersave_early_suspend);
#elif defined(CONFIG_POWERSUSPEND)
		register_power_suspend(&powersave_powersuspend);
#endif
		break;

	case CPUFREQ_GOV_STOP:
		/*
		 * ZZ: enable all cores to avoid cores staying in offline state
		 * when changing to a non-hotplugging-able governor
		 */
#ifdef ENABLE_HOTPLUGGING
		enable_cores = true;
		queue_work_on(0, dbs_wq, &hotplug_online_work);
#endif
		dbs_timer_exit(this_dbs_info);

		this_dbs_info->idle_exit_time = 0;					// ZZ: idle exit time handling

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
#if defined(CONFIG_HAS_EARLYSUSPEND)
		unregister_early_suspend(&_powersave_early_suspend);
#elif defined(CONFIG_POWERSUSPEND)
		unregister_power_suspend(&powersave_powersuspend);
#endif
		break;

	case CPUFREQ_GOV_LIMITS:

		pol_max = policy->max;							// ZZ: for proportional scaling and freq thresholds ajustment
		pol_min = policy->min;							// ZZ: for freq thresholds ajustment

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

		/*
		 * ZZ: here again table order detection and limit optimizations
		 * in case max freq has changed after gov start and before
		 * Limit case due to apply timing issues. now we should be able to
		 * catch all freq max changes during start of the governor
		 */
		evaluate_scaling_order_limit_range(0, 1, suspend_flag, policy->max);

		if (old_pol_max == 0)							// ZZ: initialize var if we start the first time
		    old_pol_max = policy->max;

		if (dbs_tuners_ins.auto_adjust_freq_thresholds != 0) {
		    if (old_pol_max != policy->max) {
			pol_step = (old_pol_max / 100000) - (policy->max / 100000);	// ZZ: truncate and calculate step
			pol_step *= 100000;						// ZZ: bring it back to kHz
			pol_step *= -1;							// ZZ: invert for proper addition
		    } else {
			pol_step = 0;
		    }
		    old_pol_max = policy->max;
		}

		adjust_freq_thresholds(pol_step);					// ZZ: adjust thresholds

		this_dbs_info->time_in_idle
		= get_cpu_idle_time_us(cpu, &this_dbs_info->idle_exit_time);		// ZZ: idle exit time handling
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

static int __init cpufreq_gov_dbs_init(void)						// ZZ: idle exit time handling
{
    unsigned int i;
    struct cpu_dbs_info_s *this_dbs_info;
    // Initalize per-cpu data:
    for_each_possible_cpu(i) {
	this_dbs_info = &per_cpu(cs_cpu_dbs_info, i);
	this_dbs_info->time_in_idle = 0;
	this_dbs_info->idle_exit_time = 0;
    }

    dbs_wq = alloc_workqueue("zzmoove_dbs_wq", WQ_HIGHPRI, 0);

    if (!dbs_wq) {
	printk(KERN_ERR "[zzmoove] Failed to create zzmoove_dbs_wq workqueue!\n");
	return -EFAULT;
    }
#ifdef ENABLE_HOTPLUGGING
    INIT_WORK(&hotplug_offline_work, hotplug_offline_work_fn);				// ZZ: init hotplug offline work
    INIT_WORK(&hotplug_online_work, hotplug_online_work_fn);				// ZZ: init hotplug online work
#endif
	return cpufreq_register_governor(&cpufreq_gov_zzmoove);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_zzmoove);
	destroy_workqueue(dbs_wq);
}

MODULE_AUTHOR("Zane Zaminsky <cyxman@yahoo.com>");
MODULE_DESCRIPTION("'cpufreq_zzmoove' - A dynamic cpufreq governor based "
		"on smoove governor from Michael Weingaertner which was originally based on "
		"cpufreq_conservative from Alexander Clouter. Optimized for use with Samsung I9300 "
		"using a fast scaling and CPU hotplug logic - ported/modified for I9300 "
		"by ZaneZam since November 2012 and further improved in 2013/14");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
