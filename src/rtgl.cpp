#include "testScene.h"
#include "renderer.h"

#include <iostream>

int main() {
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Path Tracer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }
    glfwSwapInterval(1);

    Scene scene = testSceneBox();
    BVH bvh(scene.triangles, 4);

    Renderer renderer(window, scene, bvh);
    renderer.init();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        static float fpsTimer = currentTime;
        static float printTimer = currentTime;
        float dt = currentTime - fpsTimer;
        fpsTimer = currentTime;

        if (currentTime - printTimer > 1.0f) {
            std::cout << "Frame rate: " << 1.0 / dt << "fps\n";
            std::cout << "Camera Pos: " << renderer.cam.getPosition().x << ","
                      << renderer.cam.getPosition().y << "," << renderer.cam.getPosition().z << "\n";
            renderer.handleResize();
            printTimer = currentTime;
        }

        renderer.processInput();
        renderer.updateUniforms(currentTime);
        renderer.renderScene(currentTime, dt);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
