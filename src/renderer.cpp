#include "renderer.h"
#include <iostream>

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    switch (severity) {
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
    switch (type) {
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
    if (error) {
        throw std::runtime_error(message);
    }
}

Renderer::Renderer(GLFWwindow* window, Scene& scene, BVH& bvh)
    : window(window), rt_shader("shaders/rt.glsl", "shaders/vert.glsl"), display_shader("shaders/display.glsl", "shaders/vert.glsl"), cam({0, 0, -1.0f}), scene(scene), bvh(bvh) {
    glfwGetFramebufferSize(window, &width, &height);
    current = 0;
    frame = 1;
    fpsTimer = 0.0;
    printTimer = 0.0;
    startTime = glfwGetTime();
    delta = 0;
    dcounter = 0;
}

Renderer::~Renderer() {
    // Clean up resources if necessary
}

void Renderer::init() {
    printGLVersion();
    glViewport(0, 0, width, height);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    setupShaders();
    setupTextures();
    setupFramebuffers();

    bvh = BVH(scene.triangles, 4);

    triangle_data.bind();
    triangle_data.fillData(scene.triangles);
    triangle_data.bindBase(6);

    mat_data.bind();
    mat_data.fillData(scene.mats);
    mat_data.bindBase(7);

    sphere_data.bind();
    sphere_data.fillData(scene.spheres);
    sphere_data.bindBase(8);

    bvh_data.bind();
    bvh_data.fillData(bvh.nodes);
    bvh_data.bindBase(9);

    indices.bind();
    indices.fillData(bvh.triangleIndices);
    indices.bindBase(10);

    Texture::setActiveUnit(4);
    Texture env(scene.hdr.width, scene.hdr.height, scene.hdr.data);
    rt_shader.bind();
    rt_shader.initUniForm("hdri");
    rt_shader.setUniform("hdri", 4);
    Texture::setActiveUnit(1);
}

void Renderer::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void Renderer::updateUniforms(float currentTime) {
    rt_shader.bind();
    rt_shader.setUniform("iResolution", (float)width, (float)height);
    rt_shader.setUniform("iTime", currentTime - startTime);
    rt_shader.setUniform("iFrame", frame);
    rt_shader.setUniform("delta", delta);
    rt_shader.setUniform("camera_pos", cam.getPosition().x, cam.getPosition().y, cam.getPosition().z);
    rt_shader.setUniform("angle_offset", cam.getAngleOffset().x, cam.getAngleOffset().y);
    rt_shader.setUniform("triangle_size", (int)scene.triangles.size());
    rt_shader.setUniform("sphere_size", (int)scene.spheres.size());
    rt_shader.setUniform("bvh_size", (int)bvh.nodes.size());
}

void Renderer::renderScene(float currentTime, float dt) {
    this->cam.onUpdate(window, dt);
    fbo[current].bind();
    texture[1 - current].bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glFinish();
    frame++;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    display_shader.bind();
    display_shader.setUniform("iResolution", (float)width, (float)height);
    display_shader.setUniform("iFrame", frame);
    display_shader.setUniform("delta", delta);
    display_shader.setUniform("textureSampler", 1);

    texture[current].bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    current = 1 - current;
}

void Renderer::handleResize() {
    int current_width, current_height;
    glfwGetWindowSize(window, &current_width, &current_height);
    if (current_width != width || current_height != height) {
        width = current_width;
        height = current_height;
        glViewport(0, 0, width, height);
        for (auto& t : texture) {
            t.reSize(width, height);
        }
        for (int i = 0; i < fbo.size(); ++i) {
            fbo[i].attachTexure(texture[i]);
        }
    }
}

void Renderer::printGLVersion() {
    char* glVersion = (char*)glGetString(GL_VERSION);
    char* glVendor = (char*)glGetString(GL_VENDOR);
    char* glRenderer = (char*)glGetString(GL_RENDERER);
    std::cout << "GL Version: " << glVersion << "\n";
    std::cout << "GL Vendor: " << glVendor << "\n";
    std::cout << "GL Renderer: " << glRenderer << "\n";
}

void Renderer::setupShaders() {
    rt_shader.bind();
    rt_shader.initUniForm("iResolution");
    rt_shader.initUniForm("iTime");
    rt_shader.initUniForm("iFrame");
    rt_shader.initUniForm("camera_pos");
    rt_shader.initUniForm("angle_offset");
    rt_shader.initUniForm("delta");
    rt_shader.initUniForm("triangle_size");
    rt_shader.initUniForm("sphere_size");
    rt_shader.initUniForm("bvh_size");
    rt_shader.initUniForm("screenTexture");
    rt_shader.setUniform("screenTexture", 1);

    display_shader.bind();
    display_shader.initUniForm("iResolution");
    display_shader.initUniForm("iFrame");
    display_shader.initUniForm("delta");
    display_shader.initUniForm("textureSampler");
    display_shader.setUniform("textureSampler", 1);
}

void Renderer::setupTextures() {
    texture.reserve(2);
    for (int i = 0; i < 2; ++i) {
        texture.push_back(Texture(width, height));
    }
}

void Renderer::setupFramebuffers() {
    fbo.resize(2);
    for (int i = 0; i < 2; ++i) {
        fbo[i].bind();
        fbo[i].attachTexure(texture[i]);
    }
}
