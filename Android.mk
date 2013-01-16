LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
		main.cpp \
		server.cpp \
		workers.c \
		sampler.c \
		settings.c \
		storage.c \
		key.c 

LOCAL_SHARED_LIBRARIES := libcutils libc libutils \
    libbinder \
    libcutils \
    
LOCAL_MODULE:= hdm
LOCAL_MODULE_TAGS:= optional

include $(BUILD_EXECUTABLE)

