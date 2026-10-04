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
file(GLOB_RECURSE DEPS_IMGUI
    "${PROJECT_SOURCE_DIR}/deps/imgui/*.cpp"
    "${PROJECT_SOURCE_DIR}/deps/imgui/*.c"
    "${PROJECT_SOURCE_DIR}/deps/imgui/*.h"
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
    ${PROJECT_SOURCE_DIR}/deps/imgui/

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
# glad provides the GL declarations; stop GLFW from pulling in the system GL headers.
target_compile_definitions(PathTracingRenderer PUBLIC GLFW_INCLUDE_NONE)

# Link libraries (specify the libraries you need here)
if (WIN32)
    target_link_libraries(PathTracingRenderer glfw3)
else()
    find_package(glfw3 3.3 QUIET)
    if (glfw3_FOUND)
        target_link_libraries(PathTracingRenderer glfw)
    else()
        target_link_libraries(PathTracingRenderer glfw3)
    endif()
endif()

# ---------------------------------------------------------------------------
# OpenGL context backend (see src/GLContext.h)
#   OFF: native GLFW context (Windows/Linux).
#   ON : headless OpenGL 4.6 via Mesa Zink + KosmicKrisp, frames shown through a
#        tiny Apple-GL presenter. macOS only (needs `brew install mesa glfw`).
# ---------------------------------------------------------------------------
if (APPLE)
    option(KIRAN_MESA_OFFSCREEN "Use Mesa Zink/KosmicKrisp for OpenGL 4.6 on macOS" ON)
else()
    set(KIRAN_MESA_OFFSCREEN OFF)
endif()

if (KIRAN_MESA_OFFSCREEN)
    set(MESA_ROOT "" CACHE PATH "Mesa install prefix (defaults to Homebrew's)")
    find_path(MESA_EGL_INCLUDE_DIR EGL/egl.h
        HINTS ${MESA_ROOT}/include /opt/homebrew/opt/mesa/include /usr/local/opt/mesa/include)
    find_library(MESA_EGL_LIBRARY EGL
        HINTS ${MESA_ROOT}/lib /opt/homebrew/opt/mesa/lib /usr/local/opt/mesa/lib)
    find_library(VULKAN_LOADER_LIBRARY vulkan HINTS /opt/homebrew/lib /usr/local/lib)
    find_file(KOSMICKRISP_ICD
        NAMES kosmickrisp_mesa_icd.aarch64.json kosmickrisp_mesa_icd.json
        HINTS ${MESA_ROOT}/share/vulkan/icd.d /opt/homebrew/share/vulkan/icd.d /usr/local/share/vulkan/icd.d)

    if (NOT MESA_EGL_INCLUDE_DIR OR NOT MESA_EGL_LIBRARY OR NOT VULKAN_LOADER_LIBRARY OR NOT KOSMICKRISP_ICD)
        message(FATAL_ERROR "KIRAN_MESA_OFFSCREEN needs Mesa with Zink/KosmicKrisp: `brew install mesa`. "
                            "Or configure with -DKIRAN_MESA_OFFSCREEN=OFF.")
    endif()
    message(STATUS "GL backend: Mesa offscreen (ICD: ${KOSMICKRISP_ICD})")

    # After glad's include dir so glad's KHR headers stay first.
    target_include_directories(PathTracingRenderer PRIVATE ${MESA_EGL_INCLUDE_DIR})
    target_compile_definitions(PathTracingRenderer PRIVATE
        KIRAN_MESA_OFFSCREEN
        KIRAN_KOSMICKRISP_ICD="${KOSMICKRISP_ICD}"
        IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    # ImGui's built-in loader would bind Apple's GL; make it use glad (i.e. Mesa) instead.
    set_source_files_properties(${PROJECT_SOURCE_DIR}/deps/imgui/imgui_impl_opengl3.cpp
        PROPERTIES COMPILE_OPTIONS "-include;glad/glad.h")
    target_link_libraries(PathTracingRenderer ${MESA_EGL_LIBRARY} "-framework OpenGL")

    # Zink dlopens @rpath/libvulkan.1.dylib.
    get_filename_component(MESA_LIB_DIR ${MESA_EGL_LIBRARY} DIRECTORY)
    get_filename_component(VULKAN_LIB_DIR ${VULKAN_LOADER_LIBRARY} DIRECTORY)
    set_target_properties(PathTracingRenderer PROPERTIES BUILD_RPATH "${MESA_LIB_DIR};${VULKAN_LIB_DIR}")
else()
    message(STATUS "GL backend: native GLFW context")
endif()

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
