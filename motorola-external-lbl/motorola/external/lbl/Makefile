#
# Makefile: Makefile for the Linux Boot Loader (lbl)
#
# Copyright (c) 2006-2007 Motorola, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Date         Author          Comment
# ===========  ==============  ==============================================
# 04-Oct-2006  Motorola        Initial revision.
# 09-Nov-2006  Motorola        Changed how setup.h is included (now through API_INCS)
# 22-Nov-2006  Motorola        Fixed some Makelint bugs.
# 14-Dec-2006  Motorola        Changed chmod to rm to get around clearcase
#                              winkin issue.
# 08-May-2007  Motorola	       Removed hardcoded values for HOSTCC & INSTALLDIR
# 01-Aug-2007  Motorola	       Fixed Makelint issues.
# 12-Sep-2007  Motorola        Added CONFIG_MOT_FEAT_DEVICE_TREE.
#


LOCAL_MODULE := lbl

TOPDIR := ./
TARGET_PREFIX := $(TOPDIR)prebuilt/$(HOST_PREBUILT_TAG)/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
TARGET_CC := $(TARGET_PREFIX)gcc
TARGET_OBJCOPY := $(TARGET_PREFIX)objcopy
TARGET_AR := $(TARGET_PREFIX)ar
TARGET_NM := $(TARGET_PREFIX)nm

LOCAL_PATH := $(TOPDIR)motorola/external/lbl

HOST_CC := gcc

# this is from properties files
PART_VERSION_SIZE := 24

LBL_CFLAGS += -Os -Wall
LBL_CFLAGS += -fpic -fno-builtin
LBL_CFLAGS += -DCONFIG_MOT_FEAT_BOOTINFO -DCONFIG_MOT_FEAT_POWERUP_LOGO
LBL_CFLAGS += -DCONFIG_MOT_FEAT_FX2LP_I2C -DCONFIG_MOT_FEAT_DEVICE_TREE
LBL_CFLAGS += -DPART_VERSION_SIZE=$(PART_VERSION_SIZE)
LBL_CFLAGS += -I$(TOPDIR)bionic/libc/kernel/common
LBL_CFLAGS += -I$(TOPDIR)bionic/libc/kernel/arch-arm

LBL_ASFLAGS = -D__ASSEMBLY__ -fpic
LBL_STARTLDFLAGS := -static -nostdlib -T $(LOCAL_PATH)/start-ld-script

GENINCFILE := $(LBL_OUT)/geninc.h

# start.o MUST be the first object in STARTOBJS.
STARTOBJS := start.o
## trampoline.o MUST be the first object in RESTOBJS.
LIB_UTIL_A=util.a
LBLBIN := $(LBL_OUT)/lbl
LBL_STARTBIN := $(LBL_OUT)/lbl_start
LBL_STARTELF := $(LBL_OUT)/lbl_start_elf32

LBL_RESTBIN := $(LBL_OUT)/lbl_rest
GENASM=genasm

#
#STARTDEPENDS=lbl.h
RESTDEPENDS := $(LOCAL_PATH)/util.h $(LOCAL_PATH)/lbl.h $(LOCAL_PATH)/boot_info.h $(LOCAL_PATH)/types.h
#
#MACH_DEBUG_TEST = ${${HW_ARCH}_DEBUG_TEST}
MACH_SRC := mach-zas

LBL_DEBUG ?= 0
#
# debugging support
ifneq (${LBL_DEBUG},0) 
	# machine/board specific flags should only be set if LBL debugging is enabled
	LBL_CFLAGS += -DLBL_DEBUG -g
	LBL_ASFLAGS += -DLBL_DEBUG -g ${${HW_ARCH}_ASFLAGS}
	MACH_SRC := mach-zas

	ifneq (${MACH_SRC},)
		LBL_CFLAGS += -I$(COMPTOP)/$(MACH_SRC) -I$(COMPTOP)
		LBL_ASFLAGS += -I$(COMPTOP)/$(MACH_SRC) -I$(COMPTOP)
	endif

	STARTDEPENDS += $(LOCAL_PATH)/debug.h $(LOCAL_PATH)/$(MACH_SRC)/mach-debug.h
	RESTDEPENDS += $(LOCAL_PATH)/debug.h $(LOCAL_PATH)/$(MACH_SRC)/mach-debug.h
	STARTOBJS += $(MACH_SRC)/initdebug.o $(MACH_SRC)/putchar-asm.o puts-asm.o

	DEBUG_TEST = $(MACH_DEBUG_TEST)

endif # LBL_DEBUG

restelf_object_files :=
#--------------------------------------------------------------------------
# for LBL_RESTBIN

UTIL_LIB := $(LBL_OUT)/util.a
RESTELF := $(LBL_OUT)/lbl_rest_elf32

lbl_rest_bin: $(LBL_RESTBIN)
$(LBL_RESTBIN): $(RESTELF)
	@mkdir -p $(dir $@)
	$(TARGET_OBJCOPY) -O binary -R .note -R .comment -R .bss -S $^ $@

#--------------------------------------------------------------------------
lbl_start_bin: $(LBL_STARTBIN)
$(LBL_STARTBIN): $(LBL_STARTELF)
	@mkdir -p $(dir $@)
	$(TARGET_OBJCOPY) -O binary -R .note -R .comment -R .bss -S $^ $@

#--------------------------------------------------------------------------
start_objects_files :=
GEN := $(LBL_OUT)/start.o
$(GEN): $(LOCAL_PATH)/start.S $(GENINCFILE)
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_ASFLAGS) -c $< -o $@
start_objects_files += $(GEN)

# when debug
#STARTOBJS += $(MACH_SRC)/initdebug.o $(MACH_SRC)/putchar-asm.o puts-asm.o

lbl_start_elf32: $(LBL_STARTELF)
$(LBL_STARTELF): $(start_objects_files)
	$(TARGET_CC) $(LBL_STARTLDFLAGS) -o $@ $^

#--------------------------------------------------------------------------
RESTLDFLAGS := -Wl,-z,nocombreloc,-Bsymbolic -shared -nostdlib -T $(LOCAL_PATH)/rest-ld-script
RESTLDFLAGS += -g

# we don't use transform-c-or-s-to-o-no-deps, for more flexible control
GEN := $(LBL_OUT)/trampoline.o
$(GEN): $(LOCAL_PATH)/trampoline.S
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_ASFLAGS) -c $< -o $@
restelf_object_files += $(GEN)

GEN := $(LBL_OUT)/dynlink.o
$(GEN): $(LOCAL_PATH)/dynlink.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
restelf_object_files += $(GEN)

GEN := $(LBL_OUT)/main.o
$(GEN): $(LOCAL_PATH)/main.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) \
		$(foreach incdir, \
			$(if $(PRIVATE_NO_DEFAULT_COMPILER_FLAGS),, \
				$(TARGET_PROJECT_INCLUDES) \
				$(TARGET_C_INCLUDES) \
			) \
			$(PRIVATE_C_INCLUDES) \
			, \
			-I $(incdir) \
		) \
		$(LBL_CFLAGS) -c $< -o $@
restelf_object_files += $(GEN)

GEN := $(LBL_OUT)/stack.o
$(GEN): $(LOCAL_PATH)/stack.S
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_ASFLAGS) -c $< -o $@
restelf_object_files += $(GEN)

GEN := $(LBL_OUT)/atag.o
$(GEN): $(LOCAL_PATH)/atag.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) \
		$(foreach incdir, \
			$(if $(PRIVATE_NO_DEFAULT_COMPILER_FLAGS),, \
				$(TARGET_PROJECT_INCLUDES) \
				$(TARGET_C_INCLUDES) \
			) \
			$(PRIVATE_C_INCLUDES) \
			, \
			-I $(incdir) \
		) \
		$(LBL_CFLAGS) -c $< -o $@
restelf_object_files += $(GEN)

# comment the following line to make lbl_rest_elf32 module invisible
lbl_rest_elf32: $(RESTELF)
$(RESTELF): $(restelf_object_files) $(UTIL_LIB)
	$(TARGET_CC) $(RESTLDFLAGS) -o $@ $^ -static -lgcc
	# fail if there are any undefined symbols
	test -z `$(TARGET_NM) -u $@`

#--------------------------------------------------------------------------
lbl_lib_util_objs :=

GEN := $(LBL_OUT)/strlen.o
$(GEN): $(LOCAL_PATH)/strlen.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
lbl_lib_util_objs += $(GEN)

GEN := $(LBL_OUT)/memcpy.o
$(GEN): $(LOCAL_PATH)/memcpy.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
lbl_lib_util_objs += $(GEN)

GEN := $(LBL_OUT)/vsprintf.o
$(GEN): $(LOCAL_PATH)/vsprintf.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
lbl_lib_util_objs += $(GEN)

GEN := $(LBL_OUT)/printf.o
$(GEN): $(LOCAL_PATH)/printf.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
lbl_lib_util_objs += $(GEN)

# stdio.o is for debug 
#GEN := $(LBL_OUT)/stdio.o
#$(GEN): $(LOCAL_PATH)/stdio.c
#	@mkdir -p $(dir $@)
#	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
#lbl_lib_util_objs += $(GEN)

# putchar.o is for debug
#GEN := $(LBL_OUT)/putchar.o
#$(GEN): $(LOCAL_PATH)/$(MACH_SRC)/putchar.c
#	@mkdir -p $(dir $@)
#	$(TARGET_CC) $(LBL_CFLAGS) -c $< -o $@
#lbl_lib_util_objs += $(GEN)

lbl_lib_util: $(UTIL_LIB)
$(UTIL_LIB): $(lbl_lib_util_objs)
	$(TARGET_AR) rc $@ $^

#--------------------------------------------------------------------------
#$(STARTOBJS): $(GENINCFILE) $(STARTDEPENDS)

#--------------------------------------------------------------------------
all: lbl_bin

lbl_bin: $(LBLBIN)

$(LBLBIN): $(LBL_STARTBIN) $(LBL_RESTBIN)
	cat $^ > $@

#--------------------------------------------------------------------------
# for GENINCLFILE
# can be removed later, currently  I am testing to build geninc.h

gen_inc_file: $(GENINCFILE)

$(GENINCFILE): $(GENASM) $(LBL_RESTBIN)
	@mkdir -p $(dir $@)
	@set - `wc -c $(LBL_RESTBIN)` && \
	lbl_rest_size=$$1 && \
	rm -f $(GENINCFILE) && \
	echo "#define LBL_REST_SIZE ($$1)" > $(GENINCFILE)
# TBD
	$(MOTO_GENASM) >> $(GENINCFILE)

#--------------------------------------------------------------------------
# for GENASM

MOTO_GENASM := $(LBL_OUT)/$(GENASM)

include $(CLEAR_VARS)
LOCAL_MODULE := $(GENASM)
LOCAL_SRC_FILES := genasm.c

ifeq ($(HOST_OS),cygwin)
LOCAL_CFLAGS += -DWIN32_EXE
endif
ifeq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DMACOSX_RSRC
endif
ifeq ($(HOST_OS),linux)
endif

LOCAL_CFLAGS += -O2 -Wall

$(GENASM): $(LOCAL_PATH)/genasm.c
	mkdir -p $(LBL_OUT)
	$(HOST_CC) $(LOCAL_CFLAGS) -o $(MOTO_GENASM) $< 

