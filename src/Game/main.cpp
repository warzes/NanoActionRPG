﻿#include "NanoEngine.h"
#include "Example.h"
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	Example001();
	return 0;

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

	const std::vector<MeshVertex> verticesQuad =
	{
		MeshVertex(glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f)),
		MeshVertex(glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f)),
		MeshVertex(glm::vec3(0.5f, 0.0f,-0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f)),
		MeshVertex(glm::vec3(-0.5f, 0.0f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f)),
	};
	const std::vector<uint8_t> indicesQuad =
	{
		0, 1, 2, 2, 3, 0,
	};

	const char* mainVertSource = R"(
#version 460
#pragma debug(on)

out gl_PerVertex { vec4 gl_Position; };

out out_block
{
	vec3 pos;
	vec3 col;
	vec3 nrm;
	vec2 uvs;
} o;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;
layout (location = 2) in vec3 nrm;
layout (location = 3) in vec2 uvs;

layout (location = 0) uniform mat4 proj;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 modl;

void main()
{
	const vec4 mpos = (view * modl * vec4(pos, 1.0));
	o.pos = (modl * vec4(pos, 1.0)).xyz;
	o.col = col;
	o.nrm = mat3(transpose(inverse(modl))) * nrm;
	o.uvs = uvs;
	gl_Position = proj * mpos;
}
)";

	const char* mainFragSource = R"(
#version 460
#pragma debug(on)

in in_block
{
	vec3 pos;
	vec3 col;
	vec3 nrm;
	vec2 uvs;
} i;

layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D dif;
layout(binding = 1) uniform sampler2D spc;
layout(binding = 2) uniform sampler2D nrm;
layout(binding = 3) uniform sampler2D emi;

void main()
{
	outColor.rgb = texture(dif, i.uvs).rgb + i.pos - i.nrm;
	outColor.rgb = outColor.rgb * i.col;
	outColor.a = 1.0;
}
)";
	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.SetPosition({ 0.0f, 0.3f, -1.0f });

	glEnable(GL_DEPTH_TEST);

	GLTexture2DRef textureCubeDiffuse{ new GLTexture2D("data/textures/T_Default_D.png", STBI_rgb, true) };

	GLVertexArrayRef quadGeom{ new GLVertexArray(verticesQuad, indicesQuad, GetMeshVertexFormat()) };

	GLProgramPipelineRef ppMain{ new GLProgramPipeline(mainVertSource, mainFragSource) };

	while (!Window::ShouldClose())
	{
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

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			const float mouseSensitivity = 10.0f * deltaTime;
			const float moveSpeed = 10.0f * deltaTime;
			const glm::vec3 oldCameraPos = camera.position;

			auto change = Mouse::GetPosition() - lastMousePosition;

			if (Keyboard::IsPressed(GLFW_KEY_W)) camera.MoveBy(moveSpeed);
			if (Keyboard::IsPressed(GLFW_KEY_S)) camera.MoveBy(-moveSpeed);
			if (Keyboard::IsPressed(GLFW_KEY_A)) camera.StrafeBy(moveSpeed);
			if (Keyboard::IsPressed(GLFW_KEY_D)) camera.StrafeBy(-moveSpeed);

			if (Mouse::IsPressed(Mouse::Button::Right))
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Disabled);

				lastMousePosition = Mouse::GetPosition();

				if (change.x != 0.0f)  camera.RotateLeftRight(change.x * mouseSensitivity);
				if (change.y != 0.0f)  camera.RotateUpDown(-change.y * mouseSensitivity);
			}
			else
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Normal);
			}
		}

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
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::PlotHistogram("Fps graph", fpsArr, FPS_RECORD_ARR_SIZE, 0, 0, 0, 60);
			ImGui::Text("Fps %d", currentFps);

			ImGui::PlotHistogram("Mill second avg", millArr, FPS_RECORD_ARR_SIZE, 0, 0, 0, 30);
			ImGui::Text("Milliseconds: %f", currentMill);

			ImGui::PlotHistogram("Milli seconds graph", deltaTimeArr, DELTA_TIME_ARR_SIZE, 0, 0, 0, 30);

			ImGui::End();
		}
#pragma endregion

#pragma region render
		renderDurationProfiler.start();
		renderDurationProfilerFine.start();

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		textureCubeDiffuse->Bind(0);
		ppMain->Bind();
		ppMain->SetVertexUniform(0, perspective);
		ppMain->SetVertexUniform(1, camera.GetViewMatrix());
		ppMain->SetVertexUniform(2, glm::mat4(1.0f));

		quadGeom->Bind();

		glDrawElements(GL_TRIANGLES, indicesQuad.size(), GL_UNSIGNED_BYTE, nullptr);

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