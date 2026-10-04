#ifdef KIRAN_MESA_OFFSCREEN

#include "GLContext.h"
#include "MacPresenter.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
	GLFWwindow *g_window = nullptr;
	EGLDisplay g_display = EGL_NO_DISPLAY;
	EGLContext g_context = EGL_NO_CONTEXT;

	GLuint g_fbo = 0;
	GLuint g_colorRb = 0;
	GLuint g_depthRb = 0;
	int g_width = 0;
	int g_height = 0;
	std::vector<unsigned char> g_pixels;

	// Defaults only; anything already set in the environment wins.
	void configureMesaEnvironment()
	{
		setenv("GALLIUM_DRIVER", "zink", 0);
		// KosmicKrisp lacks VK_EXT_custom_border_color, so Zink conservatively
		// advertises GL 2.1. Everything Kiran uses is supported, so ask for 4.6.
		setenv("MESA_GL_VERSION_OVERRIDE", "4.6", 0);
		setenv("MESA_GLSL_VERSION_OVERRIDE", "460", 0);
#ifdef KIRAN_KOSMICKRISP_ICD
		setenv("VK_ICD_FILENAMES", KIRAN_KOSMICKRISP_ICD, 0);
#endif
	}

	void resizeTarget(int width, int height)
	{
		g_width = width;
		g_height = height;
		g_pixels.resize(size_t(width) * size_t(height) * 4);

		if (!g_fbo)
		{
			glGenFramebuffers(1, &g_fbo);
			glGenRenderbuffers(1, &g_colorRb);
			glGenRenderbuffers(1, &g_depthRb);
		}
		glBindRenderbuffer(GL_RENDERBUFFER, g_colorRb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, g_depthRb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_colorRb);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_depthRb);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cerr << "[glctx] offscreen framebuffer incomplete\n";
	}
}

namespace glctx
{
	void windowHints()
	{
		// The window only needs Apple's legacy GL context to blit finished frames.
		// Render at window resolution rather than 2x Retina; the path tracer is the bottleneck.
		glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
	}

	bool init(GLFWwindow *window)
	{
		g_window = window;
		configureMesaEnvironment();

		if (!macpresent::init(window))
		{
			std::cerr << "[glctx] failed to set up window presenter\n";
			return false;
		}

		auto getPlatformDisplay =
			(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
		if (!getPlatformDisplay)
		{
			std::cerr << "[glctx] eglGetPlatformDisplayEXT unavailable\n";
			return false;
		}
		g_display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, nullptr, nullptr);
		EGLint major = 0, minor = 0;
		if (g_display == EGL_NO_DISPLAY || !eglInitialize(g_display, &major, &minor))
		{
			std::cerr << "[glctx] eglInitialize failed (0x" << std::hex << eglGetError() << std::dec
					  << "). Is Mesa installed (brew install mesa)?\n";
			return false;
		}
		eglBindAPI(EGL_OPENGL_API);

		const EGLint ctxAttribs[] = {
			EGL_CONTEXT_MAJOR_VERSION, 4,
			EGL_CONTEXT_MINOR_VERSION, 6,
			EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
			EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
			EGL_NONE};
		g_context = eglCreateContext(g_display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, ctxAttribs);
		if (g_context == EGL_NO_CONTEXT ||
			!eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_context))
		{
			std::cerr << "[glctx] could not create an OpenGL 4.6 context (0x" << std::hex
					  << eglGetError() << std::dec << ")\n";
			return false;
		}

		if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress))
			return false;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		resizeTarget(width > 0 ? width : 1, height > 0 ? height : 1);
		return true;
	}

	GLuint defaultFramebuffer() { return g_fbo; }

	void beginFrame()
	{
		int width, height;
		glfwGetFramebufferSize(g_window, &width, &height);
		if (width > 0 && height > 0 && (width != g_width || height != g_height))
			resizeTarget(width, height);
		glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	}

	void present()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo);
		glPixelStorei(GL_PACK_ALIGNMENT, 4);
		glReadPixels(0, 0, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, g_pixels.data());
		macpresent::present(g_window, g_pixels.data(), g_width, g_height);
	}

	void shutdown()
	{
		if (g_display != EGL_NO_DISPLAY)
		{
			eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (g_context != EGL_NO_CONTEXT)
				eglDestroyContext(g_display, g_context);
			eglTerminate(g_display);
		}
		g_context = EGL_NO_CONTEXT;
		g_display = EGL_NO_DISPLAY;
	}

	const char *backendName() { return "mesa-offscreen (zink/kosmickrisp)"; }
}

#endif
