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


if (WIN32)
    # Add GLFW include directory and link directory only for Windows
    target_include_directories(PathTracingRenderer PUBLIC
        ${PROJECT_SOURCE_DIR}/deps/GLFW/
    )

    target_link_directories(PathTracingRenderer PUBLIC
        ${PROJECT_SOURCE_DIR}/deps/GLFW/
    )
endif()
if (MSVC)
    # Use the Multi-threaded DLL (/MD) or Multi-threaded Debug DLL (/MDd)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
# Link libraries (specify the libraries you need here)
target_link_libraries(PathTracingRenderer
    glfw3
)

# Set platform-specific configurations
if (WIN32)
    # For Windows-specific configurations
    set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Target platform")

    # Specify additional dependencies for Windows if needed
    #target_link_libraries(PathTracingRenderer
   #     opengl32
   # )

    # Enable solution folder organization (optional, requires newer CMake)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Export Visual Studio solution when on Windows
if (WIN32)
    message(STATUS "Generating Visual Studio solution for Windows")
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
endif()

# Custom output paths (optional, for organized builds)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
