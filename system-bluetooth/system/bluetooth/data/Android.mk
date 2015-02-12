#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

dest_dir := $(TARGET_OUT)/etc/bluetooth

files := \
	audio.conf \
	input.conf \
	blacklist.conf \
	auto_pairing.conf

BT_VENDOR_ID ?= 0x0008
BT_PRODUCT_ID ?= 0xb000
BT_VERSION_ID ?= 0x0000
BT_SPECIFICATION_ID ?= 0x0103
BT_PRIMARY_RECORD ?= 0x01
BT_VENDOR_ID_SOURCE ?= 0x0001
BT_DEBUGKEYS ?= false
BT_SUPPORT_LE ?= false

copy_to := $(addprefix $(dest_dir)/,$(files))

ifneq ($(TARGET_BUILD_VARIANT),user)
BT_DEBUGKEYS := true
endif

ifeq ($(BOARD_HAVE_BLUETOOTH_LE),true)
BT_SUPPORT_LE := true
endif

TARGET_CPP := $(TARGET_TOOLS_PREFIX)cpp$(HOST_EXECUTABLE_SUFFIX)
MAIN_CONF = $(dest_dir)/main.conf
$(MAIN_CONF): $(LOCAL_PATH)/main.conf
	mkdir -p $(dest_dir)
	$(TARGET_CPP) -x assembler-with-cpp -nostdinc -P \
		-DVENDOR_ID=$(BT_VENDOR_ID) \
		-DPRODUCT_ID=$(BT_PRODUCT_ID) \
		-DVERSION_ID=$(BT_VERSION_ID) \
		-DSPECIFICATION_ID=$(BT_SPECIFICATION_ID) \
		-DPRIMARY_RECORD=$(BT_PRIMARY_RECORD) \
		-DVENDOR_ID_SOURCE=$(BT_VENDOR_ID_SOURCE) \
		-DDEBUGKEYS=$(BT_DEBUGKEYS) $< $@ \
		-DSUPPORT_LE=$(BT_SUPPORT_LE)

$(copy_to): PRIVATE_MODULE := bluetooth_etcdir
$(copy_to): $(dest_dir)/%: $(LOCAL_PATH)/% | $(ACP)
	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(copy_to)
ALL_PREBUILT += $(MAIN_CONF)
