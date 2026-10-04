#ifndef KIRAN_MESA_OFFSCREEN

#include "GLContext.h"
#include <GLFW/glfw3.h>

namespace
{
	GLFWwindow *g_window = nullptr;
}

namespace glctx
{
	void windowHints()
	{
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	}

	bool init(GLFWwindow *window)
	{
		g_window = window;
		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			return false;
		glfwSwapInterval(1);
		return true;
	}

	GLuint defaultFramebuffer() { return 0; }

	void beginFrame() {}

	void present() { glfwSwapBuffers(g_window); }

	void shutdown() {}

	const char *backendName() { return "native"; }
}

#endif
