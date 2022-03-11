#
# Linux kernel configuration
#

include $(REP_DIR)/src/zynq_linux/common.inc

# additional options needed for real-world startup (things happening when
# executing /sbin/init on Ubuntu) without these options, we are only able to
# boot into a shell
LX_ENABLE += COMPAT_32BIT_TIME MULTIUSER UNIX EPOLL PROC_FS FILE_LOCKING
LX_ENABLE += POSIX_TIMERS TMPFS DEVTMPFS DEVTMPFS_MOUNT BINFMT_SCRIPT

# RTC stuff needed for hwclock
LX_ENABLE += I2C RTC_CLASS RTC_DRV_DS1307 I2C_CADENCE

# performance stuff
LX_ENABLE += SMP ARM_ZYNQ_CPUIDLE VFP VFPv3 NEON

# some userspace processes may slow down the system if futexes are unavailable
LX_ENABLE += FUTEX
