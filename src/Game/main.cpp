#include "NanoEngine.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 1);
#if defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Game", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	gladLoadGL(glfwGetProcAddress);

	Renderer3D renderer;

#if defined(_DEBUG)
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	//glDebugMessageCallback(glDebugOutput, &renderer);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

	Print("OpenGL: OpenGL device information:");
	Print("    > Vendor:   " + std::string((const char*)glGetString(GL_VENDOR)));
	Print("    > Renderer: " + std::string((const char*)glGetString(GL_RENDERER)));
	Print("    > Version:  " + std::string((const char*)glGetString(GL_VERSION)));
	Print("    > GLSL:     " + std::string((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)));

#pragma region ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.IniFilename = nullptr;

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	//ImGuiStyle& style = ImGui::GetStyle();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion

#pragma region deltaTime
	int fpsCount = 0;
	float timeFpsCount = 0;
	int timeBeg = clock();
	int currentFps = 0;
	float currentMill = 0;
	const int FPS_RECORD_ARR_SIZE = 20;
	int recordPos = 0;
	float fpsArr[FPS_RECORD_ARR_SIZE] = { };
	float millArr[FPS_RECORD_ARR_SIZE] = { };
	const int DELTA_TIME_ARR_SIZE = 60;
	int recordPosDeltaTime = 0;
	float deltaTimeArr[DELTA_TIME_ARR_SIZE] = { };
#pragma endregion

#pragma region windowSize
	int windowWidth = 0;
	int windowHeight = 0;
#pragma endregion

	PL::ImguiProfiler renderDurationProfiler("Render duration (ms) (avg over one sec)", 0, 30);
	PL::ImguiProfiler renderDurationProfilerFine("Render duration (ms) ", 0, 30);
	PL::ImguiProfiler imguiRenderDuration("Imgui render duration (ms) ", 0, 30);
	PL::ImguiProfiler swapBuffersDuration("Swap buffers duration (ms) ", 0, 30);

	while (!glfwWindowShouldClose(window))
	{
#pragma region window event
		glfwPollEvents();
#pragma endregion

#pragma region window metrics
		//glfwGetWindowSize(window, &windowWidth, &windowHeight);
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

		windowWidth = std::max(windowWidth, 1);
		windowHeight = std::max(windowHeight, 1);
#pragma endregion

#pragma region deltatime
		int timeEnd = clock();
		float deltaTime = float(timeEnd - timeBeg) / 1000.f;
		timeBeg = clock();

		timeFpsCount += deltaTime;
		fpsCount++;
		if (timeFpsCount > 1)
		{
			timeFpsCount -= 1;
			std::string name = std::to_string(fpsCount);
			name = "fps: " + name;
			glfwSetWindowTitle(window, name.c_str());

			fpsCount = 0;
		}
#pragma endregion

#pragma region profiler ui code
		{
			static float fpsCounterFloat;
			static float currentFpsCounter;

			fpsCounterFloat += deltaTime;
			currentFpsCounter += 1;
			if (fpsCounterFloat >= 1)
			{
				fpsCounterFloat -= 1;
				currentFps = (int)currentFpsCounter;
				currentMill = (1.f / (float)currentFps) * 1000.f;
				currentFpsCounter = 0;

				if (recordPos < FPS_RECORD_ARR_SIZE)
				{
					fpsArr[recordPos] = (float)currentFps;
					millArr[recordPos] = currentMill;
					recordPos++;
				}
				else
				{
					for (int i = 0; i < FPS_RECORD_ARR_SIZE - 1; i++)
					{
						fpsArr[i] = fpsArr[i + 1];
						millArr[i] = millArr[i + 1];

					}
					fpsArr[FPS_RECORD_ARR_SIZE - 1] = (float)currentFps;
					millArr[FPS_RECORD_ARR_SIZE - 1] = currentMill;
				}

				//if (profileAveragePos < ProfilerAverage_ARR_SIZE)
				//{
				//
				//	profileAverageArr[profileAveragePos] = renderProfiler.getAverageAndResetData().timeSeconds * 1000;
				//	profileAveragePos++;
				//}
				//else
				//{
				//	for (int i = 0; i < ProfilerAverage_ARR_SIZE - 1; i++)
				//	{
				//		profileAverageArr[i] = profileAverageArr[i + 1];
				//	}
				//	profileAverageArr[ProfilerAverage_ARR_SIZE - 1] = renderProfiler.getAverageAndResetData().timeSeconds * 1000;
				//}

				renderDurationProfiler.updateValue(1000);
				imguiRenderDuration.updateValue(1000);
				swapBuffersDuration.updateValue(1000);
			}

			if (recordPosDeltaTime < DELTA_TIME_ARR_SIZE)
			{
				deltaTimeArr[recordPosDeltaTime] = deltaTime * 1000;
				recordPosDeltaTime++;
			}
			else
			{
				for (int i = 0; i < DELTA_TIME_ARR_SIZE - 1; i++)
				{
					deltaTimeArr[i] = deltaTimeArr[i + 1];
				}
				deltaTimeArr[DELTA_TIME_ARR_SIZE - 1] = deltaTime * 1000;
			}

			//if (profilePos < Profiler_ARR_SIZE)
			//{
			//	profileeArr[profilePos] = lastProfilerRezult.timeSeconds;
			//	profilePos++;
			//}
			//else
			//{
			//	for (int i = 0; i < Profiler_ARR_SIZE - 1; i++)
			//	{
			//		profileeArr[i] = profileeArr[i + 1];
			//	}
			//	profileeArr[Profiler_ARR_SIZE - 1] = lastProfilerRezult.timeSeconds;
			//}

			renderDurationProfilerFine.updateValue(1000);

		}
#pragma endregion

#pragma region imgui

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool showStats = true;

		//imgui main menu
		{
			ImGui::Begin("Menu");
			ImGui::SetWindowFontScale(1.2f);

			ImGui::Checkbox("Stats##check", &showStats);
			ImGui::NewLine();
			ImGui::Text("Settings");

			ImGui::End();
		}

		ImGuiWindowFlags flags = {};

		if (showStats)
		{
			ImGui::Begin("Stats", &showStats, flags);
			//ImGui::SetWindowFontScale(2.0f);

			ImGui::PlotHistogram("Fps graph", fpsArr, FPS_RECORD_ARR_SIZE, 0, 0,
				0, 60);
			ImGui::Text("Fps %d", currentFps);

			ImGui::PlotHistogram("Mill second avg", millArr, FPS_RECORD_ARR_SIZE, 0, 0,
				0, 30);
			ImGui::Text("Milliseconds: %f", currentMill);

			ImGui::PlotHistogram("Milli seconds graph", deltaTimeArr, DELTA_TIME_ARR_SIZE, 0, 0,
				0, 30);

			//ImGui::PlotHistogram("Frame duration graph", deltaTimeArr, DELTA_TIME_ARR_SIZE, 0, 0,
			//0, 0.32);
			//ImGui::Text("Frame duration (ms) %f", deltaTime * 1000);


			//ImGui::PlotHistogram("Render duration graph", profileeArr, Profiler_ARR_SIZE, 0, 0,
			//0, 0.32);
			//ImGui::Text("Render duration (ms) %f", lastProfilerRezult.timeSeconds * 1000);
			//renderDurationProfilerFine.imguiPlotValues();

			//renderDurationProfiler.imguiPlotValues();

			//imguiRenderDuration.imguiPlotValues();

			//swapBuffersDuration.imguiPlotValues();

			ImGui::End();
		}

		ImGui::Render();

#pragma endregion

#pragma region camera
#pragma endregion

#pragma region render

		renderDurationProfiler.start();
		renderDurationProfilerFine.start();

		glViewport(0, 0, windowWidth, windowHeight);
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		renderDurationProfiler.end();
		renderDurationProfilerFine.end();
#pragma endregion
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}
//-----------------------------------------------------------------------------