LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := GTASASpeedometer
LOCAL_SRC_FILES := main.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_CPPFLAGS  += -O2 -std=c++17 -fno-rtti -fno-exceptions -DNDEBUG
LOCAL_LDLIBS    += -llog
include $(BUILD_SHARED_LIBRARY)

