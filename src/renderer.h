#pragma once


#include "BVH.h"
#include "ShaderStorage.h"
#include "camera.h"
#include "framebuffer.h"
#include "primitives.h"
#include "texture.h"
#include "shader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <vector>

class Renderer {
public:
    Renderer(GLFWwindow* window, Scene& scene, BVH& bvh);
    ~Renderer();
    void init();
    void processInput();
    void updateUniforms(float currentTime);
    void renderScene(float currentTime, float dt);
    void handleResize();
    Camera cam;
private:
    GLFWwindow* window;
    Shader rt_shader;
    Shader display_shader;

    std::vector<Texture> texture;
    std::vector<FrameBuffer> fbo;
    int width, height;
    int current;
    int frame;
    double fpsTimer;
    double printTimer;
    float startTime;
    Scene& scene;
    BVH& bvh;
    ShaderStorage<Triangle> triangle_data;
    ShaderStorage<Material> mat_data;
    ShaderStorage<Sphere> sphere_data;
    ShaderStorage<BVHNode> bvh_data;
    ShaderStorage<int> indices;
    float delta;
    int dcounter;

    void printGLVersion();
    void setupShaders();
    void setupTextures();
    void setupFramebuffers();
};

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam);
