LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := librlog
LOCAL_LDLIBS := -llog

MY_FILES_PATH := $(LOCAL_PATH)/src/log
MY_FILES_SUFFIX := %.cpp %.c %.cc
My_All_Files := \
	$(patsubst \
		$(LOCAL_PATH)/%, \
		%, \
		$(filter \
			$(MY_FILES_SUFFIX), \
			$(shell \
				find $(MY_FILES_PATH) -type f \
			) \
		) \
	)
LOCAL_SRC_FILES := $(My_All_Files)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/log
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/log
include $(BUILD_SHARED_LIBRARY)
