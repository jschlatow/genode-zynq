#
# \brief  Build config for Genodes core process
# \author Johannes Schlatow
# \date   2014-12-15
#

REQUIRES = hw

# add C++ sources
SRC_CC += platform_services.cc

CC_MARCH = -mcpu=cortex-a9 -mfpu=neon-vfpv4 -mfloat-abi=softfp

# include less specific configuration
include $(call select_from_repositories,lib/mk/spec/cortex_a9/core-hw.inc)
