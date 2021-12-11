CC := g++
CFLAGS := $(CFLAGS) -fpic

LIB_DIRS := \
	/usr/local/lib

INCLUDE_DIRS := \
	/usr/local/include

BUILD_FLAGS := \
	$(BUILD_FLAGS) \
	-pthread -lGLEW -lGLU -lGL
LINK_LIBRARIES := \
	$(LINK_LIBRARIES) \
	stdc++fs \
	X11

PRODUCTION_LINUX_ICON := icon

PRODUCTION_LINUX_APP_NAME := 3D CGoL
PRODUCTION_LINUX_APP_COMMENT := 3D Conways Game of Life
