AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS) -I../../3rdparty/imgui -I../../3rdparty/imgui/examples -I../../3rdparty/imgui/misc/stl
AM_CFLAGS = $(WZ_CFLAGS) -DIMGUI_IMPL_OPENGL_LOADER_GLEW
AM_CXXFLAGS = $(WZ_CXXFLAGS) -DIMGUI_IMPL_OPENGL_LOADER_GLEW

# Here, we need to set "plugins" for Qt5 to be static
# FIXME: At a future date, we should check if they want static builds, but, since we only support those for now...
if MINGW32
AM_CFLAGS += -DQT_STATICPLUGIN
AM_CPPFLAGS += -DQT_STATICPLUGIN
AM_CXXFLAGS += -DQT_STATICPLUGIN
endif

noinst_LIBRARIES = libsdl.a
noinst_HEADERS = \
	cursors_sdl.h \
	wz2100icon.h

libsdl_a_SOURCES = \
	cursors_sdl.cpp \
	../../3rdparty/imgui/imgui.cpp \
	../../3rdparty/imgui/imgui_draw.cpp \
	../../3rdparty/imgui/imgui_widgets.cpp \
	../../3rdparty/imgui/imgui_demo.cpp \
	../../3rdparty/imgui/examples/imgui_impl_opengl3.cpp \
	../../3rdparty/imgui/examples/imgui_impl_sdl.cpp \
	../../3rdparty/imgui/misc/cpp/imgui_stdlib.cpp \
	main_sdl.cpp

EXTRA_DIST= \
	swapinterval.mm
