# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(flutter_software VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include header files
include_directories(include)

# Add executable target
add_executable(flutter_software src/main.cpp)

############################################################
# Flutter Engine
############################################################

# include engine headers
include_directories(/Users/charafau/Utils/flutter_dev/engine/src/flutter/shell/platform/embedder)
include_directories(/opt/homebrew/Cellar/libsixel/1.10.5/include)
# find locally build engine library (libflutter_engine.so) and assingn it to FLUTTER_LIB variable
find_library(FLUTTER_LIB flutter_engine PATHS /Users/charafau/Utils/flutter_dev/engine/src/out/host_debug_unopt_arm64)
find_library(SIXEL_LIB sixel PATHS /opt/homebrew/Cellar/libsixel/1.10.5/lib)
# link library to our executable
target_link_libraries(flutter_software ${FLUTTER_LIB} ${SIXEL_LIB})
