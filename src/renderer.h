#pragma once

#include "AccelerationStructure.h"
#include "LightTable.h"
#include "ShaderStorage.h"
#include "camera.h"
#include "framebuffer.h"
#include "primitives.h"
#include "texture.h"
#include "shader.h"
#include "BindlessTexture.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <vector>
struct BkEndSimpleTriangle
{
	alignas(16) glm::vec3 v0;
	alignas(16) glm::vec3 v1;
	alignas(16) glm::vec3 v2;
};
class Renderer
{
public:
	Renderer(GLFWwindow *window);
	~Renderer();
	void setScene(Scene &scene);
	void setAccel(AccelerationStructure &accel);
	void setLights(LightTable &lights);
	void init();
	void processInput();
	void updateUniforms(float currentTime);
	void renderScene(float currentTime, float dt);
	void handleResize();
	void resetFrame();
	void setAccumulation(bool accumulate);
	// Debug: mean accumulated radiance of a (2r x 2r) block at the image centre.
	glm::vec3 meanCenterRadiance(int r);
	Camera cam;

private:
	GLFWwindow *window;
	Shader rt_shader;
	Shader display_shader;
	Shader *compute_shader; // Use pointer to avoid default constructor issue
	Shader *light_shader;	// light-tracing variant of rt.comp (caustic paths)
	std::vector<Texture> texture;
	std::vector<FrameBuffer> fbo;
	GLuint fullscreenVao = 0; // attribute-less fullscreen draw; core profile requires a bound VAO
	int width, height;
	int current;
	int frame;
	double fpsTimer;
	double printTimer;
	float startTime;
	Scene *scene; // Now a pointer
	AccelerationStructure *accel;
	LightTable *lightTable = nullptr;
	ShaderStorage<Triangle> triangle_data;
	ShaderStorage<Material> mat_data;
	ShaderStorage<Sphere> sphere_data;
	ShaderStorage<GPUBVHNode> blas_data;
	ShaderStorage<GPUBVHNode> tlas_data;
	ShaderStorage<GPUInstance> instance_data;
	ShaderStorage<glm::vec4> triangle_vert_data;
	ShaderStorage<GPULight> light_data;
	ShaderStorage<GPULightNode> light_node_data;
	ShaderStorage<int> light_index_data;
	ShaderStorage<float> emit_cdf_data;
	ShaderStorage<uint32_t> caustic_data; // per-pixel RGB fixed-point splats from light tracing
	ShaderStorage<uint32_t> photon_data;   // photon count, hash-grid heads, photon records (rt.comp PhotonMap)
	ShaderStorage<int> delta_light_data;
	ShaderStorage<int> indices;
	ShaderStorage<BkEndSimpleTriangle> simple_triangle_data;
	float delta;
	int dcounter;
	bool accumulate;
	float fov = 50.0f; // Default FOV in degrees
	float exposure = 0.0f; // display exposure in EV stops
	bool srgbOutput = true;
	float emissionScale = 4.0f; // temporary: global emitter multiplier
	int debugView = 0;
	bool useNEE = true; // light sampling + MIS
	int neeDepth = 1;	// light-sample the first N path vertices
	int rrStart = 1;	// Russian roulette from this vertex on, -1: off
	bool causticTracing = true; // light-trace light->glass->(surface|fog) paths
	bool photonMerging = true;	// caustics seen through glass: progressive photon merging
	float photonRadius0 = 0.01f; // initial merge radius (m); shrinks as k^(-1/8) over frames
	static constexpr uint32_t kPhotonGridSize = 1u << 20;
	static constexpr uint32_t kPhotonWords = 13;
	void allocatePhotonMap();
	bool equiangular = true;	// equiangular fog sampling for direct light
	int tileWidth = 0;			// thread-group tiling width in groups (0: off)
	float noiseThreshold = 0.0f; // adaptive sampling: Cycles noise threshold (0: off)
	int adaptiveMinSpp = 16;	// samples before a pixel may converge
	int adaptiveMaxSpp = 4096;	// samples after which a pixel stops regardless
	bool renderDone = false;	// adaptive: every pixel has stopped
	int lastUnconverged = -1;	// adaptive: pixels still sampling after the last frame
	bool showAdaptive = false;	// display the adaptive sampling heat map
	bool ltDebugDirect = false; // debug: light tracing does direct light only, camera pass disabled
	float ltMinDist = 0.5f;		// light tracing skips points closer than this to the camera
	// GPU timers (double-buffered so reading never stalls): [frame parity][trace, display]
	GLuint timerQueries[2][2] = {};
	int timerParity = 0;
	bool timersPrimed = false;
	float gpuTraceMs = 0.0f, gpuDisplayMs = 0.0f;

	// --- Path tracing control uniforms ---
	int accumulateBounces = 2;
	int movingBounces = 2;
	int accumulateSamples = 1;
	int movingSamples = 1;

public:
	// Getters
	int getAccumulateBounces() const { return accumulateBounces; }
	int getMovingBounces() const { return movingBounces; }
	int getAccumulateSamples() const { return accumulateSamples; }
	int getMovingSamples() const { return movingSamples; }
	float getFov() const { return fov; }
	Fog* getFog() { return scene ? &scene->fog : nullptr; }
	float getExposure() const { return exposure; }
	bool getSrgbOutput() const { return srgbOutput; }
	float getEmissionScale() const { return emissionScale; }
	int getDebugView() const { return debugView; }
	bool getUseNEE() const { return useNEE; }
	int getNeeDepth() const { return neeDepth; }
	int getRRStart() const { return rrStart; }
	bool getCausticTracing() const { return causticTracing; }
	bool getEquiangular() const { return equiangular; }
	float getGpuTraceMs() const { return gpuTraceMs; }
	float getGpuDisplayMs() const { return gpuDisplayMs; }
	// Setters
	void setAccumulateBounces(int val) { accumulateBounces = val; }
	void setMovingBounces(int val) { movingBounces = val; }
	void setAccumulateSamples(int val) { accumulateSamples = val; }
	void setMovingSamples(int val) { movingSamples = val; }
	void setFov(float val) { fov = val; }
	void setExposure(float val) { exposure = val; }
	void setSrgbOutput(bool val) { srgbOutput = val; }
	void setEmissionScale(float val) { emissionScale = val; resetFrame(); }
	void setDebugView(int val) { debugView = val; resetFrame(); }
	void setUseNEE(bool val) { useNEE = val; resetFrame(); }
	void setNeeDepth(int val) { neeDepth = val; resetFrame(); }
	void setRRStart(int val) { rrStart = val; resetFrame(); }
	void setCausticTracing(bool val) { causticTracing = val; resetFrame(); }
	void setPhotonMerging(bool val) { photonMerging = val; resetFrame(); }
	bool getPhotonMerging() const { return photonMerging; }
	void setPhotonRadius(float val) { photonRadius0 = val; resetFrame(); }
	float getPhotonRadius() const { return photonRadius0; }
	void setEquiangular(bool val) { equiangular = val; resetFrame(); }
	void setTileWidth(int val) { tileWidth = val; }
	int getTileWidth() const { return tileWidth; }
	void setNoiseThreshold(float val) { noiseThreshold = val; resetFrame(); }
	float getNoiseThreshold() const { return noiseThreshold; }
	void setAdaptiveMinSpp(int val) { adaptiveMinSpp = val; resetFrame(); }
	int getAdaptiveMinSpp() const { return adaptiveMinSpp; }
	void setAdaptiveMaxSpp(int val) { adaptiveMaxSpp = std::max(val, 2); resetFrame(); }
	int getAdaptiveMaxSpp() const { return adaptiveMaxSpp; }
	bool getRenderDone() const { return renderDone; }
	// Fraction of pixels that have stopped sampling.
	float getAdaptiveProgress() const
	{
		if (renderDone)
			return 1.0f;
		return lastUnconverged < 0 ? 0.0f : 1.0f - float(lastUnconverged) / float(std::max(width * height, 1));
	}
	void setShowAdaptive(bool val) { showAdaptive = val; }
	bool getShowAdaptive() const { return showAdaptive; }
	void applyTraceUniforms(Shader &s, float currentTime);
	void setLtDebugDirect(bool v) { ltDebugDirect = v; }
	float getLtMinDist() const { return ltMinDist; }
	void setLtMinDist(float v) { ltMinDist = v; resetFrame(); }

	void printGLVersion();
	void setupShaders();
	void setupTextures();
	void setupFramebuffers();
};

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
								GLenum severity, GLsizei length,
								const GLchar *message, const void *userParam);
