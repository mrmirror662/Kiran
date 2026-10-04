#include <algorithm>
#include "renderer.h"
#include "GLContext.h"
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
	: window(window), rt_shader("shaders/rt.glsl", "shaders/vert.glsl"), display_shader("shaders/display.glsl", "shaders/vert.glsl"), compute_shader(new Shader("shaders/rt.comp")), light_shader(new Shader("shaders/rt.comp", ShaderDefines{ "#define LIGHT_TRACE\n" })), cam({0, 0, -1.0f}), scene(nullptr), accel(nullptr)
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

void Renderer::setLights(LightTable &lightsRef)
{
	lightTable = &lightsRef;
}

void Renderer::setAccel(AccelerationStructure &accelRef)
{
	accel = &accelRef;
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
	glGenVertexArrays(1, &fullscreenVao);
	glGenQueries(4, &timerQueries[0][0]);
	setupShaders();
	setupTextures();
	setupFramebuffers();

	if (!scene || !accel)
	{
		std::cerr << "Renderer::init: Scene or acceleration structure not set!" << std::endl;
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

	triangle_data.bind();
	triangle_data.fillData(accel->triangles);
	triangle_data.bindBase(6);

	mat_data.bind();
	mat_data.fillData(scene->mats);
	mat_data.bindBase(7);

	sphere_data.bind();
	sphere_data.fillData(scene->spheres);
	sphere_data.bindBase(8);

	blas_data.bind();
	blas_data.fillData(accel->blasGpu);
	blas_data.bindBase(9);

	tlas_data.bind();
	tlas_data.fillData(accel->tlasGpu);
	tlas_data.bindBase(12);

	instance_data.bind();
	instance_data.fillData(accel->instances);
	instance_data.bindBase(13);

	triangle_vert_data.bind();
	triangle_vert_data.fillData(accel->triangleVerts);
	triangle_vert_data.bindBase(14);

	light_data.bind();
	light_data.fillData(lightTable ? lightTable->lights : std::vector<GPULight>{});
	light_data.bindBase(15);
	light_node_data.bind();
	light_node_data.fillData(lightTable ? lightTable->nodes : std::vector<GPULightNode>{});
	light_node_data.bindBase(16);
	light_index_data.bind();
	light_index_data.fillData(lightTable ? lightTable->lightIndexOf : std::vector<int>{ -1 });
	light_index_data.bindBase(17);
	emit_cdf_data.bind();
	emit_cdf_data.fillData(lightTable ? lightTable->emitCdf : std::vector<float>{ 0.0f });
	emit_cdf_data.bindBase(18);
	caustic_data.bind();
	caustic_data.fillData(std::vector<uint32_t>(size_t(width) * height * 3, 0u));
	caustic_data.bindBase(19);
	allocatePhotonMap();
	delta_light_data.bind();
	delta_light_data.fillData(lightTable ? lightTable->deltaLights : std::vector<int>{ -1 });
	delta_light_data.bindBase(20);
	std::cout << "Lights: " << (lightTable ? lightTable->lights.size() : 0) << " (analytic + emissive triangles)" << std::endl;

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

// Mean clamped linear-sRGB colour of a wavelength over [380, 730] nm, using the same CIE 1931
// fit as rt.comp's wavelength_rgb, so spectral weights average to white.
static glm::vec3 spectralNormalization()
{
	auto lobe = [](float l, float mu, float s1, float s2) {
		float t = (l - mu) / (l < mu ? s1 : s2);
		return std::exp(-0.5f * t * t);
	};
	glm::vec3 sum(0.0f);
	const int n = 2048;
	for (int i = 0; i < n; ++i)
	{
		float l = 360.0f + 470.0f * (i + 0.5f) / n;
		float x = 1.056f * lobe(l, 599.8f, 37.9f, 31.0f) + 0.362f * lobe(l, 442.0f, 16.0f, 26.7f) - 0.065f * lobe(l, 501.1f, 20.4f, 26.2f);
		float y = 0.821f * lobe(l, 568.8f, 46.9f, 40.5f) + 0.286f * lobe(l, 530.9f, 16.3f, 31.1f);
		float z = 1.217f * lobe(l, 437.0f, 11.8f, 36.0f) + 0.681f * lobe(l, 459.0f, 26.0f, 13.8f);
		glm::vec3 rgb(3.2406f * x - 1.5372f * y - 0.4986f * z,
			-0.9689f * x + 1.8758f * y + 0.0415f * z,
			0.0557f * x - 0.2040f * y + 1.0570f * z);
		sum += glm::max(rgb, glm::vec3(0.0f));
	}
	return sum * (470.0f / float(n)); // integral over [360, 830] nm: sample_wavelength divides by the pdf
}

// Uniforms shared by the camera pass and the light-tracing pass.
// One photon record per light path (one light path per pixel), plus the grid heads.
void Renderer::allocatePhotonMap()
{
	photon_data.bind();
	photon_data.fillData(std::vector<uint32_t>(2 + kPhotonGridSize + size_t(kPhotonWords) * width * height, 0u));
	photon_data.bindBase(22);
}

void Renderer::applyTraceUniforms(Shader &s, float currentTime)
{
	// Progressive photon merging: radius r0 * k^((alpha - 1) / 2), alpha = 0.75 (Knaus & Zwicker).
	s.setUniform("usePhotonMerging", photonMerging ? 1 : 0);
	s.setUniform("photonRadius", photonRadius0 * std::pow(float(std::max(frame, 1)), -0.125f));
	glUniform1ui(glGetUniformLocation(s.getId(), "photonGridSize"), kPhotonGridSize);
	glUniform1ui(glGetUniformLocation(s.getId(), "photonCapacity"), static_cast<GLuint>(width * height));
	s.setUniform("photonsEmitted", float(width) * float(height));

	// Set uniforms
	s.setUniform("iResolution", (float)width, (float)height);
	s.setUniform("iTime", currentTime - startTime);
	s.setUniform("iFrame", frame);
	// s.setUniform("delta", delta);
	s.setUniform("camera_pos", cam.getPosition().x, cam.getPosition().y, cam.getPosition().z);
	s.setUniform("angle_offset", cam.getAngleOffset().x, cam.getAngleOffset().y);
	s.setUniform("prevFrame", 1); // Bind prevFrame to texture unit 1

	s.setUniform("sphere_size", static_cast<int>(scene->spheres.size()));
	s.setUniform("tlas_size", static_cast<int>(accel->tlasGpu.size()));

	// Set path tracing control uniforms
	s.setUniform("accumulateBounces", accumulateBounces);
	s.setUniform("movingBounces", movingBounces);
	s.setUniform("accumulateSamples", accumulateSamples);
	s.setUniform("movingSamples", movingSamples);
	// --- FOV uniform ---
	s.setUniform("fov", fov);
	s.setUniform("emissionScale", emissionScale);
	s.setUniform("debugView", debugView);

	s.setUniform("light_count", lightTable ? static_cast<int>(lightTable->lights.size()) : 0);
	s.setUniform("light_node_count", lightTable ? static_cast<int>(lightTable->nodes.size()) : 0);
	s.setUniform("infinite_light_count", lightTable ? lightTable->infiniteCount : 0);
	s.setUniform("useNEE", useNEE ? 1 : 0);
	s.setUniform("neeDepth", neeDepth);
	s.setUniform("rrStart", rrStart);
	const Fog& fog = scene->fog;
	s.setUniform("fogEnabled", fog.enabled ? 1 : 0);
	s.setUniform("fogMin", fog.min.x, fog.min.y, fog.min.z);
	s.setUniform("fogMax", fog.max.x, fog.max.y, fog.max.z);
	glUniformMatrix4fv(glGetUniformLocation(s.getId(), "fogToLocal"), 1, GL_FALSE, &fog.toLocal[0][0]);
	s.setUniform("fogSigmaS", fog.sigma_s.x, fog.sigma_s.y, fog.sigma_s.z);
	s.setUniform("fogSigmaA", fog.sigma_a.x, fog.sigma_a.y, fog.sigma_a.z);
	s.setUniform("fogG", fog.g);

	s.setUniform("useEquiangular", equiangular ? 1 : 0);
	s.setUniform("tileWidth", tileWidth);
	s.setUniform("emit_light_count", lightTable ? static_cast<int>(lightTable->lights.size()) : 0);
	s.setUniform("ltDebugDirect", ltDebugDirect ? 1 : 0);
	s.setUniform("ltMinDist", ltMinDist);
	s.setUniform("delta_light_count", lightTable ? lightTable->deltaLightCount() : 0);
	static const glm::vec3 spectralNorm = spectralNormalization();
	s.setUniform("spectralNorm", spectralNorm.x, spectralNorm.y, spectralNorm.z);
}

void Renderer::renderScene(float currentTime, float dt)
{
	if (this->cam.onUpdate(window, dt))
	{
		this->resetFrame();
	}
	if (!scene || !accel)
		return;

	// Ensure camera + state updates
	delta = dt;

	// Adaptive sampling accumulates in place (per-pixel sample counts in alpha): converged
	// pixels keep their sums while the rest go on sampling, until none is left.
	const bool adaptive = noiseThreshold > 0.0f && accumulate;
	auto display = [&](GLuint imageTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, glctx::defaultFramebuffer());
		display_shader.bind();
		display_shader.setUniform("iResolution", (float)width, (float)height);
		display_shader.setUniform("iFrame", frame);
		display_shader.setUniform("exposure", exposure);
		display_shader.setUniform("srgbOutput", srgbOutput ? 1 : 0);
		display_shader.setUniform("perPixelCount", adaptive ? 1 : 0);
		display_shader.setUniform("showAdaptive", adaptive && showAdaptive ? 1 : 0);
		display_shader.setUniform("textureSampler", 1);
		display_shader.setUniform("adaptiveSampler", 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture[2].getId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, imageTex);
		glBindVertexArray(fullscreenVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	};
	if (adaptive && renderDone)
	{
		display(texture[current].getId()); // render complete: keep showing the result
		return;
	}

	// CORRECT ping-pong logic:
	GLuint outputTexId = texture[current].getId();
	GLuint prevTexId = texture[1 - current].getId();

	if (adaptive)
	{
		if (frame == 1)
		{
			glClearTexImage(outputTexId, 0, GL_RGBA, GL_FLOAT, nullptr);
			glClearTexImage(texture[2].getId(), 0, GL_RGBA, GL_FLOAT, nullptr);
		}
		glBindImageTexture(2, texture[2].getId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(0, outputTexId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, outputTexId, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	}
	else
	{
		// Bind output texture for compute shader (writeonly image)
		glBindImageTexture(0, outputTexId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(1, prevTexId, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	}

	// Bind required SSBOs
	triangle_data.bindBase(6);
	mat_data.bindBase(7);
	sphere_data.bindBase(8);
	blas_data.bindBase(9);
	indices.bindBase(10);
	tlas_data.bindBase(12);
	instance_data.bindBase(13);
	triangle_vert_data.bindBase(14);
	light_data.bindBase(15);
	light_node_data.bindBase(16);
	light_index_data.bindBase(17);
	emit_cdf_data.bindBase(18);
	caustic_data.bindBase(19);
	delta_light_data.bindBase(20);

	int groupCountX = (width + 7) / 8;
	int groupCountY = (height + 7) / 8;

	// Light tracing first: caustic paths splat into caustic_data, which the camera pass
	// resolves (and clears) per pixel.
	// Only transmissive surfaces form the specular chains the light tracer handles; without
	// them it would only add cost, and the camera pass skips nothing.
	bool hasTransmission = scene && std::any_of(scene->mats.begin(), scene->mats.end(),
		[](const Material &m) { return m.transmission > 0.0f; });
	bool traceCaustics = causticTracing && hasTransmission && lightTable && lightTable->emitCdf.back() > 0.0f;
	photon_data.bindBase(22);
	if (traceCaustics)
	{
		if (photonMerging)
		{
			// Fresh photon map per frame: count 0, every grid list empty.
			const uint32_t none = 0xFFFFFFFFu, zero = 0u;
			glClearNamedBufferSubData(photon_data.getId(), GL_R32UI, 2 * sizeof(uint32_t), kPhotonGridSize * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &none);
			glClearNamedBufferSubData(photon_data.getId(), GL_R32UI, 0, sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
		}
		light_shader->bind();
		applyTraceUniforms(*light_shader, currentTime);
		glDispatchCompute(groupCountX, groupCountY, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	compute_shader->bind();
	applyTraceUniforms(*compute_shader, currentTime);
	compute_shader->setUniform("useCausticTracing", traceCaustics ? 1 : 0);
	compute_shader->setUniform("adaptive", adaptive ? 1 : 0);
	compute_shader->setUniform("noiseThreshold", noiseThreshold);
	compute_shader->setUniform("adaptiveMinSpp", adaptiveMinSpp);
	compute_shader->setUniform("adaptiveMaxSpp", adaptiveMaxSpp);
	if (adaptive)
	{
		// Unconverged-pixel counter: word 1 of the photon buffer.
		const uint32_t zero = 0u;
		glClearNamedBufferSubData(photon_data.getId(), GL_R32UI, sizeof(uint32_t), sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	}

	// Launch compute shader
	glBeginQuery(GL_TIME_ELAPSED, timerQueries[timerParity][0]);
	glDispatchCompute(groupCountX, groupCountY, 1);
	glEndQuery(GL_TIME_ELAPSED);
	// Make the image writes visible to the display fetch and next frame's imageLoad.
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Display result with display shader; the output holds `frame` accumulated samples.
	glBeginQuery(GL_TIME_ELAPSED, timerQueries[timerParity][1]);
	display(outputTexId);
	glEndQuery(GL_TIME_ELAPSED);

	// Read last frame's timers (this frame's are still in flight).
	if (timersPrimed)
	{
		GLuint64 traceNs = 0, displayNs = 0;
		glGetQueryObjectui64v(timerQueries[1 - timerParity][0], GL_QUERY_RESULT, &traceNs);
		glGetQueryObjectui64v(timerQueries[1 - timerParity][1], GL_QUERY_RESULT, &displayNs);
		// Ignore occasional bogus readings from the driver.
		if (traceNs < 10'000'000'000ull) gpuTraceMs = traceNs * 1e-6f;
		if (displayNs < 10'000'000'000ull) gpuDisplayMs = displayNs * 1e-6f;
	}
	timersPrimed = true;
	timerParity = 1 - timerParity;

	if (accumulate)
		frame++;
	if (adaptive)
	{
		// Same texture next frame; done once no pixel is still sampling.
		if (currentTime - startTime > 1.0f) // the shader stores nothing in the first second
		{
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			uint32_t unconverged = 0;
			glGetNamedBufferSubData(photon_data.getId(), sizeof(uint32_t), sizeof(uint32_t), &unconverged);
			lastUnconverged = int(unconverged);
			if (unconverged == 0)
			{
				renderDone = true;
				std::cout << "Render complete after " << frame - 1 << " frames" << std::endl;
			}
		}
		return;
	}
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
		caustic_data.bind();
		caustic_data.fillData(std::vector<uint32_t>(size_t(width) * height * 3, 0u));
		allocatePhotonMap();
		this->resetFrame();
	}
}

glm::vec3 Renderer::meanCenterRadiance(int r)
{
	// texture[1 - current] holds the most recent output (`current` flips after each frame);
	// with adaptive sampling it is texture[current], with per-pixel sample counts in alpha.
	if (noiseThreshold > 0.0f && accumulate)
	{
		std::vector<glm::vec4> bp(size_t(4 * r * r));
		glGetTextureSubImage(texture[current].getId(), 0, width / 2 - r, height / 2 - r, 0, 2 * r, 2 * r, 1,
			GL_RGBA, GL_FLOAT, GLsizei(bp.size() * sizeof(glm::vec4)), bp.data());
		glm::vec3 mean(0.0f);
		for (const auto& p : bp)
			mean += glm::vec3(p) / std::max(p.a, 1.0f);
		return mean / float(bp.size());
	}
	std::vector<glm::vec4> px(size_t(4 * r * r));
	glGetTextureSubImage(texture[1 - current].getId(), 0, width / 2 - r, height / 2 - r, 0, 2 * r, 2 * r, 1,
		GL_RGBA, GL_FLOAT, GLsizei(px.size() * sizeof(glm::vec4)), px.data());
	glm::vec3 sum(0.0f);
	for (const auto& p : px)
		sum += glm::vec3(p);
	// frame was incremented after the last dispatch; that output holds frame - 1 samples.
	int samples = accumulate ? frame - 1 : 1;
	return sum / float(px.size()) / float(std::max(samples, 1));
}

void Renderer::resetFrame()
{
	this->frame = 1;
	renderDone = false;
	lastUnconverged = -1;
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
	display_shader.initUniform("exposure");
	display_shader.initUniform("srgbOutput");
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
	compute_shader->initUniform("sphere_size");
	compute_shader->initUniform("tlas_size");
	compute_shader->initUniform("hdri");
	compute_shader->setUniform("hdri", 3);
	compute_shader->initUniform("prevFrame"); // Bind prevFrame to texture unit 2

	compute_shader->initUniform("accumulateBounces");
	compute_shader->initUniform("movingBounces");
	compute_shader->initUniform("accumulateSamples");
	compute_shader->initUniform("movingSamples");
	// --- FOV uniform ---
	compute_shader->initUniform("fov");
	compute_shader->initUniform("emissionScale");
	compute_shader->initUniform("debugView");
	compute_shader->initUniform("light_count");
	compute_shader->initUniform("light_node_count");
	compute_shader->initUniform("infinite_light_count");
	compute_shader->initUniform("useNEE");
	compute_shader->initUniform("neeDepth");
	compute_shader->initUniform("rrStart");
	for (const char* u : { "fogEnabled", "fogMin", "fogMax", "fogSigmaS", "fogSigmaA", "fogG" })
		compute_shader->initUniform(u);
	compute_shader->setUniform("fov", fov);
}

void Renderer::setupTextures()
{
	texture.reserve(3);
	for (int i = 0; i < 3; ++i) // ping-pong pair + adaptive sampling half buffer
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
