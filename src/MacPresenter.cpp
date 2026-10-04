#ifdef KIRAN_MESA_OFFSCREEN

#define GL_SILENCE_DEPRECATION
#include "MacPresenter.h"
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

namespace
{
	GLuint g_texture = 0;
	int g_texWidth = 0;
	int g_texHeight = 0;
}

namespace macpresent
{
	bool init(GLFWwindow *window)
	{
		glfwMakeContextCurrent(window);
		// No vsync: frames are progressive accumulation steps, waiting on refresh only wastes time.
		glfwSwapInterval(0);
		glGenTextures(1, &g_texture);
		glBindTexture(GL_TEXTURE_2D, g_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		return glGetError() == GL_NO_ERROR;
	}

	void present(GLFWwindow *window, const void *rgba, int width, int height)
	{
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);

		glBindTexture(GL_TEXTURE_2D, g_texture);
		if (width != g_texWidth || height != g_texHeight)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
			g_texWidth = width;
			g_texHeight = height;
		}
		else
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
		}

		// glReadPixels rows start at the bottom, matching GL texture orientation.
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glEnd();

		glfwSwapBuffers(window);
	}
}

#endif
