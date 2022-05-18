LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

LOCAL_SRC_FILES := ../../../../main.cpp ../../../../vendor/PUGIXML/src/pugixml.cpp ../../../../vendor/IMGUI/imgui.cpp ../../../../vendor/IMGUI/imgui_demo.cpp ../../../../vendor/IMGUI/imgui_draw.cpp ../../../../vendor/IMGUI/imgui_tables.cpp ../../../../vendor/IMGUI/imgui_widgets.cpp ../../../../vendor/IMGUI/backends/imgui_impl_sdl.cpp ../../../../vendor/IMGUI/backends/imgui_impl_sdlrenderer.cpp ../../../../vendor/IMGUI/misc/cpp/imgui_stdlib.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf SDL2_mixer SDL2_net

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
