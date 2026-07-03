# Compiler and flags
SANITIZERS = -fsanitize=address,undefined
CXX = /opt/homebrew/opt/llvm/bin/clang++
CXXFLAGS = -fcolor-diagnostics -fansi-escape-codes -g -pedantic-errors -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -fopenmp -std=c++23 -O0 -arch arm64 $(SANITIZERS) -MMD -MP

# Include paths
INCLUDES = -Isrc -I/Users/max/OpenGL_Framework/src -I/opt/homebrew/Cellar/SDL2/2.32.10/include -I/opt/homebrew/Cellar/glew/2.3.1/include -I/opt/homebrew/Cellar/glm/1.0.3/include -isystem/opt/homebrew/Cellar/eigen/5.0.1/include/eigen3/Eigen

# Library paths and linker flags
LDFLAGS = -L/Users/max/OpenGL_Framework/lib -L/opt/homebrew/Cellar/SDL2/2.32.10/lib -L/opt/homebrew/Cellar/glew/2.3.1/lib -L/opt/homebrew/Cellar/glm/1.0.3/lib
LINKER_FLAGS = -lopenglframework -lsdl2 -framework OpenGL -lglew -lglm

# Directories
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = lib

# Source and object files
# SRCS = $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/main.cpp)
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
TARGET = $(BUILD_DIR)/main

all: $(TARGET)

# Ensure the framework library is built before the project
$(TARGET): $(OBJS) /Users/max/OpenGL_Framework/$(LIB_DIR)/libopenglframework.a
	$(CXX) $(CXXFLAGS) $(SANITIZERS) $(LINKER_FLAGS) $(LDFLAGS) -o $@ $^

# Compile source files to object files
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
