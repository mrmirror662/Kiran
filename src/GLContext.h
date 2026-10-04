#pragma once

// Pluggable OpenGL context backend.
//
// The rest of the renderer only talks to this interface, so the GL backend can be
// swapped at build time without touching rendering code:
//
//   - Native (default on Windows/Linux): GLFW creates the GL context for the window,
//     the default framebuffer is 0 and presenting is glfwSwapBuffers.
//
//   - Mesa offscreen (KIRAN_MESA_OFFSCREEN, default on macOS): macOS caps OpenGL at 4.1,
//     so we create a headless OpenGL 4.6 context with Mesa's Zink driver running on
//     KosmicKrisp (Vulkan on Metal), render into an offscreen framebuffer, and copy
//     each finished frame into the GLFW window.
//
// Implementations: GLContextNative.cpp, GLContextMesa.cpp. Exactly one is compiled in.

#include <glad/glad.h>

struct GLFWwindow;

namespace glctx
{
	// Call after glfwInit() and before glfwCreateWindow().
	void windowHints();

	// Creates/makes current the GL context and loads GL function pointers (glad).
	bool init(GLFWwindow *window);

	// Framebuffer that represents "the screen". Use this instead of binding 0.
	GLuint defaultFramebuffer();

	// Call at the start of every frame (keeps the offscreen target sized to the window).
	void beginFrame();

	// Show the finished frame in the window.
	void present();

	void shutdown();

	const char *backendName();
}
