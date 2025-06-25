#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "renderer.h"
#include "testScene.h"

#include <iostream>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

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
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	Scene scene1 = testSceneBox();
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
		//imgui stuff
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

		}
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

		//imgui stuff
		{
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}


	}

	glfwTerminate();
	return 0;
}
