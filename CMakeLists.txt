cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the project name and version
project(PathTracingRenderer VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all source files in the src directory recursively
file(GLOB_RECURSE SRC_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

# Collect all source files in the deps directory recursively
file(GLOB_RECURSE DEPS_SOURCES
    "${PROJECT_SOURCE_DIR}/deps/**/*.cpp"
    "${PROJECT_SOURCE_DIR}/deps/**/*.c"
    "${PROJECT_SOURCE_DIR}/deps/**/*.h"

)

# Combine all source files
set(SOURCES ${SRC_SOURCES} ${DEPS_SOURCES})

# Add the executable
add_executable(PathTracingRenderer ${SOURCES})

# Include directories
target_include_directories(PathTracingRenderer PUBLIC
    ${PROJECT_SOURCE_DIR}/deps
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/deps/Glad/include/
)

# Link directories (if you have any precompiled libraries in the deps directory)
target_link_directories(PathTracingRenderer PUBLIC
    ${PROJECT_SOURCE_DIR}/deps/lib
)

# Link libraries (specify the libraries you need here)
target_link_libraries(PathTracingRenderer
    # Example libraries, replace with actual libraries you are using
    glfw
)
