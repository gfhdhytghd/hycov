# Makefile for hycov plugin
ifeq ($(CXX),g++)
    EXTRA_FLAGS = --no-gnu-unique
else
    EXTRA_FLAGS =
endif

SOURCES = src/main.cpp src/dispatchers.cpp src/OvGridLayout.cpp src/globaleventhook.cpp
CXXFLAGS = -shared -fPIC $(EXTRA_FLAGS) -g `pkg-config --cflags pixman-1 libdrm hyprland pangocairo` -std=c++2b -Wno-narrowing -Wno-template-body

all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o hycov.so

clean:
	rm -f ./hycov.so
