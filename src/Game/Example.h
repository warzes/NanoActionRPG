#pragma once

/*
* Минимальный пример вывода квада на экран
* Функции:
* - камера, движение по WASD, вращение мышью
* - шейдеры
* - текстура
* - массив вершин VertexArray
*/
void Example001()
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

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

/*
* Загрузка и вывод 3д модели из файла
* - mesh, model, material
*/
void Example002()
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

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

layout(binding = 0) uniform sampler2D sDiffuseTexture;

void main()
{
	outColor = texture(sDiffuseTexture, i.uvs);
	outColor.rgb = outColor.rgb * i.col;
}
)";
#pragma endregion
	
	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.SetPosition({ 0.0f, 0.3f, -1.0f });

	glEnable(GL_DEPTH_TEST);

	GLProgramPipelineRef ppMain{ new GLProgramPipeline(mainVertSource, mainFragSource) };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };

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
		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ppMain->Bind();
		ppMain->SetVertexUniform(0, perspective);
		ppMain->SetVertexUniform(1, camera.GetViewMatrix());
		ppMain->SetVertexUniform(2, glm::mat4(1.0f));

		model->Draw(ppMain);

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}

/*
* Deffered render shading
* - фреймбуферы
*/
void Example003()
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.SetPosition({ 0.0f, 0.3f, -1.0f });

	glEnable(GL_DEPTH_TEST);

	class GBuffer
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };

out outBlock
{
	vec3 FragPosInViewSpace; // POSITION
	vec3 Color;
	vec3 Normal;
	vec2 TexCoord;
} o;

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Color;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec2 TexCoord;

layout (location = 0) uniform mat4 ProjectionMatrix;
layout (location = 1) uniform mat4 ViewMatrix;
layout (location = 2) uniform mat4 WorldMatrix;

void main()
{
	const mat4 VWMatrix = ViewMatrix * WorldMatrix;
	const vec4 FragPosInViewSpace = VWMatrix * vec4(Position, 1.0);
	gl_Position = ProjectionMatrix * FragPosInViewSpace;
	o.Color = Color;
	o.TexCoord = TexCoord;
	o.Normal = normalize(mat3(transpose(inverse(VWMatrix))) * Normal);
	o.FragPosInViewSpace = FragPosInViewSpace.xyz;
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

in inBlock
{
	vec3 FragPosInViewSpace;
	vec3 Color;
	vec3 Normal;
	vec2 TexCoord;
} i;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAlbedo;

layout(binding = 0) uniform sampler2D DiffuseTexture;

void main()
{
	vec4 diffuseTex = texture(DiffuseTexture, i.TexCoord);

	outPosition = i.FragPosInViewSpace;
	outNormal = i.Normal;
	outAlbedo.rgb = diffuseTex.rgb * i.Color;
	outAlbedo.a = diffuseTex.a;
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}
		void Destroy()
		{
			program.reset();
			fbo.reset();
			position.reset();
			normal.reset();
			albedo.reset();
			depth.reset();
			width = height = 0;
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;

			fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0f)));
			fbo->ClearFramebuffer(GL_COLOR, 1, glm::value_ptr(glm::vec4(0.0f)));
			fbo->ClearFramebuffer(GL_COLOR, 2, glm::value_ptr(glm::vec4(0.0f)));

			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

			fbo->Bind();
			Renderer::SetViewport(0, 0, width, height);

			program->Bind();
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			position.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_LINEAR));
			normal.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_LINEAR));
			albedo.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_LINEAR));
			depth.reset(new GLTexture2D(GL_DEPTH_COMPONENT32, GL_DEPTH, GL_FLOAT, width, height, nullptr, GL_NEAREST));

			fbo = GLFramebufferRef{ new GLFramebuffer({ position, normal, albedo }, depth) };
		}

		GLFramebufferRef fbo = nullptr;

		GLTexture2DRef position = nullptr;
		GLTexture2DRef normal = nullptr;
		GLTexture2DRef albedo = nullptr;
		GLTexture2DRef depth = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};

	GBuffer gbuffer;
	gbuffer.Create(Window::GetWidth(), Window::GetHeight());
	
	class FinalFB
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };

out outBlock
{
	vec2 texcoord;
} o;

void main()
{
	vec2 v[] = {
		vec2(-1.0f, 1.0f),
		vec2(1.0f, 1.0f),
		vec2(1.0f,-1.0f),
		vec2(-1.0f,-1.0f)
	};
	vec2 t[] = {
		vec2(0.0f, 1.0f),
		vec2(1.0f, 1.0f),
		vec2(1.0f, 0.0f),
		vec2(0.0f, 0.0f)

	};
	uint i[] = { 0, 3, 2, 2, 1, 0 };

	const vec2 position = v[i[gl_VertexID]];
	const vec2 texcoord = t[i[gl_VertexID]];

	o.texcoord = texcoord;
	gl_Position = vec4(position, 0.0, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D computeTexture;

in in_block
{
	vec2 texcoord;
} i;

void main()
{
	vec3 TexelColor = texture(computeTexture, i.texcoord).rgb;
	TexelColor = pow(TexelColor, vec3(1.0f/2.2f));
	outColor = vec4(TexelColor, 1.0f);
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}
		void Destroy()
		{
			program.reset();
			fbo.reset();
			color.reset();
			width = height = 0;
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;
			fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.2, 0.6f, 1.0f)));
			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

			fbo->Bind();
			Renderer::SetViewport(0, 0, width, height);

			program->Bind();
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			color.reset(new GLTexture2D(GL_RGB8, GL_RGB, width, height, nullptr, GL_NEAREST));

			fbo = GLFramebufferRef{ new GLFramebuffer({ color }) };
		}

		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef color = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};

	FinalFB finalFB;
	finalFB.Create(Window::GetWidth(), Window::GetHeight());
	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };

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
			gbuffer.Resize(Window::GetWidth(), Window::GetHeight());
			finalFB.Resize(Window::GetWidth(), Window::GetHeight());
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
		// GBuffer
		{
			gbuffer.Bind();
			gbuffer.program->SetVertexUniform(0, perspective);
			gbuffer.program->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer.program->SetVertexUniform(2, glm::mat4(1.0f));
			model->Draw(gbuffer.program);
		}

		// Final framebuffer
		{
			finalFB.Bind();
			gbuffer.albedo->Bind(0);
			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Main frame
		{
			Renderer::MainFrameBuffer();
			Renderer::BlitFrameBuffer(finalFB.fbo, nullptr,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}