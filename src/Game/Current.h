#pragma once

https://github.com/timurson/DeferredShading
https://github.com/swq0553/DeferredShading

void Example00X()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthMask(GL_TRUE);


	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.SetPosition({ 0.0f, 0.3f, -1.0f });

	const float INITIAL_POINT_LIGHT_RADIUS = 0.663f;
	const unsigned int LIGHT_GRID_WIDTH = 10;  // point light grid size
	const unsigned int LIGHT_GRID_HEIGHT = 3;  // point light vertical grid height

	bool enableShadows = true;
	bool drawPointLights = false;
	bool showDepthMap = false;
	bool drawPointLightsWireframe = true;
	glm::vec3 diffuseColor = glm::vec3(0.847f, 0.52f, 0.19f);
	glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
	float glossiness = 16.0f;
	float gLinearAttenuation = 0.09f;
	float gQuadraticAttenuation = 0.032f;
	float pointLightIntensity = 0.736f;
	float pointLightRadius = INITIAL_POINT_LIGHT_RADIUS;
	float pointLightVerticalOffset = 0.636f;
	float pointLightSeparation = 0.670f;
	const int totalLights = LIGHT_GRID_WIDTH * LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT;

	class SceneLight
	{
	public:
		SceneLight(const glm::vec3& _position, const glm::vec3& _color, float _radius) : position(_position), color(_color), radius(_radius) {}
		glm::vec3 position; // world light position
		glm::vec3 color; // light's color
		float radius; // light's radius
	};

	SceneLight globalLight(glm::vec3(-2.5f, 5.0f, -1.25f), glm::vec3(1.0f, 1.0f, 1.0f), 0.125f);


	UtilsExample::SimpleShadowMapPass simpleShadowMapFB;
	simpleShadowMapFB.Create(UtilsExample::SHADOW_WIDTH, UtilsExample::SHADOW_HEIGHT);


	UtilsExample::GBufferRef gbuffer{ new UtilsExample::GBuffer(Window::GetWidth(), Window::GetHeight()) };
		
	UtilsExample::DeferredLightingPassFB lightingPassFB;
	lightingPassFB.Create(Window::GetWidth(), Window::GetHeight());



	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };
	//ModelRef model{ new Model("Data/Models/sponza2/sponza.obj") };
	//ModelRef model{ new Model("Data/Models/holodeck/holodeck.obj") };
	//ModelRef model{ new Model("Data/Models/lost-empire/lost_empire.obj") };
	//ModelRef model{ new Model("Data/Models/sibenik/sibenik.obj") };

	ModelRef model2{ new Model("Data/Models/Dragon.obj") };

#pragma region lighting info
	const unsigned int NR_LIGHTS = 64;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(123);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);

		//auto bmin = glm::abs(model->GetBounding().min);
		//auto bsize = bmin + glm::abs(model->GetBounding().max);
		//float xPos = static_cast<float>(rand() % (int)bsize.x - bmin.x);
		//float yPos = static_cast<float>(rand() % (int)bsize.y - bmin.y);
		//float zPos = static_cast<float>(rand() % (int)bsize.z - bmin.z);

		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

#pragma endregion

	while (!Window::ShouldClose())
	{
#pragma region deltatime
		int timeEnd = clock();
		float deltaTime = float(timeEnd - timeBeg) / 1000.f;
		timeBeg = clock();
#pragma endregion

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
			gbuffer->Resize(Window::GetWidth(), Window::GetHeight());
			lightingPassFB.Resize(Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			const float mouseSensitivity = 100.0f * deltaTime;
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

#pragma region imgui

		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::End();
		}
#pragma endregion

#pragma region render
		glEnable(GL_DEPTH_TEST);

		// 1. render depth of scene to texture (from light's perspective)
		{
			glm::mat4 lightProjection, lightView;
			glm::mat4 lightSpaceMatrix;
			float zNear = 1.0f, zFar = 10.0f;

			if (enableShadows) 
			{
				lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, zNear, zFar);
				lightView = glm::lookAt(globalLight.position, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
				lightSpaceMatrix = lightProjection * lightView;
				// render scene from light's point of view

				simpleShadowMapFB.Bind();
				simpleShadowMapFB.program->SetVertexUniform(0, lightSpaceMatrix);

				// DRAW MODEL
				glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
				glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));

				simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
				model2->Draw(simpleShadowMapFB.program);
			}
		}

		// 2. geometry pass: render scene's geometry/color data into gbuffer
		{
			gbuffer->BindForWriting();
			gbuffer->GetProgram()->SetVertexUniform(0, perspective);
			gbuffer->GetProgram()->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer->GetProgram()->SetVertexUniform(2, glm::mat4(1.0f));
			model->Draw(gbuffer->GetProgram());
				
			glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
			glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));

			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			model2->Draw(gbuffer->GetProgram());
		}

		// 3. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content and shadow map
		{
			lightingPassFB.Bind();
			lightingPassFB.program->SetFragmentUniform(0, camera.position);
			// send light relevant uniforms
			for (unsigned int i = 0; i < lightPositions.size(); i++)
			{
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].position"), lightPositions[i]);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].color"), lightColors[i]);
				// update attenuation parameters and calculate radius
				const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
				const float linear = 0.7f;
				const float quadratic = 1.8f;
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].linear"), linear);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].quadratic"), quadratic);
				// then calculate radius of light volume/sphere
				const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].x, lightColors[i].y), lightColors[i].z);
				float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].radius"), radius);
			}

			gbuffer->BindForReading();
			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Main frame
		{
			glDisable(GL_DEPTH_TEST);
			Renderer::MainFrameBuffer();
			Renderer::BlitFrameBuffer(lightingPassFB.fbo, nullptr,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	gbuffer.reset();
	lightingPassFB.Destroy();

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}