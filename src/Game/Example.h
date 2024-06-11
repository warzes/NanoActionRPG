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
* Deferred shading
* - фреймбуферы
*/
void Example003()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	int timeBeg = clock();

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	glm::ivec2 lastMousePosition = Mouse::GetPosition();

	Camera camera;
	camera.SetPosition({ 0.0f, 0.3f, -1.0f });

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
	vec2 TexCoords;
} o;

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoords;

layout (location = 0) uniform mat4 uProjectionMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uWorldMatrix;

void main()
{
	vec4 WorldPos = uWorldMatrix * vec4(aPosition, 1.0);
	mat3 NormalMatrix = transpose(inverse(mat3(uWorldMatrix)));

	o.FragPosInViewSpace = WorldPos.xyz;
	o.Color = aColor;
	o.TexCoords = aTexCoords;
	o.Normal = NormalMatrix * aNormal;

	gl_Position = uProjectionMatrix * uViewMatrix * WorldPos;
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
layout (location = 2) out vec4 outAlbedoSpec;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D SpecularTexture;

void main()
{
	vec4 diffuseTex = texture(DiffuseTexture, i.TexCoord);
	if (diffuseTex.a < 0.2)
		discard;

	outPosition = i.FragPosInViewSpace;
	outNormal = normalize(i.Normal);
	outAlbedoSpec.rgb = diffuseTex.rgb * i.Color;
	outAlbedoSpec.a = texture(SpecularTexture, i.TexCoord).r;
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

		void BindTextures()
		{
			position->Bind(0);
			normal->Bind(1);
			albedo->Bind(2);
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			position.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_NEAREST));
			normal.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_NEAREST));
			albedo.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, width, height, nullptr, GL_NEAREST));
			depth.reset(new GLTexture2D(GL_DEPTH_COMPONENT32, GL_DEPTH, GL_FLOAT, width, height, nullptr, GL_NEAREST));

			fbo.reset( new GLFramebuffer({ position, normal, albedo }, depth) );
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
	
	class LightingPassFB
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };

out vec2 TexCoord;

// full screen quad vertices
const vec2 verts[] = { vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,-1.0f) };
const vec2 uvs[] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };
const uint index[] = { 0, 3, 2, 2, 1, 0 };

void main()
{
	TexCoord = uvs[index[gl_VertexID]];
	gl_Position = vec4(verts[index[gl_VertexID]], 0.0, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

out vec4 outFragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D positionTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D albedoTexture;

struct Light {
	vec3 position;
	vec3 color;

	float linear;
	float quadratic;
	float radius;
};
const int NR_LIGHTS = 32;

layout (location = 0) uniform vec3 uCameraPos;
layout (location = 1) uniform Light uLights[NR_LIGHTS];

void main()
{
	// retrieve data form gbuffer
	const vec3 fragPos = texture(positionTexture, TexCoords).rgb;
	const vec3 normalTex = texture(normalTexture, TexCoords).rgb;
	const vec4 diffuseTex = texture(albedoTexture, TexCoords);
	const vec3 Diffuse = diffuseTex.rgb;
	const float Specular = diffuseTex.a;

	vec3 lighting = Diffuse * 0.1; // hard-coded ambient component
	vec3 viewDir = normalize(uCameraPos - fragPos);

	// point lights
	for(int i = 0; i < NR_LIGHTS; ++i)
	{
		// calculate distance between light source and current fragment
		float distance = length(uLights[i].position - fragPos);
		if (distance < uLights[i].radius)
		{
			// diffuse
			vec3 lightDir = normalize(uLights[i].position - fragPos);
			vec3 diffuse = max(dot(normalTex, lightDir), 0.0) * Diffuse * uLights[i].color;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(normalTex, halfwayDir), 0.0), 16.0);
			vec3 specular = uLights[i].color * spec * Specular;
			// attenuation
			float attenuation = 1.0 / (1.0 + uLights[i].linear * distance + uLights[i].quadratic * distance * distance);
			diffuse *= attenuation;
			specular *= attenuation;
			lighting += diffuse + specular;
		}
	}
	
	outFragColor = vec4(lighting, 1.0);
	//outFragColor = pow(outFragColor, vec3(1.0f/2.2f));
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

			color.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, width, height, nullptr, GL_NEAREST));

			fbo.reset(new GLFramebuffer({ color }));
		}

		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef color = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};

	LightingPassFB lightingPassFB;
	lightingPassFB.Create(Window::GetWidth(), Window::GetHeight());

	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };

#pragma region lighting info
	const unsigned int NR_LIGHTS = 32;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(123);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
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
			gbuffer.Resize(Window::GetWidth(), Window::GetHeight());
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
		// GBuffer
		{
			glEnable(GL_DEPTH_TEST);
			gbuffer.Bind();
			gbuffer.program->SetVertexUniform(0, perspective);
			gbuffer.program->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer.program->SetVertexUniform(2, glm::mat4(1.0f));
			model->Draw(gbuffer.program);
		}

		// Lighting pass framebuffer
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

			gbuffer.BindTextures();
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

	gbuffer.Destroy();
	lightingPassFB.Destroy();

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}