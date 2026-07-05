# Compiler and flags
SANITIZERS = -fsanitize=address,undefined
CXX = /opt/homebrew/opt/llvm/bin/clang++
CXXFLAGS = -fcolor-diagnostics -fansi-escape-codes -g -pedantic-errors -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -fopenmp -std=c++23 -O0 -arch arm64 $(SANITIZERS) -MMD -MP

# Include paths
INCLUDES = -Isrc -I/Users/max/OpenGL_Framework/src -I/opt/homebrew/Cellar/sdl2/2.32.10/include -I/opt/homebrew/Cellar/sdl2/2.32.10/include/sdl2 -I/opt/homebrew/Cellar/glew/2.3.1/include -I/opt/homebrew/Cellar/glm/1.0.3/include -I/Users/max/OpenGL_Framework/vendor/imgui -I/Users/max/OpenGL_Framework/vendor/imgui/backends

# Library paths and linker flags
LDFLAGS = -L/Users/max/OpenGL_Framework/lib -L/opt/homebrew/Cellar/sdl2/2.32.10/lib -L/opt/homebrew/Cellar/glew/2.3.1/lib -L/opt/homebrew/Cellar/glm/1.0.3/lib
LINKER_FLAGS = -lopenglframework -lsdl2 -framework OpenGL -lglew -lglm

# Directories
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = lib
IMGUI_DIR = /Users/max/OpenGL_Framework/vendor/imgui

# Source and object files
LOCAL_SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
LOCAL_OBJS  = $(LOCAL_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# External ImGui Source and Object files
IMGUI_SRCS= $(IMGUI_DIR)/imgui.cpp \
			$(IMGUI_DIR)/imgui_draw.cpp \
			$(IMGUI_DIR)/imgui_widgets.cpp \
			$(IMGUI_DIR)/imgui_tables.cpp \
			$(IMGUI_DIR)/imgui_demo.cpp \
			$(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
			$(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# Map external absolute source paths to flat object names inside build/imgui/
IMGUI_OBJS= $(patsubst $(IMGUI_DIR)/%.cpp, $(BUILD_DIR)/imgui/%.o, \
			$(patsubst $(IMGUI_DIR)/backends/%.cpp, $(BUILD_DIR)/imgui/%.o, $(IMGUI_SRCS)))

ALL_OBJS = $(LOCAL_OBJS) $(IMGUI_OBJS)
DEPS     = $(ALL_OBJS:.o=.d)
TARGET   = $(BUILD_DIR)/main

all: $(TARGET)

# Link the executable
$(TARGET): $(ALL_OBJS) /Users/max/OpenGL_Framework/$(LIB_DIR)/libopenglframework.a
	$(CXX) $(CXXFLAGS) $(SANITIZERS) -o $@ $(ALL_OBJS) $(LDFLAGS) $(LINKER_FLAGS)

# Rule 1: Compile local project source files
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Rule 2: Compile core ImGui external files
$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Rule 3: Compile ImGui backend external files
$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/backends/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

release:
	$(MAKE) clean
	$(MAKE) SANITIZERS="" CXXFLAGS="$(filter-out -O0 $(SANITIZERS),$(CXXFLAGS)) -O3"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
