#pragma once

#include "BVH.h"
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
	void setBVH(BVH &bvh);
	void init();
	void processInput();
	void updateUniforms(float currentTime);
	void renderScene(float currentTime, float dt);
	void handleResize();
	void resetFrame();
	void setAccumulation(bool accumulate);
	Camera cam;

private:
	GLFWwindow *window;
	Shader rt_shader;
	Shader display_shader;
	Shader *compute_shader; // Use pointer to avoid default constructor issue
	std::vector<Texture> texture;
	std::vector<FrameBuffer> fbo;
	int width, height;
	int current;
	int frame;
	double fpsTimer;
	double printTimer;
	float startTime;
	Scene *scene; // Now a pointer
	BVH *bvh;	  // Now a pointer
	ShaderStorage<Triangle> triangle_data;
	ShaderStorage<Material> mat_data;
	ShaderStorage<Sphere> sphere_data;
	ShaderStorage<BVHNode> bvh_data;
	ShaderStorage<int> indices;
	ShaderStorage<BkEndSimpleTriangle> simple_triangle_data;
	float delta;
	int dcounter;
	bool accumulate;
	float fov = 50.0f; // Default FOV in degrees

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
	// Setters
	void setAccumulateBounces(int val) { accumulateBounces = val; }
	void setMovingBounces(int val) { movingBounces = val; }
	void setAccumulateSamples(int val) { accumulateSamples = val; }
	void setMovingSamples(int val) { movingSamples = val; }
	void setFov(float val) { fov = val; }

	void printGLVersion();
	void setupShaders();
	void setupTextures();
	void setupFramebuffers();
};

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
								GLenum severity, GLsizei length,
								const GLchar *message, const void *userParam);
