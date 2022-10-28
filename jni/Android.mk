LOCAL_PATH := $(call my-dir)  
include $(CLEAR_VARS)  
LOCAL_MODULE := tools 

ZIPA_FILES = zipalign/ZipAlign.cpp zipalign/ZipEntry.cpp zipalign/ZipFile.cpp zipalign/android/native/src/utils/SharedBuffer.cpp zipalign/android/native/src/utils/VectorImpl.cpp

MD5_FILES = md5/md5_wrapper.cpp md5/md5.cpp

ELF_FILES = elfparser/elfparser.c elfparser/symbols.c

LOCAL_SRC_FILES := main.cpp $(ZIPA_FILES)

LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_LDLIBS := -lz -llog

LOCAL_C_INCLUDES += $(LOCAL_PATH)/zipalign \
$(LOCAL_PATH)/zipalign/android/base/include \
$(LOCAL_PATH)/zipalign/android/core/include \
$(LOCAL_PATH)/zipalign/android/native/include

LOCAL_CFLAGS := -fexceptions -Os -ffunction-sections -fdata-sections -fvisibility=hidden -w

include $(BUILD_SHARED_LIBRARY)
