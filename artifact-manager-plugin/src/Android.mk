LOCAL_PATH := $(call my-dir)
LIB_PATH = /opt/dependency-libs

################################################################
# External Modules
################################################################

# include $(CLEAR_VARS)
# LOCAL_MODULE := z
# LOCAL_LDLIBS := -L$(LIB_PATH)/lib
# LOCAL_SRC_FILES := $(LIB_PATH)/lib/libz.a
# LOCAL_C_INCLUDES += $(LIB_PATH)/lib/include
# include $(PREBUILT_STATIC_LIBRARY)

# include $(CLEAR_VARS)
# LOCAL_MODULE := z
# LOCAL_LDLIBS := -L$(LIB_PATH)/lib
# LOCAL_SRC_FILES := $(LIB_PATH)/lib/libz.so
# LOCAL_C_INCLUDES += $(LIB_PATH)/lib/include
# include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := json-c
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libjson-c.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

# # include $(CLEAR_VARS)
# LOCAL_MODULE := z3
# LOCAL_LDLIBS := -L$(LIB_PATH)/lib
# LOCAL_SRC_FILES := $(LIB_PATH)/lib/libz3.a
# LOCAL_C_INCLUDES += $(LIB_PATH)/lib/include
# include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := picosat
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libpicosat.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SHARED_LIBRARIES := ssl crypto
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libcurl.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libssl.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sodium
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libsodium.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := jerasure
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libJerasure.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_C_INCLUDES += $(LIB_PATH)/include/jerasure
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gf_complete
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libgf_complete.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libcrypto.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := b64
LOCAL_LDLIBS := -L$(LIB_PATH)/lib
LOCAL_SRC_FILES := $(LIB_PATH)/lib/libb64.a
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

# # include $(CLEAR_VARS)
# LOCAL_MODULE := libiop
# LOCAL_LDLIBS := -L$(LIB_PATH)/lib
# LOCAL_SRC_FILES := $(LIB_PATH)/lib/libiop.a
# LOCAL_C_INCLUDES += $(LIB_PATH)/include
# include $(PREBUILT_STATIC_LIBRARY)

################################################################
# Custom Static Modules
################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := linked_list
LOCAL_STATIC_LIBRARIES := picosat
LOCAL_MODULE_FILENAME := liblinkedlist
LOCAL_SRC_FILES := $(LOCAL_PATH)/linked_list.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/linked_list.h
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dep_solver
LOCAL_STATIC_LIBRARIES := picosat linked_list
LOCAL_MODULE_FILENAME := libdepsolver
LOCAL_SRC_FILES := $(LOCAL_PATH)/dep_solver.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/dep_solver.h
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := slothy
LOCAL_STATIC_LIBRARIES := ssl sss linked_list dep_solver curl json-c crypto
# LOCAL_LDLIBS += -lz
# define __android__ to use the android get_fd_archdep
LOCAL_CFLAGS += -D__android__
LOCAL_MODULE_FILENAME := libslothy
LOCAL_SRC_FILES := $(LOCAL_PATH)/slothy.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/slothy.h
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sss
LOCAL_CPP_FEATURES += exceptions
LOCAL_STATIC_LIBRARIES := ssl sodium jerasure gf_complete crypto
LOCAL_CFLAGS := -fblocks
LOCAL_MODULE_FILENAME := libsss
# LOCAL_SRC_FILES := $(LOCAL_PATH)/sss.c
LOCAL_SRC_FILES := $(LOCAL_PATH)/rs_shard.c
# LOCAL_SRC_FILES += $(LOCAL_PATH)/sss_shard.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/sss_shard.cpp
# LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/rs_shard.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss_shard.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss_shard.hpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libiop_classes.hpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LIB_PATH)/include
include $(BUILD_STATIC_LIBRARY)

################################################################
# Custom Shared Modules
################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := slothydeps-shared
LOCAL_WHOLE_STATIC_LIBRARIES := ssl sodium jerasure gf_complete crypto json-c picosat curl
LOCAL_MODULE_FILENAME := libslothydeps
LOCAL_LDLIBS = -lz
LOCAL_LDFLAGS += -Wl,-rpath=$$\{ORIGIN\} # -Wl,--no-whole-archive
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CPP_FEATURES += exceptions
LOCAL_MODULE := sss-shared
LOCAL_STATIC_LIBRARIES := ssl sodium jerasure gf_complete crypto
LOCAL_CFLAGS := -fblocks
LOCAL_MODULE_FILENAME := libsss
# LOCAL_SRC_FILES := $(LOCAL_PATH)/sss.c
LOCAL_SRC_FILES := $(LOCAL_PATH)/rs_shard.c
# LOCAL_SRC_FILES += $(LOCAL_PATH)/sss_shard.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/sss_shard.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/rs_shard.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss_shard.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sss_shard.hpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libiop_classes.hpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_LDFLAGS += -Wl,-rpath=$$\{ORIGIN\}
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := linked_list-shared
LOCAL_STATIC_LIBRARIES := picosat
LOCAL_MODULE_FILENAME := liblinkedlist
LOCAL_SRC_FILES := $(LOCAL_PATH)/linked_list.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/linked_list.h
LOCAL_LDFLAGS += -Wl,-rpath=$$\{ORIGIN\}
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dep_solver-shared
LOCAL_STATIC_LIBRARIES := picosat
LOCAL_SHARED_LIBRARIES := linked_list-shared
LOCAL_MODULE_FILENAME := libdepsolver
LOCAL_SRC_FILES := $(LOCAL_PATH)/dep_solver.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/dep_solver.h
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_LDFLAGS += -Wl,-rpath=$$\{ORIGIN\}
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := slothy-shared
LOCAL_SHARED_LIBRARIES := slothydeps-shared sss-shared linked_list-shared dep_solver-shared
# LOCAL_LDLIBS := -L$(LIB_PATH)/lib
# LOCAL_LDLIBS += -lssl -lz -lcurl -ljson-c -lcrypto
# define __android__ to use the android get_fd_archdep
LOCAL_CFLAGS += -D__android__ -fPIC -Wall #-pthread
LOCAL_MODULE_FILENAME := libslothy
LOCAL_SRC_FILES := $(LOCAL_PATH)/slothy.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/slothy.h
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_LDFLAGS += -fPIC
LOCAL_LDFLAGS += -Wl,-rpath=$$\{ORIGIN\}
include $(BUILD_SHARED_LIBRARY)

################################################################
# Executables
################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := slothy-jni
LOCAL_MODULE_FILENAME := libslothy-jni
# LOCAL_STATIC_LIBRARIES := curl z z3 json-c b64 ssl crypto sodium c++_shared linked_list dep_solver slothy sss
LOCAL_STATIC_LIBRARIES := b64 ssl sss slothy crypto
LOCAL_LDLIBS = -lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)
# LOCAL_C_INCLUDES += $(LOCAL_PATH)/lib/include
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/slothyjni.c
LOCAL_CFLAGS += -fPIC -Wall #-pthread
LOCAL_LDFLAGS += -fPIC
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := b64_android
LOCAL_MODULE_FILENAME := b64_android
# LOCAL_STATIC_LIBRARIES := curl z z3 json-c b64 ssl crypto sodium c++_shared linked_list dep_solver slothy sss
LOCAL_STATIC_LIBRARIES := b64 ssl sss slothy crypto
LOCAL_LDLIBS := -lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/b64_android.c
LOCAL_CFLAGS += -fPIE -Wall #-pthread
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := load_artifact-static
LOCAL_MODULE_FILENAME := load_artifact-static
# LOCAL_STATIC_LIBRARIES := curl z z3 json-c b64 ssl crypto sodium c++_shared linked_list dep_solver slothy sss
LOCAL_STATIC_LIBRARIES := ssl sss slothy crypto
LOCAL_CFLAGS += -D__android__
LOCAL_LDLIBS := -lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/load_artifact.c
LOCAL_CFLAGS += -fPIE -Wall #-pthread
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := load_artifact-dynamic
LOCAL_MODULE_FILENAME := load_artifact-dynamic
LOCAL_SHARED_LIBRARIES := slothydeps-shared sss-shared linked_list-shared dep_solver-shared slothy-shared
LOCAL_CFLAGS += -D__android__
LOCAL_LDLIBS := -lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/load_artifact.c
LOCAL_CFLAGS += -fPIE -Wall #-pthread
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := two-face
LOCAL_MODULE_FILENAME := two-face
# LOCAL_STATIC_LIBRARIES := curl z z3 json-c b64 ssl crypto sodium c++_shared linked_list dep_solver slothy sss
LOCAL_STATIC_LIBRARIES := ssl curl crypto
LOCAL_LDLIBS = -lz
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/two-face.c
LOCAL_CFLAGS += -fPIE #-pthread
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := load_so
LOCAL_MODULE_FILENAME := load_so
# LOCAL_STATIC_LIBRARIES := curl z z3 json-c b64 ssl crypto sodium c++_shared linked_list dep_solver slothy sss
LOCAL_STATIC_LIBRARIES := ssl sss slothy crypto
LOCAL_LDLIBS = -lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LIB_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/load_so.c
LOCAL_CFLAGS += -fPIE #-pthread
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)
