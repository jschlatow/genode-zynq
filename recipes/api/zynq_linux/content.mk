#
# Content hosted in the dde_linux repository
#

MIRRORED_FROM_DDE_LINUX := src/lib/lx_emul \
                           src/lib/lx_kit \
                           src/include/lx_emul \
                           src/include/lx_user \
                           src/include/spec/arm/lx_kit \
                           src/include/lx_kit \
                           lib/import/import-lx_emul_common.inc

content: $(MIRRORED_FROM_DDE_LINUX)
$(MIRRORED_FROM_DDE_LINUX):
	mkdir -p $(dir $@); cp -r $(GENODE_DIR)/repos/dde_linux/$@ $(dir $@)


#
# Content hosted in the zynq repository
#

MIRRORED_FROM_REP_DIR := lib/mk/spec/arm_v7/zynq_linux_generated.mk \
                         lib/mk/spec/arm_v7/zynq_lx_emul.mk \
                         src/zynq_linux/common.inc \
                         lib/import/import-zynq_lx_emul.mk \
                         src/include/lx_emul/initcall_order.h \
                         src/lib/lx_emul/zynq

content: $(MIRRORED_FROM_REP_DIR)
$(MIRRORED_FROM_REP_DIR):
	$(mirror_from_rep_dir)


#
# Content from the Linux source tree
#

PORT_DIR := $(call port_dir,$(GENODE_DIR)/repos/dde_linux/ports/linux)
LX_REL_DIR := src/linux
LX_ABS_DIR := $(addsuffix /$(LX_REL_DIR),$(PORT_DIR))

# ingredients needed for creating a Linux build directory / generated headers
LX_FILES += Makefile \
            Kbuild \
            scripts/Makefile \
            scripts/Kbuild.include \
            scripts/subarch.include \
            scripts/Makefile.build \
            scripts/basic/Makefile \
            scripts/Makefile.extrawarn \
            scripts/Makefile.lib \
            scripts/Makefile.host \
            scripts/Makefile.asm-generic \
            scripts/Makefile.compiler \
            scripts/config \
            scripts/basic/fixdep.c \
            scripts/cc-version.sh \
            scripts/ld-version.sh \
            scripts/dtc \
            scripts/kconfig/merge_config.sh \
            scripts/remove-stale-files \
            scripts/sorttable.h \
            scripts/sorttable.c \
            scripts/setlocalversion \
            scripts/min-tool-version.sh \
            scripts/as-version.sh \
            scripts/mod \
            scripts/checksyscalls.sh \
            scripts/syscallhdr.sh \
            scripts/syscalltbl.sh \
            scripts/atomic/check-atomics.sh \
            tools/include/tools \
            arch/arm/Makefile \
            arch/arm/configs \
            kernel/configs/tiny.config \
            kernel/configs/tiny-base.config \
            arch/arm/boot/dts \
            arch/arm/include/asm/Kbuild \
            arch/arm/include/uapi/asm/Kbuild \
            arch/arm/tools/Makefile \
            arch/arm/tools/syscall.tbl \
            arch/arm/tools/syscallnr.sh \
            arch/arm/tools/gen-mach-types \
            arch/arm/tools/mach-types \
            include/asm-generic/Kbuild \
            include/linux/compiler-version.h \
            include/linux/license.h \
            include/linux/kbuild.h \
            include/uapi/Kbuild \
            include/uapi/asm-generic/Kbuild

LX_SCRIPTS_KCONFIG_FILES := $(notdir $(wildcard $(LX_ABS_DIR)/scripts/kconfig/*.c)) \
                            $(notdir $(wildcard $(LX_ABS_DIR)/scripts/kconfig/*.h)) \
                            Makefile lexer.l parser.y
LX_FILES += $(addprefix scripts/kconfig/,$(LX_SCRIPTS_KCONFIG_FILES)) \

LX_FILES += $(shell cd $(LX_ABS_DIR); find -name "Kconfig*" -printf "%P\n")

# needed for generated/asm-offsets.h
LX_FILES += kernel/bounds.c \
            kernel/time/timeconst.bc \
            arch/arm/kernel/asm-offsets.c \
            arch/arm/kernel/signal.h \
            arch/arm/include/asm/memory.h \
            arch/arm/include/asm/irqflags.h \
            arch/arm/include/asm/page.h \
            arch/arm/include/asm/pgtable.h \
            arch/arm/include/asm/fixmap.h \
            arch/arm/include/asm/smp_plat.h \
            arch/arm/include/asm/suspend.h \
            arch/arm/include/asm/kexec-internal.h \
            arch/arm/include/asm/glue-df.h \
            arch/arm/include/asm/glue-pf.h \
            arch/arm/include/asm/mach/arch.h \
            arch/arm/include/asm/mpu.h \
            arch/arm/include/asm/procinfo.h \
            arch/arm/include/asm/ucontext.h \
            arch/arm/include/asm/hardware/cache-l2x0.h \
            include/uapi/linux/arm_sdei.h \
            include/acpi \
            include/asm-generic/current.h \
            include/asm-generic/memory_model.h \
            include/asm-generic/qspinlock.h \
            include/asm-generic/fixmap.h \
            include/asm-generic/pgtable_uffd.h \
            include/linux/arm_sdei.h \
            include/linux/arm-smccc.h \
            include/linux/cper.h \
            include/linux/pgtable.h

# needed for gen_crc32table
LX_FILES += lib/gen_crc32table.c \
            lib/crc32.c

content: src/linux/include/linux/kvm_host.h
src/linux/include/linux/kvm_host.h: # cut dependencies from kvm via dummy header
	mkdir -p $(dir $@)
	touch $@

# prevent build of vdso as side effect of the linux-build directory preparation
content: src/linux/arch/arm/kernel/vdso/Makefile
src/linux/arch/arm/kernel/vdso/Makefile:
	mkdir -p $(dir $@)
	echo "default:" > $@
	echo "include/generated/vdso-offsets.h:" >> $@
	echo "arch/arm/kernel/vdso/vdso.so:" >> $@

# add content listed in the repository's source.list or dep.list files
LX_FILE_LISTS := $(shell find -H $(REP_DIR) -name dep.list -or -name source.list)
LX_FILES += $(shell cat $(LX_FILE_LISTS))
LX_FILES := $(sort $(LX_FILES))
MIRRORED_FROM_PORT_DIR += $(addprefix $(LX_REL_DIR)/,$(LX_FILES))

content: $(MIRRORED_FROM_PORT_DIR)
$(MIRRORED_FROM_PORT_DIR):
	mkdir -p $(dir $@)
	cp -r $(addprefix $(PORT_DIR)/,$@) $@

content: LICENSE
LICENSE:
	cp $(PORT_DIR)/src/linux/COPYING $@
