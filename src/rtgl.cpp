#include "renderer.h"
#include "testScene.h"

#include <iostream>

int main()
{
	if (!glfwInit())
	{
		return -1;
	}
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Path Tracer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}
	glfwSwapInterval(1);

	Scene scene1 = testSceneDragon();
	BVH bvh1(scene1.triangles, 4);
	Scene scene2 = testSceneBox();
	BVH bvh2(scene2.triangles, 4);

	Scene* scenes[2] = { &scene1, &scene2 };
	BVH* bvhs[2] = { &bvh1, &bvh2 };
	int currentScene = 0;

	Renderer renderer(window);
	renderer.setScene(*scenes[currentScene]);
	renderer.setBVH(*bvhs[currentScene]);
	renderer.init();

	float lastToggleTime = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float currentTime = glfwGetTime();
		static float fpsTimer = currentTime;
		static float printTimer = currentTime;
		float dt = currentTime - fpsTimer;
		fpsTimer = currentTime;

		// Toggle scene and BVH every 10 seconds
		if (currentTime - lastToggleTime > 1000000000.0f)
		{
			currentScene = 1 - currentScene;
			renderer.setScene(*scenes[currentScene]);
			renderer.setBVH(*bvhs[currentScene]);
			renderer.init();
			lastToggleTime = currentTime;
		}

		if (currentTime - printTimer > 1.0f)
		{
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
