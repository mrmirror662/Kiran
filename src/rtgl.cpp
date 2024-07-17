#define CE_SERIAL_IMPLEMENTATION

#include "BVH.h"
#include "ShaderStorage.h"
#include "camera.h"
#include "framebuffer.h"
#include "primitives.h"
#include "testScene.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <serial/ceSerial.h>
#include <vector>

#include "shader.h"

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
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

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void printGLVersion() {
  char *glVersion = (char *)glGetString(GL_VERSION);
  char *glVendor = (char *)glGetString(GL_VENDOR);
  char *glRenderer = (char *)glGetString(GL_RENDERER);
  std::cout << "GL Version:" << glVersion << "\n";
  std::cout << "GL Vendor:" << glVendor << "\n";
  std::cout << "GL Renderer:" << glRenderer << "\n";
}

int main(void) {
  GLFWwindow *window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(1280, 700, "Path Tracer", NULL, NULL);

  // Successfully loaded OpenGL
  // printf("Loaded OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version),
  // GLAD_VERSION_MINOR(version));
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
  /* Make the window's context current */
  glfwMakeContextCurrent(window);
  int version = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(0);

  printGLVersion();
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  std::cout << "width: " << width << " height: " << height << '\n';
  glViewport(0, 0, width, height);
  if (version == 0) {
    std::cout << "Failed to initialize OpenGL context\n";
    return -1;
  }
  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  auto rt_shader = Shader("shaders/rt.glsl", "shaders/vert.glsl");
  auto display_shader = Shader("shaders/display.glsl", "shaders/vert.glsl");

  int maxTextureUnits = 0;

  // Query the maximum number of combined texture image units
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
  std::cout << "Maximum supported texture units: " << maxTextureUnits
            << std::endl;

  if (maxTextureUnits < 2) {
    std::cerr << "Error: Not enough texture units supported by the hardware."
              << std::endl;
    return -1;
  }

  int current = 0; // Current index for reading from texture
  std::vector<Texture> texture;
  const int fbo_tex = 1;
  Texture::setActiveUnit(fbo_tex);
  std::vector<FrameBuffer> fbo;
  fbo.resize(2);
  // Generate Framebuffers and Textures
  for (int i = 0; i < 2; ++i) {
    texture.push_back(Texture(width, height));
    fbo[i].bind();
    fbo[i].attachTexure(texture[i]);
  }

  if (rt_shader.getId() == -1 || display_shader.getId() == -1)
    return 0;
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

  rt_shader.setUniform("screenTexture", fbo_tex);

  display_shader.bind();

  display_shader.initUniForm("iResolution");
  display_shader.initUniForm("iFrame");
  display_shader.initUniForm("delta");
  display_shader.initUniForm("textureSampler");

  display_shader.setUniform("textureSampler", fbo_tex);

  float startTime = glfwGetTime();
  int frame = 1;
  double fpsTimer = 0.0;
  int frameCounter = 0;
  double fps = 0;
  double printTimer = startTime;
  Camera cam({0, 0, -1.0f});
  /* Loop until the user closes the window */
  auto start_pos = cam.getPosition();

  start_pos.x += cam.getAngleOffset().x;
  start_pos.y += cam.getAngleOffset().y;
  float pd = 0;
  int dcounter = 0;

  // auto scene = testSceneBox();
  auto scene = testSceneBox();
  auto scene_copy = scene;
  std::cout << "T size:" << scene.triangles.size() << "\n";
  std::cout << "S size:" << scene.spheres.size() << "\n";
  std::cout << "M size:" << scene.mats.size() << "\n";
  BVH bvh(scene.triangles, 4);

  ShaderStorage<Triangle> triangle_data;
  triangle_data.bind();
  triangle_data.fillData(scene.triangles);
  triangle_data.bindBase(6);

  ShaderStorage<Material> mat_data;
  mat_data.bind();
  mat_data.fillData(scene.mats);
  mat_data.bindBase(7);

  ShaderStorage<Sphere> sphere_data;
  sphere_data.bind();
  sphere_data.fillData(scene.spheres);
  sphere_data.bindBase(8);

  ShaderStorage<BVHNode> bvh_data;
  bvh_data.bind();
  bvh_data.fillData(bvh.nodes);
  bvh_data.bindBase(9);

  ShaderStorage<int> indices;
  indices.bind();
  indices.fillData(bvh.triangleIndices);
  indices.bindBase(10);
  std::cout << "bvh size:" << bvh.nodes.size() << "\n";

  Texture::setActiveUnit(4);
  Texture env(scene.hdr.width, scene.hdr.height, scene.hdr.data);
  rt_shader.bind();
  rt_shader.initUniForm("hdri");
  rt_shader.setUniform("hdri", 4);
  Texture::setActiveUnit(fbo_tex);

  while (!glfwWindowShouldClose(window)) {
    float currentTime = glfwGetTime();
    float dt = currentTime - fpsTimer;
    fps = 1 / dt;

    if (currentTime - printTimer > 1.0f) {
      std::cout << "Frame rate:" << fps << "fps\n";
      std::cout << "Camera Pos:" << cam.getPosition().x << ","
                << cam.getPosition().y << "," << cam.getPosition().z << "\n";
      int current_width, current_height;
      glfwGetWindowSize(window, &current_width, &current_height);
      if (current_width != width || current_height != height) {
        std::cout << "window resized";
        width = current_width;
        height = current_height;
        glViewport(0, 0, width, height);
        for (auto &t : texture)
          t.reSize(width, height);
        for (int i = 0; i < fbo.size(); i++)
          fbo[i].attachTexure(texture[i]);
      }
      printTimer = currentTime;
    }
    fpsTimer = currentTime;

    auto current_pos = cam.getPosition();

    current_pos.x += cam.getAngleOffset().x;
    current_pos.y += cam.getAngleOffset().y;

    float delta = glm::length(current_pos - start_pos);
    start_pos = current_pos;

    if (delta > 0.00001) {
      dcounter = (1.0 / dt) / 2;
      frame = 1;
    }
    if (dcounter > 0) {
      delta = 0.0001;
      dcounter--;
      frame = 1;
    }

    fbo[current].bind();
    rt_shader.bind();

    rt_shader.setUniform("iResolution", (float)width, (float)height);

    float iTime = currentTime - startTime;
    rt_shader.setUniform("iTime", iTime);
    rt_shader.setUniform("iFrame", frame);
    rt_shader.setUniform("delta", delta);
    rt_shader.setUniform("camera_pos", cam.getPosition().x, cam.getPosition().y,
                         cam.getPosition().z);
    rt_shader.setUniform("angle_offset", cam.getAngleOffset().x,
                         cam.getAngleOffset().y);
    rt_shader.setUniform("triangle_size", (int)scene.triangles.size());
    rt_shader.setUniform("sphere_size", (int)scene.spheres.size());
    rt_shader.setUniform("bvh_size", (int)bvh.nodes.size());
    rt_shader.setUniform("hdri", 4);
    rt_shader.setUniform("screenTexture", fbo_tex);

    texture[1 - current].bind(); // Render to the other texture
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glFinish();
    frame++;
    // Swap Framebuffers
    current = 1 - current;

    // Rendering from texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    display_shader.bind();

    display_shader.setUniform("iResolution", (float)width, (float)height);
    display_shader.setUniform("iFrame", frame);
    display_shader.setUniform("delta", delta);
    display_shader.setUniform("textureSampler", fbo_tex);

    texture[current].bind(); // Reading from the texture we rendered to
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    cam.onUpdate(window, dt);
    frameCounter++;

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
