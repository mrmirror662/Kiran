#include "renderer.h"
#include <iostream>

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "[OpenGL Error] ";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "[OpenGL Warning] ";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "[OpenGL Performance Warning] ";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << "[OpenGL Notification] ";
		break;
	default:
		std::cout << "[Unknown OpenGL Severity] ";
		break;
	}
	std::cout << "(";
	bool error = false;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "ERROR";
		error = true;
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "OTHER";
		break;
	}

	std::cout << ") " << message << std::endl;
	if (error)
	{
		throw std::runtime_error(message);
	}
}
struct temptex
{

	uint64_t textureHandle;
};
static temptex testTexture()
{
	const int width = 512;
	const int height = 512;
	const size_t textureSize = width * height * 3; // 3 channels (RGB)
	unsigned char textureData[textureSize];

	// Generate a checkerboard pattern
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Determine the color of the current pixel
			bool isWhite = ((x / 10) % 2 == (y / 10) % 2); // 4x4 tiles
			unsigned char color = isWhite ? 255 : 0;	   // White or black

			// Set the pixel's RGB values
			int index = (y * width + x) * 3;
			textureData[index + 0] = isWhite ? color : 50; // Red
			textureData[index + 1] = isWhite ? color : 50; // Green
			textureData[index + 2] = isWhite ? color : 50; // Blue
		}
	}

	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, 1, GL_RGB8, width, height);

	// Upload the checkerboard pattern to the texture
	glTextureSubImage2D(
		texture,
		0, 0, 0, width, height, // level, xoffset, yoffset, width, height
		GL_RGB, GL_UNSIGNED_BYTE,
		(const void *)textureData);

	// Retrieve the texture handle after we finish creating the texture
	const uint64_t handle = glGetTextureHandleARB(texture);
	if (handle == 0)
	{
		std::cerr << "Error! Handle returned null" << std::endl;
		exit(-1);
	}

	return {handle};
}
static BindlessTexture testBindlessTex()
{
	const int width = 512;
	const int height = 512;
	const size_t textureSize = width * height * 3; // 3 channels (RGB)
	std::vector<uint8_t> textureData(textureSize);

	// Generate a checkerboard pattern
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Determine the color of the current pixel
			bool isWhite = ((x / 10) % 2 == (y / 10) % 2); // 4x4 tiles
			unsigned char color = isWhite ? 255 : 0;	   // White or black

			// Set the pixel's RGB values
			int index = (y * width + x) * 3;
			textureData[index + 0] = isWhite ? color : 50; // Red
			textureData[index + 1] = isWhite ? color : 50; // Green
			textureData[index + 2] = isWhite ? color : 50; // Blue
		}
	}
	BindlessTexture test(width, height, 3, textureData);
	return test;
}
Renderer::Renderer(GLFWwindow *window)
	: window(window), rt_shader("shaders/rt.glsl", "shaders/vert.glsl"), display_shader("shaders/display.glsl", "shaders/vert.glsl"), compute_shader(new Shader("shaders/rt.comp")), cam({0, 0, -1.0f}), scene(nullptr), bvh(nullptr)
{
	glfwGetFramebufferSize(window, &width, &height);
	current = 0;
	frame = 1;
	fpsTimer = 0.0;
	printTimer = 0.0;
	startTime = glfwGetTime();
	delta = 0;
	dcounter = 0;
}

void Renderer::setScene(Scene &sceneRef)
{
	scene = &sceneRef;
}

void Renderer::setBVH(BVH &bvhRef)
{
	bvh = &bvhRef;
}

Renderer::~Renderer()
{

	// Clean up resources if necessary
}
temptex tt;
void Renderer::init()
{
	printGLVersion();
	glViewport(0, 0, width, height);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	setupShaders();
	setupTextures();
	setupFramebuffers();

	if (!scene || !bvh)
	{
		std::cerr << "Renderer::init: Scene or BVH not set!" << std::endl;
		return;
	}
	std::cout << "colorMaps size: " << scene->colorMaps.size() << std::endl;
	for (const auto &t : scene->colorMaps)
	{
		std::cout << "w: " << t.width << " h: " << t.height << " ch: " << t.channel << " buf: " << std::endl;
	}
	std::vector<BindlessTexture> blCMap;
	for (auto &t : scene->colorMaps)
	{
		if (t.channel != 3 && t.channel != 4)
		{
			std::cerr << "Skipping texture with unsupported channel count: " << t.channel << std::endl;
			continue;
		}
		blCMap.emplace_back(t.width, t.height, t.channel, t.buffer);
	}

	// Only rebuild BVH if needed externally, not here

	triangle_data.bind();
	triangle_data.fillData(bvh->triangles);
	triangle_data.bindBase(6);

	mat_data.bind();
	mat_data.fillData(scene->mats);
	mat_data.bindBase(7);

	sphere_data.bind();
	sphere_data.fillData(scene->spheres);
	sphere_data.bindBase(8);

	bvh_data.bind();
	bvh_data.fillData(bvh->nodes);
	bvh_data.bindBase(9);

	std::vector<uint64_t> handles;
	for (auto &blT : blCMap)
	{
		blT.MakeResident();
		handles.push_back(blT.GetHandle());
	}
	ShaderStorage<uint64_t> textureHandles;
	textureHandles.bind();
	textureHandles.fillData(handles);
	textureHandles.bindBase(11);

	Texture::setActiveUnit(3);
	Texture env(scene->hdr.width, scene->hdr.height, scene->hdr.data);
}

void Renderer::processInput()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void Renderer::updateUniforms(float currentTime)
{
}

void Renderer::renderScene(float currentTime, float dt)
{
	if (this->cam.onUpdate(window, dt))
	{
		this->resetFrame();
	}
	if (!scene || !bvh)
		return;

	// Ensure camera + state updates
	delta = dt;

	// CORRECT ping-pong logic:
	GLuint outputTexId = texture[current].getId();
	GLuint prevTexId = texture[1 - current].getId();

	// Bind output texture for compute shader (writeonly image)
	glBindImageTexture(0, outputTexId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, prevTexId, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	// Bind compute shader
	compute_shader->bind();

	// Set uniforms
	compute_shader->setUniform("iResolution", (float)width, (float)height);
	compute_shader->setUniform("iTime", currentTime - startTime);
	compute_shader->setUniform("iFrame", frame);
	// compute_shader->setUniform("delta", delta);
	compute_shader->setUniform("camera_pos", cam.getPosition().x, cam.getPosition().y, cam.getPosition().z);
	compute_shader->setUniform("angle_offset", cam.getAngleOffset().x, cam.getAngleOffset().y);
	compute_shader->setUniform("prevFrame", 1); // Bind prevFrame to texture unit 1

	compute_shader->setUniform("sphere_size", static_cast<int>(scene->spheres.size()));
	compute_shader->setUniform("bvh_size", static_cast<int>(bvh->nodes.size()));

	// Set path tracing control uniforms
	compute_shader->setUniform("accumulateBounces", accumulateBounces);
	compute_shader->setUniform("movingBounces", movingBounces);
	compute_shader->setUniform("accumulateSamples", accumulateSamples);
	compute_shader->setUniform("movingSamples", movingSamples);
	// --- FOV uniform ---
	compute_shader->setUniform("fov", fov);

	// Bind required SSBOs
	triangle_data.bindBase(6);
	mat_data.bindBase(7);
	sphere_data.bindBase(8);
	bvh_data.bindBase(9);
	indices.bindBase(10);

	// Launch compute shader
	int groupCountX = (width + 7) / 8;
	int groupCountY = (height + 7) / 8;
	glDispatchCompute(groupCountX, groupCountY, 1);
	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	if (accumulate)
		frame++;

	// Display result with display shader
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	display_shader.bind();
	display_shader.setUniform("iResolution", (float)width, (float)height);
	display_shader.setUniform("iFrame", frame);
	// display_shader.setUniform("delta", delta);
	display_shader.setUniform("textureSampler", 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, outputTexId);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Flip buffers AFTER everything is done
	current = 1 - current;
}
void Renderer::handleResize()
{
	int current_width, current_height;
	glfwGetWindowSize(window, &current_width, &current_height);
	if (current_width != width || current_height != height)
	{
		width = current_width;
		height = current_height;
		glViewport(0, 0, width, height);
		for (auto &t : texture)
		{
			t.reSize(width, height);
		}
		for (int i = 0; i < fbo.size(); ++i)
		{
			fbo[i].attachTexure(texture[i]);
		}
		this->resetFrame();
	}
}

void Renderer::resetFrame()
{
	this->frame = 1;
}

void Renderer::setAccumulation(bool accumulate)
{
	if (this->accumulate != accumulate)
		this->resetFrame();
	this->accumulate = accumulate;
}

void Renderer::printGLVersion()
{
	char *glVersion = (char *)glGetString(GL_VERSION);
	char *glVendor = (char *)glGetString(GL_VENDOR);
	char *glRenderer = (char *)glGetString(GL_RENDERER);
	std::cout << "GL Version: " << glVersion << "\n";
	std::cout << "GL Vendor: " << glVendor << "\n";
	std::cout << "GL Renderer: " << glRenderer << "\n";
}

void Renderer::setupShaders()
{
	// No rt_shader usage here
	display_shader.bind();
	display_shader.initUniform("iResolution");
	display_shader.initUniform("iFrame");
	// display_shader.initUniform("delta");
	display_shader.initUniform("textureSampler");
	display_shader.setUniform("textureSampler", 1);

	compute_shader->bind();
	compute_shader->initUniform("iResolution");
	compute_shader->initUniform("iTime");
	compute_shader->initUniform("iFrame");
	compute_shader->initUniform("camera_pos");
	compute_shader->initUniform("angle_offset");
	// compute_shader->initUniform("delta");
	compute_shader->initUniform("triangle_size");
	compute_shader->initUniform("sphere_size");
	compute_shader->initUniform("bvh_size");
	compute_shader->initUniform("hdri");
	compute_shader->setUniform("hdri", 3);
	compute_shader->initUniform("prevFrame"); // Bind prevFrame to texture unit 2

	compute_shader->initUniform("accumulateBounces");
	compute_shader->initUniform("movingBounces");
	compute_shader->initUniform("accumulateSamples");
	compute_shader->initUniform("movingSamples");
	// --- FOV uniform ---
	compute_shader->initUniform("fov");
	compute_shader->setUniform("fov", fov);
}

void Renderer::setupTextures()
{
	texture.reserve(2);
	for (int i = 0; i < 2; ++i)
	{
		texture.push_back(Texture(width, height));
	}
}

void Renderer::setupFramebuffers()
{
	fbo.resize(2);
	for (int i = 0; i < 2; ++i)
	{
		fbo[i].bind();
		fbo[i].attachTexure(texture[i]);
	}
}
