LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := TeapotNativeActivity
LOCAL_SRC_FILES := TeapotNativeActivity.cpp \
 TeapotRenderer.cpp \
 SkyboxRenderer.cpp

LOCAL_C_INCLUDES :=

LOCAL_CFLAGS :=

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -latomic
LOCAL_STATIC_LIBRARIES := cpufeatures android_native_app_glue ndk_helper jui_helper

ifneq ($(filter %armeabi-v7a,$(TARGET_ARCH_ABI)),)
LOCAL_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1
LOCAL_LDLIBS += -lm_hard
ifeq (,$(filter -fuse-ld=mcld,$(APP_LDFLAGS) $(LOCAL_LDFLAGS)))
LOCAL_LDFLAGS += -Wl,--no-warn-mismatch
endif
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(LOCAL_PATH))
$(call import-module,ndk_helper)
$(call import-module,jui_helper)
$(call import-module,android/native_app_glue)
$(call import-module,android/cpufeatures)
