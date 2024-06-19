#pragma once

const float INITIAL_POINT_LIGHT_RADIUS = 1.663f;
const unsigned int LIGHT_GRID_WIDTH = 10;  // point light grid size
const unsigned int LIGHT_GRID_HEIGHT = 3;  // point light vertical grid height

struct InstanceData {
	glm::vec4 instanceParam;
	glm::mat4 instanceMatrix;
};

// Node: separation < 1.0 will cause lights to penetrate each other, and > 1.0 they will separate (1.0 is just touching)
void configurePointLights(std::vector<InstanceData>& modelData, float radius, float separation, float yOffset)
{
	srand(glfwGetTime());
	// add some uniformly spaced point lights
	for (unsigned int lightIndexX = 0; lightIndexX < LIGHT_GRID_WIDTH; lightIndexX++)
	{
		for (unsigned int lightIndexZ = 0; lightIndexZ < LIGHT_GRID_WIDTH; lightIndexZ++)
		{
			for (unsigned int lightIndexY = 0; lightIndexY < LIGHT_GRID_HEIGHT; lightIndexY++)
			{
				float diameter = 2.0f * radius;
				float xPos = (lightIndexX - (LIGHT_GRID_WIDTH - 1.0f) / 2.0f) * (diameter * separation);
				float zPos = (lightIndexZ - (LIGHT_GRID_WIDTH - 1.0f) / 2.0f) * (diameter * separation);
				float yPos = (lightIndexY - (LIGHT_GRID_HEIGHT - 1.0f) / 2.0f) * (diameter * separation) + yOffset;
				double angle = double(rand()) * 2.0 * M_PI / (double(RAND_MAX));
				double length = double(rand()) * 0.5 / (double(RAND_MAX));
				float xOffset = cos(angle) * length;
				float zOffset = sin(angle) * length;
				xPos += xOffset;
				zPos += zOffset;
				// also calculate random color
				float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
				float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
				float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0

				int curLight = lightIndexX * LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT + lightIndexZ * LIGHT_GRID_HEIGHT + lightIndexY;
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(xPos, yPos, zPos));
				// now add to list of matrices
				InstanceData d;
				d.instanceParam = glm::vec4(rColor, gColor, bColor, radius);
				d.instanceMatrix = model;
				modelData.emplace_back(d);
			}
		}
	}
}

void Example00X()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.Set({ 0.0f, 0.3f, -1.0f });

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

	// initialize point lights
	// lighting info
	std::vector<InstanceData> instanceData;
	configurePointLights(instanceData, pointLightRadius, pointLightSeparation, pointLightVerticalOffset);

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
		
	UtilsExample::CoreLightingPassFB lightingPassFB;
	lightingPassFB.Create(Window::GetWidth(), Window::GetHeight());

	UtilsExample::PointsLightingPassFB pointsLightingPassFB;
	pointsLightingPassFB.Create(Window::GetWidth(), Window::GetHeight());


	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };
	ModelRef model2{ new Model("Data/Models/Dragon.obj") };
	ModelRef sphereModel{ new Model("Data/Models/Sphere.obj") };
	auto sphereVao = (*sphereModel)[0]->GetVAO();
	GLBufferRef instanceBuffer{ new GLBuffer(instanceData) };
	// TODO
	{
		// vertex data
		glEnableVertexArrayAttrib(*sphereVao, 0);
		glVertexArrayAttribBinding(*sphereVao, 0, 0);
		glVertexArrayAttribFormat(*sphereVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, position));

		// instance data
		glEnableVertexArrayAttrib(*sphereVao, 2);
		glVertexArrayAttribBinding(*sphereVao, 2, 1);
		glVertexArrayAttribFormat(*sphereVao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, instanceParam));

		glEnableVertexArrayAttrib(*sphereVao, 3);
		glVertexArrayAttribBinding(*sphereVao, 3, 1);
		glVertexArrayAttribFormat(*sphereVao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, instanceMatrix));

		glEnableVertexArrayAttrib(*sphereVao, 4);
		glVertexArrayAttribBinding(*sphereVao, 4, 1);
		glVertexArrayAttribFormat(*sphereVao, 4, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, instanceMatrix) + sizeof(float) * 4);

		glEnableVertexArrayAttrib(*sphereVao, 5);
		glVertexArrayAttribBinding(*sphereVao, 5, 1);
		glVertexArrayAttribFormat(*sphereVao, 5, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, instanceMatrix) + sizeof(float) * 8);

		glEnableVertexArrayAttrib(*sphereVao, 6);
		glVertexArrayAttribBinding(*sphereVao, 6, 1);
		glVertexArrayAttribFormat(*sphereVao, 6, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, instanceMatrix) + sizeof(float) * 12);

		glVertexArrayVertexBuffer(*sphereVao, 1, *instanceBuffer, 0, sizeof(InstanceData));
		glVertexArrayBindingDivisor(*sphereVao, 1, 1);

	}

	QuadShapeRef quad{ new QuadShape{} };
	CubeShapeRef cube{ new CubeShape{} };
	SphereShapeRef sphere{ new SphereShape{} };

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
			pointsLightingPassFB.Resize(Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			glm::ivec2 mousePosition = Mouse::GetPosition();
			auto change = mousePosition - lastMousePosition;
			lastMousePosition = mousePosition;

			if (Keyboard::IsPressed(GLFW_KEY_W)) camera.Move(Camera::Forward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_S)) camera.Move(Camera::Backward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_A)) camera.Move(Camera::Left, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_D)) camera.Move(Camera::Right, deltaTime);

			if (Mouse::IsPressed(Mouse::Button::Right))
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
				if (change.x != 0.0f || change.y != 0.0f) camera.Rotate(change.x, -change.y);
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
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		{
			simpleShadowMapFB.Bind();

			if (enableShadows) 
			{
				lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f);
				lightView = glm::lookAt(globalLight.position, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
				lightSpaceMatrix = lightProjection * lightView;
				// render scene from light's point of view
					
				simpleShadowMapFB.program->SetVertexUniform(0, lightSpaceMatrix);

				// DRAW MODEL
				{
					simpleShadowMapFB.program->SetVertexUniform(1, glm::mat4(1.0f));
					model->Draw(simpleShadowMapFB.program);

					glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
					glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));
					simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
					model2->Draw(simpleShadowMapFB.program);

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.65f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(10.0f));
					gbuffer->GetProgram()->SetVertexUniform(1, modelScale);
					quad->Draw();

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(2.0f));
					gbuffer->GetProgram()->SetVertexUniform(1, modelScale);
					cube->Draw();

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 0.0f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(1.5f));
					gbuffer->GetProgram()->SetVertexUniform(1, modelScale);
					sphere->Draw();
				}
			}
		}

		// 2. geometry pass: render scene's geometry/color data into gbuffer
		{
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			gbuffer->BindForWriting();
			gbuffer->GetProgram()->SetVertexUniform(0, perspective);
			gbuffer->GetProgram()->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer->GetProgram()->SetVertexUniform(2, glm::mat4(1.0f));
			glm::vec4 sponzaSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);
			gbuffer->GetProgram()->SetFragmentUniform(0, sponzaSpecular);
			model->Draw(gbuffer->GetProgram());
				
			glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
			glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			glm::vec4 modelSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
			gbuffer->GetProgram()->SetFragmentUniform(0, modelSpecular);
			model2->Draw(gbuffer->GetProgram());

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.65f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(10.0f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			quad->Draw();

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(2.0f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			cube->Draw();

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 0.0f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(1.5f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			sphere->Draw();
		}

		// 3. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content and shadow map
		//if (gBufferMode == 0) // если цифра, то дебажный режим для вывода выбранной текстуры из gbuffer
		{
			lightingPassFB.Bind();

			lightingPassFB.program->SetFragmentUniform(0, lightSpaceMatrix);
			lightingPassFB.program->SetFragmentUniform(1, glossiness);
			lightingPassFB.program->SetFragmentUniform(2, camera.position);

			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.position"), globalLight.position);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.color"), globalLight.color);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.linear"), gLinearAttenuation);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.quadratic"), gQuadraticAttenuation);

			gbuffer->BindForReading();
			simpleShadowMapFB.depth->Bind(4);
			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		//else
		//{
		//	// отрисовка буферов из gbuffer
		//}

		// 3.5 lighting pass: render point lights on top of main scene with additive blending and utilizing G-Buffer for lighting.
		//if (gBufferMode == 0)
		{
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW); // TODO: чтобы не рисовало сзади?
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			pointsLightingPassFB.Bind();
			Renderer::BlitFrameBuffer(lightingPassFB.fbo, pointsLightingPassFB.fbo,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);

			pointsLightingPassFB.program->SetVertexUniform(0, perspective);
			pointsLightingPassFB.program->SetVertexUniform(1, camera.GetViewMatrix());
			pointsLightingPassFB.program->SetFragmentUniform(0, camera.position);
			pointsLightingPassFB.program->SetFragmentUniform(1, pointLightIntensity);
			pointsLightingPassFB.program->SetFragmentUniform(2, glm::vec2(Window::GetWidth(), Window::GetHeight()));
			pointsLightingPassFB.program->SetFragmentUniform(3, glossiness);

			gbuffer->BindForReading();

			// draw instances
			sphereVao->Bind();
			glDrawElementsInstanced(GL_TRIANGLES, 2280, GL_UNSIGNED_INT, 0, totalLights);

			glDisable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glFrontFace(GL_CCW);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
		}

		// Main frame
		{
			glDisable(GL_DEPTH_TEST);
			Renderer::MainFrameBuffer();
			Renderer::BlitFrameBuffer(pointsLightingPassFB.fbo, nullptr,
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