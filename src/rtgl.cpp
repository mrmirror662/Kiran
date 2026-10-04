#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "renderer.h"
#include "testScene.h"
#include "GLContext.h"

#include <iostream>
#include <vector>
#include "tinygltf/stb_image_write.h"
#include <GLFW/glfw3.h>

int main(int argc, char** argv)
{
	if (!glfwInit())
	{
		return -1;
	}
	glctx::windowHints();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Path Tracer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	if (!glctx::init(window))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}
	std::cout << "GL backend: " << glctx::backendName() << std::endl;
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

	// --scene bistro (default) | teapots | nodes | simple | sponza
	std::string sceneName = argc > 2 && std::string(argv[1]) == "--scene" ? argv[2] : "bistro";
	Scene scene1 = sceneName == "sponza" ? testSceneGLTF()
		: sceneName == "simple" ? testSceneInstancing()
		: sceneName == "nodes" ? testSceneNodes()
		: sceneName == "teapots" ? testSceneTeapots()
		: sceneName == "furnace" ? testSceneFurnace(1.0f)
		: sceneName == "furnace-cm" ? testSceneFurnace(0.01f)
		: sceneName == "furnace-metal" ? testSceneFurnace(1.0f, 1.0f, 1.0f, 0.5f)
		: sceneName == "furnace-glossy" ? testSceneFurnace(1.0f, 1.0f, 0.0f, 0.3f, 1.5f)
		: sceneName == "furnace-mirror" ? testSceneFurnace(1.0f, 1.0f, 1.0f, 0.0f)
		: sceneName.rfind("light-", 0) == 0 ? testSceneLights(sceneName.substr(6))
		: sceneName.rfind("glass-", 0) == 0 ? testSceneGlassSlab(std::stof(sceneName.substr(6)))
		: sceneName.rfind("glassd-", 0) == 0 ? testSceneGlassSlab(std::stof(sceneName.substr(7)), 0.5f)
		: sceneName.rfind("fog-", 0) == 0 ? testSceneFog(sceneName.substr(4))
		: sceneName == "diamond" ? testSceneDiamond()
		: sceneName == "prism" ? testScenePrism()
		: sceneName == "prism-fog" ? testScenePrism(true)
		: sceneName == "rainbow-fog" ? testSceneRainbowFog()
		: sceneName == "diamond-rainbow" ? testSceneGemRainbow(2.417f, 55.3f)
		: sceneName == "diamond-box" ? testSceneDiamondBox()
		: sceneName == "moissanite-rainbow" ? testSceneGemRainbow(2.65f, 12.7f)
		: sceneName == "fogbow" ? testSceneFogbow(2.417f, 55.3f)
		: sceneName == "fogbow-moissanite" ? testSceneFogbow(2.65f, 12.7f)
		: sceneName == "lens" ? testSceneLens()
		: sceneName == "caustic" ? testSceneCaustic()
		: sceneName == "caustic-thru" ? testSceneCausticThrough()
		: sceneName == "wanderer" ? testSceneWanderer()
		: sceneName == "temple" ? testSceneTemple()
		: sceneName == "autumn" ? testSceneAutumn()
		: sceneName == "pavilion" ? testScenePavilion()
		: sceneName == "dragon" ? testSceneGLTFFramed("assets/DragonAttenuation/DragonAttenuation.gltf", true)
		: sceneName == "attenuation" ? testSceneGLTFFramed("assets/AttenuationTest/AttenuationTest.gltf")
		: testSceneBistro();
	for (int i = 1; i < argc; ++i)
		if (std::string(argv[i]) == "--no-dispersion") // non-spectral comparison runs
			for (auto& m : scene1.mats)
				m.dispersion = 0.0f;
	scene1.mergeUniqueInstances();
	AccelerationStructure accel1(scene1, 30); // depth cap keeps the 32-entry GPU traversal stacks safe

	Scene* scenes[1] = { &scene1 };
	AccelerationStructure* accels[1] = { &accel1 };
	LightTable lights1(scene1, accel1);
	int currentScene = 0;

	Renderer renderer(window);
	renderer.setScene(*scenes[currentScene]);
	renderer.setAccel(*accels[currentScene]);
	renderer.setLights(lights1);
	if (scene1.cameraYFov > 0.0f) // frame the shot as the file's camera does
		renderer.setFov(glm::degrees(scene1.cameraYFov));
	for (int i = 1; i < argc; ++i)
	{
		if (std::string(argv[i]) == "--no-nee")
			renderer.setUseNEE(false);
		if (std::string(argv[i]) == "--nee-depth" && i + 1 < argc)
			renderer.setNeeDepth(std::atoi(argv[i + 1]));
		if (std::string(argv[i]) == "--no-merging")
			renderer.setPhotonMerging(false);
		if (std::string(argv[i]) == "--photon-radius" && i + 1 < argc)
			renderer.setPhotonRadius(std::stof(argv[i + 1]));
		if (std::string(argv[i]) == "--no-caustics")
			renderer.setCausticTracing(false);
		if (std::string(argv[i]) == "--lt-direct")
			renderer.setLtDebugDirect(true);
		if (std::string(argv[i]) == "--lt-min" && i + 1 < argc)
			renderer.setLtMinDist(std::stof(argv[i + 1]));
		if (std::string(argv[i]) == "--max-spp" && i + 1 < argc)
			renderer.setAdaptiveMaxSpp(std::atoi(argv[i + 1]));
		if (std::string(argv[i]) == "--noise" && i + 1 < argc)
		{
			renderer.setNoiseThreshold(std::stof(argv[i + 1]));
		}
		if (std::string(argv[i]) == "--tile" && i + 1 < argc)
			renderer.setTileWidth(std::atoi(argv[i + 1]));
		if (std::string(argv[i]) == "--no-equiangular")
			renderer.setEquiangular(false);
		if (std::string(argv[i]) == "--emission" && i + 1 < argc)
			renderer.setEmissionScale(std::stof(argv[i + 1]));
		if (std::string(argv[i]) == "--rr" && i + 1 < argc)
			renderer.setRRStart(std::atoi(argv[i + 1]));
		if (std::string(argv[i]) == "--bounces" && i + 1 < argc)
		{
			renderer.setAccumulateBounces(std::atoi(argv[i + 1]));
			renderer.setMovingBounces(std::atoi(argv[i + 1]));
		}
	}
	// --shot <seconds> <file.png>: saves the displayed image (without the UI) and exits.
	float shotTime = -1.0f;
	std::string shotPath;
	for (int i = 1; i + 2 < argc; ++i)
		if (std::string(argv[i]) == "--shot")
		{
			shotTime = std::stof(argv[i + 1]);
			shotPath = argv[i + 2];
		}
	bool probeRadiance = sceneName.rfind("furnace", 0) == 0 || sceneName.rfind("light-", 0) == 0 || sceneName.rfind("glass-", 0) == 0 || sceneName.rfind("fog-", 0) == 0 || sceneName == "caustic" || sceneName == "caustic-thru" || sceneName.rfind("glassd-", 0) == 0;
	renderer.init();

	float lastToggleTime = glfwGetTime();
	const float startTime = lastToggleTime;
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
		// Frame time averaged over each ~1s window (ms is linear; fps is not).
		static float presentMs = 0.0f; // CPU: readback + window blit (Mesa backend) or swap
		static int windowFrames = 0;
		static float windowTime = 0.0f;
		static float avgFrameMs = 0.0f;
		windowFrames++;
		windowTime += dt;
		glctx::beginFrame();
		// imgui stuff
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Custom UI
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 300, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(300, io.DisplaySize.y), ImGuiCond_Always);
			ImGui::Begin("Renderer Stats", nullptr);
			ImGui::Text("Frame time: %.1f ms (%.1f fps)", avgFrameMs, avgFrameMs > 0.0f ? 1000.0f / avgFrameMs : 0.0f);
			ImGui::Text("GPU trace %.1f ms | display %.2f ms | present %.1f ms",
				renderer.getGpuTraceMs(), renderer.getGpuDisplayMs(), presentMs);
			int view = renderer.getDebugView();
			if (ImGui::Combo("View", &view, "Path traced\0Traversal cost\0"))
				renderer.setDebugView(view);
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
			ImGui::Separator();
			// Display transform (does not affect light transport or accumulation)
			float exposure = renderer.getExposure();
			if (ImGui::SliderFloat("Exposure (EV)", &exposure, -5.0f, 10.0f, "%.1f"))
				renderer.setExposure(exposure);
			bool nee = renderer.getUseNEE();
			if (ImGui::Checkbox("Light sampling (NEE + MIS)", &nee))
				renderer.setUseNEE(nee);
			int neeDepth = renderer.getNeeDepth();
			if (ImGui::SliderInt("Light sampling depth", &neeDepth, 1, 8))
				renderer.setNeeDepth(neeDepth);
			if (Fog* fog = renderer.getFog(); fog && fog->enabled)
			{
				// Grey fog controls (restart accumulation on change).
				float s = fog->sigma_s.x, a = fog->sigma_a.x, g = fog->g;
				bool changed = ImGui::SliderFloat("Fog scattering", &s, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
				changed |= ImGui::SliderFloat("Fog absorption", &a, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
				changed |= ImGui::SliderFloat("Fog anisotropy g", &g, -0.95f, 0.95f, "%.2f");
				if (changed)
				{
					fog->sigma_s = glm::vec3(s);
					fog->sigma_a = glm::vec3(a);
					fog->g = g;
					renderer.resetFrame();
				}
			}
			bool adaptive = renderer.getNoiseThreshold() > 0.0f;
			if (ImGui::Checkbox("Adaptive sampling", &adaptive))
				renderer.setNoiseThreshold(adaptive ? 0.05f : 0.0f);
			if (adaptive)
			{
				ImGui::ProgressBar(renderer.getAdaptiveProgress(), ImVec2(-1.0f, 0.0f),
					renderer.getRenderDone() ? "Render complete" : nullptr);
				float thr = renderer.getNoiseThreshold();
				if (ImGui::SliderFloat("Noise threshold", &thr, 0.001f, 1.0f, "%.4f", ImGuiSliderFlags_Logarithmic))
					renderer.setNoiseThreshold(thr);
				int minSpp = renderer.getAdaptiveMinSpp();
				if (ImGui::SliderInt("Min samples", &minSpp, 2, 256, "%d", ImGuiSliderFlags_Logarithmic))
					renderer.setAdaptiveMinSpp(minSpp);
				int maxSpp = renderer.getAdaptiveMaxSpp();
				if (ImGui::SliderInt("Max samples", &maxSpp, 16, 65536, "%d", ImGuiSliderFlags_Logarithmic))
					renderer.setAdaptiveMaxSpp(maxSpp);
				bool show = renderer.getShowAdaptive();
				if (ImGui::Checkbox("Show adaptive sampling", &show))
					renderer.setShowAdaptive(show);
			}
			bool caustics = renderer.getCausticTracing();
			if (ImGui::Checkbox("Caustic light tracing", &caustics))
				renderer.setCausticTracing(caustics);
			bool merging = renderer.getPhotonMerging();
			if (ImGui::Checkbox("Photon merging (caustics seen through glass)", &merging))
				renderer.setPhotonMerging(merging);
			float pr = renderer.getPhotonRadius();
			if (ImGui::SliderFloat("Photon radius (m)", &pr, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic))
				renderer.setPhotonRadius(pr);
			float ltMin = renderer.getLtMinDist();
			if (ImGui::SliderFloat("Light tracing min distance", &ltMin, 0.0f, 3.0f, "%.2f"))
				renderer.setLtMinDist(ltMin);
			bool equiangular = renderer.getEquiangular();
			if (ImGui::Checkbox("Equiangular fog sampling", &equiangular))
				renderer.setEquiangular(equiangular);
			int rrStart = renderer.getRRStart();
			if (ImGui::SliderInt("Russian roulette from bounce (-1 off)", &rrStart, -1, 8))
				renderer.setRRStart(rrStart);
			bool srgb = renderer.getSrgbOutput();
			if (ImGui::Checkbox("sRGB output", &srgb))
				renderer.setSrgbOutput(srgb);
			// Temporary: scales every emitter (lamps), restarts accumulation.
			float emissionScale = renderer.getEmissionScale();
			if (ImGui::SliderFloat("Emission scale", &emissionScale, 0.0f, 200.0f, "%.1f", ImGuiSliderFlags_Logarithmic))
				renderer.setEmissionScale(emissionScale);
			ImGui::End();
		}


		if (currentTime - printTimer > 1.0f)
		{
			avgFrameMs = 1000.0f * windowTime / windowFrames;
			windowFrames = 0;
			windowTime = 0.0f;
			std::cout << "Frame time: " << avgFrameMs << " ms (" << 1000.0f / avgFrameMs << " fps) | GPU trace "
				<< renderer.getGpuTraceMs() << " ms, display " << renderer.getGpuDisplayMs() << " ms, present " << presentMs << " ms" << std::endl;
			std::cout << "Camera Pos: " << renderer.cam.getPosition().x << ","
				<< renderer.cam.getPosition().y << "," << renderer.cam.getPosition().z << "\n";
			renderer.handleResize();
			if (probeRadiance)
			{
				glm::vec3 r = renderer.meanCenterRadiance(8);
				std::cout << "Probe mean radiance: " << r.x << ", " << r.y << ", " << r.z << std::endl;
			}
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
		if (shotTime >= 0.0f && currentTime - startTime >= shotTime)
		{
			GLint vp[4];
			glGetIntegerv(GL_VIEWPORT, vp);
			std::vector<unsigned char> px(size_t(vp[2]) * vp[3] * 3);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_UNSIGNED_BYTE, px.data());
			stbi_flip_vertically_on_write(1);
			stbi_write_png(shotPath.c_str(), vp[2], vp[3], 3, px.data(), vp[2] * 3);
			std::cout << "Saved " << shotPath << std::endl;
			break;
		}

		// imgui stuff
		{
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		double presentStart = glfwGetTime();
		glctx::present();
		presentMs = float((glfwGetTime() - presentStart) * 1000.0);
		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}
	}

	glctx::shutdown();
	glfwTerminate();
	return 0;
}
