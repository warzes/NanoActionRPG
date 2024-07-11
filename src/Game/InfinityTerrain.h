#pragma once

namespace terrain
{
	namespace sky
	{
		struct SkyVertex
		{
			glm::vec3 pos;
			glm::vec2 texCoord;
		};

		const SkyVertex skyVertices[] = // ЕЩВЩ
		{
			{{-0.5, -0.5, -0.5},{0.333, 0.75}},
			{{-0.5, 0.5, -0.5},{0.666, 0.75}},
			{{0.5, -0.5, -0.5},{0.333, 0.5}},
			{{-0.5, 0.5, -0.5},{0.666, 0.75}},
			{{0.5, -0.5, -0.5},{0.666, 0.5}},
			{{0.5, 0.5, -0.5},{0.666, 0.25}},
			{{0.5, 0.5, 0.5},{0.333, 0.25}},
			{{0.5, -0.5, 0.5},{0.666, 0.5}},
			{{0.5, 0.5, -0.5},{0.333, 0.25}},
			{{0.5, -0.5, 0.5},{0.666, 0.5}},
			{{0.5, 0.5, -0.5},{0.333, 0.5}},
			{{0.5, -0.5, -0.5},{0.666, 0.25}},
			{{0.5, 0.5, 0.5},{0.666, 0.0}},
			{{-0.5, 0.5, 0.5},{0.333, 0.25}},
			{{0.5, -0.5, 0.5},{0.666, 0.0}},
			{{-0.5, 0.5, 0.5},{0.333, 0.25}},
			{{0.5, -0.5, 0.5},{0.333, 0.0}},
			{{-0.5, -0.5, 0.5},{0.0, 0.75}},
			{{-0.5, -0.5, 0.5},{0.333, 0.75}},
			{{-0.5, -0.5, -0.5},{0.0, 0.5}},
			{{0.5, -0.5, 0.5},{0.333, 0.75}},
			{{-0.5, -0.5, -0.5},{0.0, 0.5}},
			{{0.5, -0.5, 0.5},{0.333, 0.5}},
			{{0.5, -0.5, -0.5},{0.666, 0.75}},
			{{-0.5, 0.5, -0.5},{0.333, 0.75}},
			{{-0.5, -0.5, -0.5},{0.666, 1.0}},
			{{-0.5, 0.5, 0.5},{0.333, 0.75}},
			{{-0.5, -0.5, -0.5},{0.666, 1.0}},
			{{-0.5, 0.5, 0.5},{0.333, 1.0}},
			{{-0.5, -0.5, 0.5},{0.666, 0.5}},
			{{0.5, 0.5, -0.5},{0.666, 0.75}},
			{{-0.5, 0.5, -0.5},{1.0, 0.5}},
			{{0.5, 0.5, 0.5},{0.666, 0.75}},
			{{-0.5, 0.5, -0.5},{1.0, 0.5}},
			{{0.5, 0.5, 0.5},{1.0, 0.75}},
			{{-0.5, 0.5, 0.5},{}},
		};

		class Sky
		{
		public:
			void Create()
			{
#pragma region VertexShader
				const char* mainVertSource = // Vertex shader:
R"(
#version 460 core

out gl_PerVertex { vec4 gl_Position; };

out vec2 uv;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvs;

layout (location = 0) uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(pos, 1);
    uv = uvs;
}
)";
#pragma endregion

#pragma region FragmentShader
				const char* mainFragSource = // Fragment shader:
R"(
#version 460

in vec2 uv;
out vec4 outColor;

layout(binding = 0) uniform sampler2D tex;

void main()
{
	outColor = texture(tex, uv);
}
)";
#pragma endregion
				program = std::make_shared<GLProgramPipeline>(mainVertSource, mainFragSource);

				texture = std::make_shared<GLTexture2D>("data/textures/sky_texture.tga", STBI_rgb, true);
			}
			void Destroy()
			{

			}

			void Draw(const glm::mat4& viewProj)
			{

			}

			GLProgramPipelineRef program;
			GLVertexArrayRef quadGeom;
			GLTexture2DRef texture;
		};
	}


}

void InfinityTerrain()
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

	float lastFrameTime = static_cast<float>(glfwGetTime());

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

#pragma region VertexShader
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
#pragma endregion

#pragma region FragmentShader
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
#pragma endregion	

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	Camera camera;
	camera.Set({ 0.0f, 0.3f, 1.0f });

	glEnable(GL_DEPTH_TEST);

	GLTexture2DRef textureCubeDiffuse{ new GLTexture2D("data/textures/T_Default_D.png", STBI_rgb, true) };

	GLVertexArrayRef quadGeom{ new GLVertexArray(verticesQuad, indicesQuad, GetMeshVertexFormat()) };

	GLProgramPipelineRef ppMain{ new GLProgramPipeline(mainVertSource, mainFragSource) };

	while (!Window::ShouldClose())
	{
#pragma region deltatime
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;
#pragma endregion

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

#pragma region imgui

		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::End();
		}
#pragma endregion

#pragma region render
		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		textureCubeDiffuse->Bind(0);
		ppMain->Bind();
		ppMain->SetVertexUniform(0, perspective);
		ppMain->SetVertexUniform(1, camera.GetViewMatrix());
		ppMain->SetVertexUniform(2, glm::mat4(1.0f));

		quadGeom->DrawTriangles();

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}