#pragma once

/*
* Минимальный пример вывода квадрата в мире
* Функции:
* - камера, движение по WASD, вращение мышью
* - шейдерная программа
* - текстура
* - вершинный буфер
* - индексный буфер
* - Vertex Array
*/
void Example001()
{
	Window::Create({});
	Renderer::Init();
	IMGUI::Init();

	struct Vertex final
	{
		glm::vec3 position;
		glm::vec2 texCoords;
	};

	const std::vector<Vertex> verticesQuad =
	{
		{{-0.5f, 0.0f, 0.5f}, {0.0f, 0.0f}},
		{{ 0.5f, 0.0f, 0.5f}, {1.0f, 0.0f}},
		{{ 0.5f, 0.0f,-0.5f}, {1.0f, 1.0f}},
		{{-0.5f, 0.0f,-0.5f}, {0.0f, 1.0f}},
	};
	const std::vector<uint8_t> indicesQuad =
	{
		0, 1, 2, 2, 3, 0,
	};

	const std::vector<AttribFormat> attribs =
	{
		CreateAttribFormat<glm::vec3>(0, offsetof(Vertex, position)),
		CreateAttribFormat<glm::vec2>(1, offsetof(Vertex, texCoords)),
	};
	GLVertexArrayRef quadGeom{ new GLVertexArray(verticesQuad, indicesQuad, attribs) };

#pragma region VertexShader
	const char* mainVertSource = // Vertex Shader:
R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };

out vec2 uvs;

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoords;

layout (location = 0) uniform mat4 uProjectionMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uWorldMatrix;

void main()
{
	uvs = aTexCoords;
	gl_Position = uProjectionMatrix * uViewMatrix * uWorldMatrix * vec4(aPosition, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
	const char* mainFragSource = // Fragment Shader:
R"(
#version 460

in vec2 uvs;
out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuse;

void main()
{
	outColor = texture(diffuse, uvs);
}
)";
#pragma endregion

	GLProgramPipelineRef shaderProgram{ new GLProgramPipeline(mainVertSource, mainFragSource) };

	GLTexture2DRef textureDiffuse{ new GLTexture2D("data/textures/texel_checker.png", STBI_rgb_alpha, true) };

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	Camera camera;
	camera.Set({ 0.0f, 0.3f, 1.0f });

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.2f, 0.4f, 1.0f);

	float lastFrameTime = static_cast<float>(glfwGetTime());

	while (!Window::ShouldClose())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			auto change = Mouse::GetDelta();

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

		// Render
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			textureDiffuse->Bind(0);
			shaderProgram->Bind();
			shaderProgram->SetVertexUniform(0, perspective);
			shaderProgram->SetVertexUniform(1, camera.GetViewMatrix());
			shaderProgram->SetVertexUniform(2, glm::mat4(1.0f));

			quadGeom->DrawTriangles();
		}

		Window::Swap();
	}

	quadGeom.reset();
	textureDiffuse.reset();
	shaderProgram.reset();

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}