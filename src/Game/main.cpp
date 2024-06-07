#include "NanoEngine.h"
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

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

	PL::ImguiProfiler renderDurationProfiler("Render duration (ms) (avg over one sec)", 0, 30);
	PL::ImguiProfiler renderDurationProfilerFine("Render duration (ms) ", 0, 30);
	PL::ImguiProfiler imguiRenderDuration("Imgui render duration (ms) ", 0, 30);
	PL::ImguiProfiler swapBuffersDuration("Swap buffers duration (ms) ", 0, 30);

	while (!Window::ShouldClose())
	{
		Window::Update();

#pragma region deltatime
		int timeEnd = clock();
		float deltaTime = float(timeEnd - timeBeg) / 1000.f;
		timeBeg = clock();

		timeFpsCount += deltaTime;
		fpsCount++;
		if (timeFpsCount > 1)
		{
			timeFpsCount -= 1;
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

		IMGUI::Update();

		{
			ImGui::Begin("Stats");
			ImGui::PlotHistogram("Fps graph", fpsArr, FPS_RECORD_ARR_SIZE, 0, 0,
				0, 60);
			ImGui::Text("Fps %d", currentFps);

			ImGui::PlotHistogram("Mill second avg", millArr, FPS_RECORD_ARR_SIZE, 0, 0,
				0, 30);
			ImGui::Text("Milliseconds: %f", currentMill);

			ImGui::PlotHistogram("Milli seconds graph", deltaTimeArr, DELTA_TIME_ARR_SIZE, 0, 0,
				0, 30);
			ImGui::End();
		}
#pragma endregion

#pragma region render
		renderDurationProfiler.start();
		renderDurationProfilerFine.start();

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		IMGUI::Draw();

		Window::Swap();

		renderDurationProfiler.end();
		renderDurationProfilerFine.end();
#pragma endregion
	}

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}
//-----------------------------------------------------------------------------