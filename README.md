ZZMoove governor is based on the modified conservative (original author Alexander Clouter <alex@digriz.org.uk>)
'smoove' governor from Michael Weingaertner <mialwe@googlemail.com>
(source: https://github.com/mialwe/mngb/)

Modified by Zane Zaminsky since November 2012 (and further improved in 2013/14 by Zane Zaminsky and Yank555)
to be hotplug-able and optimzed for use with Samsung I9300. CPU Hotplug modifications partially taken from
ktoonservative governor from ktoonsez KT747-JB kernel (source: https://github.com/ktoonsez/KT747-JB)

File Description:
cpufreq_zzmoove.c -> governor source file |
cpufreq_zzmoove_profiles.h -> governor profiles header file |
kernel_patches/support_for_cpu_temp_reading.patch -> example patch for exynos4 cpu temperature reading support
(example for boeffla kernels - kernel upstream version 3.0.101) |
kernel_patches/enable_cpu_temp_reading.patch -> example patch for enabling exynos4 cpu temperature reading support
(example for boeffla kernels - kernel upstream version 3.0.101)