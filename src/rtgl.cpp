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
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	Scene scene1 = testSceneGLTF();
	BVH bvh1(scene1.triangles, 32);

	Scene* scenes[1] = { &scene1 };
	BVH* bvhs[1] = { &bvh1 };
	int currentScene = 0;

	Renderer renderer(window);
	renderer.setScene(*scenes[currentScene]);
	renderer.setBVH(*bvhs[currentScene]);
	renderer.init();

	float lastToggleTime = glfwGetTime();
	bool resetFrameRequested = false;
	bool accumulationEnabled = true;

	while (!glfwWindowShouldClose(window))
	{
		// timing stuff
		float currentTime = glfwGetTime();
		static float fpsTimer = currentTime;
		static float printTimer = currentTime;
		float dt = currentTime - fpsTimer;
		fpsTimer = currentTime;
		// imgui stuff
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Custom UI
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 300, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(300, io.DisplaySize.y), ImGuiCond_Always);
			ImGui::Begin("Renderer Stats", nullptr);
			ImGui::Text("FPS: %.1f", 1.0f / dt);
			resetFrameRequested = ImGui::Button("Reset Frame");
			ImGui::Checkbox("Enable Accumulation", &accumulationEnabled);
			ImGui::Separator();
			// Path tracing control sliders
			int accBounces = renderer.getAccumulateBounces();
			int movBounces = renderer.getMovingBounces();
			int accSamples = renderer.getAccumulateSamples();
			int movSamples = renderer.getMovingSamples();
			if (ImGui::InputInt("Accumulate Bounces", &accBounces, 1, 8))
				renderer.setAccumulateBounces(accBounces);
			if (ImGui::InputInt("Moving Bounces", &movBounces, 1, 8))
				renderer.setMovingBounces(movBounces);
			if (ImGui::InputInt("Accumulate Samples", &accSamples, 1, 8))
				renderer.setAccumulateSamples(accSamples);
			if (ImGui::InputInt("Moving Samples", &movSamples, 1, 8))
				renderer.setMovingSamples(movSamples);
			ImGui::Separator();
			// Camera controls
			float camSpeed = renderer.cam.getSpeed();
			if (ImGui::InputFloat("Camera Speed", &camSpeed, 0.1f, 1.0f, "%.2f"))
				renderer.cam.setSpeed(camSpeed);
			glm::vec3 camPos = renderer.cam.getPosition();
			if (ImGui::InputFloat3("Camera Position", &camPos.x, "%.2f"))
				renderer.cam.setPosition(camPos);
			// --- FOV slider ---
			float fov = renderer.getFov();
			int ifov = fov;
			if (ImGui::SliderInt("FOV (deg)", &ifov, 0, 128))
				renderer.setFov(float(ifov));
			ImGui::End();
		}


		if (currentTime - printTimer > 1.0f)
		{
			std::cout << "Frame rate: " << 1.0 / dt << "fps\n";
			std::cout << "Camera Pos: " << renderer.cam.getPosition().x << ","
				<< renderer.cam.getPosition().y << "," << renderer.cam.getPosition().z << "\n";
			renderer.handleResize();
			printTimer = currentTime;
		}

		if (resetFrameRequested)
		{
			renderer.resetFrame();
		}
		renderer.setAccumulation(accumulationEnabled);
		renderer.processInput();
		renderer.updateUniforms(currentTime);
		renderer.renderScene(currentTime, dt);

		// imgui stuff
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
