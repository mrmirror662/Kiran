#pragma once

// Displays CPU-side RGBA8 frames in a GLFW window using Apple's own OpenGL.
// Kept in its own translation unit: Apple's GL headers cannot coexist with glad.
// Only used by the Mesa offscreen backend (GLContextMesa.cpp).

struct GLFWwindow;

namespace macpresent
{
	bool init(GLFWwindow *window);
	void present(GLFWwindow *window, const void *rgba, int width, int height);
}
