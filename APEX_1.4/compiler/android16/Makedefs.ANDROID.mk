
TARGET_OS   ?= android-16
TARGET_ARCH ?= arm

ifeq ($(TARGET_ARCH),arm)
  TARGET_ABI = arm-linux-androideabi
  GCC_ARCH   = arm-linux-androideabi
  GCC_ABI    = armv7-a
  STL_ABI    = armeabi-v7a
endif
ifeq ($(TARGET_ARCH),x86)
  TARGET_ABI = i686-android-linux
  GCC_ARCH   = x86
  GCC_ABI    = 
  STL_ABI    = x86
endif

# we try and pick the latest available.
ifneq ($(wildcard $(NDKROOT)/toolchains/$(GCC_ARCH)-4.4.3/*),)
  GCC_VERS      = 4.4.3
endif
ifneq ($(wildcard $(NDKROOT)/toolchains/$(GCC_ARCH)-4.6.3/*),)
  GCC_VERS      = 4.6.3
endif
# GCC 4.6 on NDK appears requires slightly different pathing for some libs and object files.
ifneq ($(wildcard $(NDKROOT)/toolchains/$(GCC_ARCH)-4.6/*),)
  GCC_VERS      = 4.6
  GCC_LIB_VERS  = 4.6.x-google
endif
ifneq ($(wildcard $(NDKROOT)/toolchains/$(GCC_ARCH)-4.7.2/*),)
  GCC_VERS      = 4.7.2
endif
ifneq ($(wildcard $(NDKROOT)/toolchains/$(GCC_ARCH)-4.8/*),)
  GCC_VERS      = 4.8
endif
GCC_LIB_VERS ?= $(GCC_VERS)


HOST_OS:=$(shell uname | tr A-Z a-z)
ifneq ($(HOST_OS),darwin)
  HOST_OS:=$(shell uname -o | tr A-Z a-z)
endif


ifeq ($(HOST_OS),cygwin)
  TOOLCHAIN_DIR      = $(NDKROOT)/toolchains/$(GCC_ARCH)-$(GCC_VERS)/prebuilt/windows
else
  ifneq (,$(findstring msys,$(HOST_OS)))
    TOOLCHAIN_DIR      = $(NDKROOT)/toolchains/$(GCC_ARCH)-$(GCC_VERS)/prebuilt/windows
  else
    ifeq ($(HOST_OS),darwin)
      TOOLCHAIN_DIR    = $(NDKROOT)/toolchains/$(GCC_ARCH)-$(GCC_VERS)/prebuilt/darwin-x86
    else
      TOOLCHAIN_DIR    = $(NDKROOT)/toolchains/$(GCC_ARCH)-$(GCC_VERS)/prebuilt/linux-x86
    endif
  endif
endif

ifeq ($(wildcard $(TOOLCHAIN_DIR)),)
    TOOLCHAIN_DIR = UNDEFINED_TOOLCHAIN
endif

NDKPREFIX    = $(TOOLCHAIN_DIR)/bin/$(TARGET_ABI)
PLATFORM_DIR = $(NDKROOT)/platforms/$(TARGET_OS)
SYSROOT      = $(PLATFORM_DIR)/arch-$(TARGET_ARCH)
STLROOT      = UNDEFINED_STL

ifneq ($(wildcard $(NDKROOT)/sources/cxx-stl/gnu-libstdc++/include),)
    STLROOT = $(NDKROOT)/sources/cxx-stl/gnu-libstdc++
endif
# NDK r8b requires the GCC version number in the path here, but older versions don't.
ifneq ($(wildcard $(NDKROOT)/sources/cxx-stl/gnu-libstdc++/$(GCC_VERS)/include),)
    STLROOT = $(NDKROOT)/sources/cxx-stl/gnu-libstdc++/$(GCC_VERS)
endif

CC          = $(NDKPREFIX)-gcc
CXX         = $(NDKPREFIX)-g++
CCLD        = $(NDKPREFIX)-g++
CC_NOCACHE    = $(CC)
CXX_NOCACHE   = $(CXX)
CCLD_NOCACHE  = $(CCLD)
AR          = $(NDKPREFIX)-ar
RANLIB      = $(NDKPREFIX)-ranlib
PRASEDEPEND = parsedepend
ECHO        = echo
RMDIR       = rm -rf
MKDIR       = mkdir
CAT         = cat
CP          = cp
STRIP       = $(NDK_PREFIX)strip
OBJDUMP     = $(NDK_PREFIX)objdump
OBJCOPY     = $(NDK_PREFIX)objcopy