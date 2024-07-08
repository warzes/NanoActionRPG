#pragma once

#include <iostream>

namespace raycast
{
	struct Sprite
	{
		inline operator glm::vec2()
		{
			return glm::vec2(x, y);
		}

		float x;
		float y;
		uint32_t texture;
		int32_t uDiv;
		int32_t vDiv;
		float vMove;
	};
	class Map
	{
	public:
		inline uint8_t& At(size_t x, size_t y)
		{
			return data[x * size.x + y];
		}

		void Destroy()
		{
			delete[] data;
		}

		static std::optional<Map> Load()
		{
			uint32_t mapWidth = 24;
			uint32_t mapHeight = 24;
			uint8_t* map = new uint8_t[mapWidth * mapHeight];

			std::vector<std::vector<uint8_t>> tempMap = {
				{8,8,8,8,8,8,8,8,8,8,8,4,4,6,4,4,6,4,6,4,4,4,6,4},
				{8,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,0,0,0,0,0,0,0,4},
				{8,0,3,3,0,0,0,0,0,8,8,4,0,0,0,0,0,0,0,0,0,0,0,6},
				{8,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6},
				{8,0,3,3,0,0,0,0,0,8,8,4,0,0,0,0,0,0,0,0,0,0,0,4},
				{8,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,0,6,6,6,0,6,4,6},
				{8,8,8,8,0,8,8,8,8,8,8,4,4,4,4,4,4,6,0,0,0,0,0,6},
				{7,7,7,7,0,7,7,7,7,0,8,0,8,0,8,0,8,4,0,4,0,6,0,6},
				{7,7,0,0,0,0,0,0,7,8,0,8,0,8,0,8,8,6,0,0,0,0,0,6},
				{7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,6,0,0,0,0,0,4},
				{7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,6,0,6,0,6,0,6},
				{7,7,0,0,0,0,0,0,7,8,0,8,0,8,0,8,8,6,4,6,0,6,6,6},
				{7,7,7,7,0,7,7,7,7,8,8,4,0,6,8,4,8,3,3,3,0,3,3,3},
				{2,2,2,2,0,2,2,2,2,4,6,4,0,0,6,0,6,3,0,0,0,0,0,3},
				{2,2,0,0,0,0,0,2,2,4,0,0,0,0,0,0,4,3,0,0,0,0,0,3},
				{2,0,0,0,0,0,0,0,2,4,0,0,0,0,0,0,4,3,0,0,0,0,0,3},
				{1,0,0,0,0,0,0,0,1,4,4,4,4,4,6,0,6,3,3,0,0,0,3,3},
				{2,0,0,0,0,0,0,0,2,2,2,1,2,2,2,6,6,0,0,5,0,5,0,5},
				{2,2,0,0,0,0,0,2,2,2,0,0,0,2,2,0,5,0,5,0,0,0,5,5},
				{2,0,0,0,0,0,0,0,2,0,0,0,0,0,2,5,0,5,0,5,0,5,0,5},
				{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5},
				{2,0,0,0,0,0,0,0,2,0,0,0,0,0,2,5,0,5,0,5,0,5,0,5},
				{2,2,0,0,0,0,0,2,2,2,0,0,0,2,2,0,5,0,5,0,0,0,5,5},
				{2,2,2,2,1,2,2,2,2,2,2,1,2,2,2,5,5,5,5,5,5,5,5,5},
			};

			for (uint32_t x = 0; x < mapWidth; x++)
			{
				for (uint32_t y = 0; y < mapHeight; y++)
				{
					map[x * mapWidth + y] = tempMap[x][y];
				}
			}

			glm::vec2 initialPos(22.0f, 11.5f);
			glm::vec2 initialDir(-1.f, 0.f);
			glm::vec2 initialPlane(0.0f, 0.666666666666f);

			std::cout << "  > Loading sprites data" << std::endl;
			std::vector<Sprite> sprites;

			// green light in front of playerstart
			{
				Sprite sprite =
				{
					20.5,
					11.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			// green lights in every room
			{
				Sprite sprite =
				{
					18.5,
					4.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					10.0,
					4.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					10.0,
					12.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					3.5,
					6.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					3.5,
					20.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					3.5,
					14.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					14.5,
					20.5,
					10,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			// row of pillars in front of wall: fisheye test
			{
				Sprite sprite =
				{
					18.5,
					10.5,
					9,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					18.5,
					11.5,
					9,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					18.5,
					12.5,
					9,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			// some barrels around the map
			{
				Sprite sprite =
				{
					21.5,
					1.5,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					15.5,
					1.5,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					16.0,
					1.8,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					16.2,
					1.2,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					3.5,
					2.5,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					9.5,
					15.5,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					10.0,
					15.1,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}
			{
				Sprite sprite =
				{
					10.5,
					15.8,
					8,
					1,
					1,
					0.0f,
				};
				sprites.push_back(sprite);
			}

			std::variant<uint32_t, glm::vec3> floor, ceil;
			floor = 3u;
			ceil = 3u;

			std::cout << "  > Loading map texture" << std::endl;
			Map mapData{
				map,
				glm::uvec2(mapWidth, mapHeight),
				floor,
				ceil,
				initialPos,
				initialDir,
				initialPlane,
				sprites,
				std::make_shared<GLTexture2D>(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, mapWidth, mapHeight, map, GL_NEAREST, GL_REPEAT),
			};

			return mapData;
		}

		uint8_t* data;
		glm::uvec2 size;
		std::variant<uint32_t, glm::vec3> floor;
		std::variant<uint32_t, glm::vec3> ceil;
		glm::vec2 initialPos;
		glm::vec2 initialDir;
		glm::vec2 initialPlane;
		std::vector<Sprite> sprites;
		GLTexture2DRef texture;
	};
}

void RaycastGame()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	glDisable(GL_DEPTH_TEST);
	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	auto raycasterComputeProgram = std::make_shared<GLProgramPipeline>(LoadShaderTextFile("RaycastData/Shader/RaycasterShader.comp"));
	auto spritecasterComputeProgram = std::make_shared<GLProgramPipeline>(LoadShaderTextFile("RaycastData/Shader/SpritecasterShader.comp"));
	auto raycasterDrawProgram = std::make_shared<GLProgramPipeline>(LoadShaderTextFile("RaycastData/Shader/MainVertexShader.vert"), LoadShaderTextFile("RaycastData/Shader/DrawerShader.frag"));

	auto currentMap = raycast::Map::Load();

	glm::vec2 pos = currentMap->initialPos;
	glm::vec2 dir = currentMap->initialDir;
	glm::vec2 plane = currentMap->initialPlane;

	auto raycastResultBuffer = std::make_shared<GLShaderStorageBuffer>(10000 * (5 * sizeof(int32_t) + 5 * sizeof(float))); // sizeof(xdata) * 10000
	auto spritecastResultBuffer = std::make_shared<GLShaderStorageBuffer>(currentMap->sprites.size() * (8 * sizeof(int32_t) + 1 * sizeof(float) + 1 * sizeof(uint32_t)));
	auto spritecastInputBuffer = std::make_shared<GLShaderStorageBuffer>(currentMap->sprites.size() * sizeof(raycast::Sprite), GL_DYNAMIC_COPY);

	const std::vector<std::string_view> filepath{
		"rc/textures/eagle.png",
		"rc/textures/redbrick.png",
		"rc/textures/purplestone.png",
		"rc/textures/greystone.png",
		"rc/textures/bluestone.png",
		"rc/textures/mossy.png",
		"rc/textures/wood.png",
		"rc/textures/colorstone.png",

		"rc/textures/barrel.png",
		"rc/textures/pillar.png",
		"rc/textures/greenlight.png"
	};
	auto textures = std::make_shared<GLTexture2DArray>(filepath, GL_RGBA32F, glm::ivec3{ 64, 64, 11 }, STBI_rgb, 1, GL_NEAREST, GL_REPEAT);

	std::vector<raycast::Sprite> sortedSprites(currentMap->sprites);

	double previousTime = glfwGetTime() - 1.0 / 60.0;
	double lastFpsTick = glfwGetTime();
	bool initialSpriteFill = false;

	glm::vec2 mouseDirection(0, 0);

	Mouse::SetCursorMode(Mouse::CursorMode::Disabled);

	
	glm::ivec2 frameSize;
	const float framebufferSize = 600.0f;
	float aspectRatioScreen = (float)Window::GetWidth() / (float)Window::GetHeight();
	if (aspectRatioScreen > 1.333)
		frameSize = { framebufferSize * aspectRatioScreen, framebufferSize };
	else
		frameSize = { framebufferSize , framebufferSize * aspectRatioScreen };


	while (!Window::ShouldClose())
	{
		Window::Update();

		if (Window::IsResize())
		{
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
			aspectRatioScreen = (float)Window::GetWidth() / (float)Window::GetHeight();
			if (aspectRatioScreen > 1.333)
				frameSize = { framebufferSize * aspectRatioScreen, framebufferSize };
			else
				frameSize = { framebufferSize , framebufferSize * aspectRatioScreen };
		}

#pragma region render
		// старт рейкастинга на вычислительном шейдере
		{
			/*
			* вычислительный шейдер работает по столбцам (ширина игрового экрана), каждый экземпляр вычисляет значения одного столбца
			* на входе он получает текстуру карты в виде uimage2D (для того чтобы можно было обращаться к xy а не uv)
			* на выходе выдает данные в SSBO raycastResultBuffer
			* также принимает юниформы - позицию игрока, направление его взгляда и плоскость направления, а также размер игрового экрана
			* TODO: ssbo буфер содержит 10000 элементов, где каждый элемент - это столбец. то есть максимальная ширина экрана - 10000 пикселей. но мне не нужна такая высота, возможно сократить до 4000.
			*/

			currentMap->texture->BindImage(1);
			raycastResultBuffer->BindBase(2);

			raycasterComputeProgram->Bind();
			raycasterComputeProgram->SetComputeUniform(1, pos);
			raycasterComputeProgram->SetComputeUniform(2, dir);
			raycasterComputeProgram->SetComputeUniform(3, plane);
			raycasterComputeProgram->SetComputeUniform(4, frameSize); // TODO: only resize window events
			glDispatchCompute(frameSize.x, 1, 1);
		}

		// старт рендера спрайтов на вычислительном шейдере
		{
			spritecastInputBuffer->BindBase(1);
			spritecastResultBuffer->BindBase(2);

			spritecasterComputeProgram->Bind();
			spritecasterComputeProgram->SetComputeUniform(1, pos);
			spritecasterComputeProgram->SetComputeUniform(2, dir);
			spritecasterComputeProgram->SetComputeUniform(3, plane);
			spritecasterComputeProgram->SetComputeUniform(4, frameSize); // TODO: only resize window events
			glDispatchCompute(currentMap->sprites.size(), 1, 1);
		}

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//ожидание завершения работы вычислительных шейдеров
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// отрисовка результата на экран через пиксельный шейдер
		{
			raycasterDrawProgram->Bind();
			textures->BindImage(1, 0, false, 0);
			raycastResultBuffer->BindBase(2);
			spritecastResultBuffer->BindBase(3);

			raycasterDrawProgram->SetFragmentUniform(1, frameSize); // TODO: only resize window events
			raycasterDrawProgram->SetFragmentUniform(2, pos);
			raycasterDrawProgram->SetFragmentUniform(3, sortedSprites.size());
			raycasterDrawProgram->SetFragmentUniform(4, glm::vec4(0.f, 0.f, 0.f, 3.0f));
			raycasterDrawProgram->SetFragmentUniform(5, glm::vec4(0.f, 0.f, 0.f, 3.0f));

			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

#pragma region imgui
		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0 / double(ImGui::GetIO().Framerate), double(ImGui::GetIO().Framerate));
			ImGui::End();
			IMGUI::Draw();
		}
#pragma endregion

		Window::Swap();
#pragma endregion

#pragma region other

		{
			auto pos = Mouse::GetPosition();

			static glm::ivec2 oldPos(pos);

			glm::ivec2 diff = pos - oldPos;
			mouseDirection = diff;

			oldPos = pos;
		}

		const float currentTime = glfwGetTime();
		const float delta = currentTime - previousTime;
		const float moveSpeed = delta * 3.5f;
		const float rotationSpeed = delta * 2.5f;
		float movement = 0.0f;
		float rotation = 0.0f;

		// move forward
		const bool forwardPressed = Keyboard::IsPressed(GLFW_KEY_W);
		if (forwardPressed) {
			movement += moveSpeed;
		}
		// move backward
		const bool backwardPressed = Keyboard::IsPressed(GLFW_KEY_S);
		if (backwardPressed) {
			movement -= moveSpeed;
		}
		// move forward or backward (with the mouse)
		if (!(forwardPressed || backwardPressed) && abs(mouseDirection.y) > 0.001f) {
			movement = -delta * mouseDirection.y * 1.75f;
		}
		if (abs(movement) > 0.00000001f) {
			// move only if the player does not collide with some wall
			if (currentMap->At(int(pos.x + dir.x * movement), int(pos.y)) == 0)
				pos.x += dir.x * movement;
			if (currentMap->At(int(pos.x), int(pos.y + dir.y * movement)) == 0)
				pos.y += dir.y * movement;
		}
		// rotate camera to the right
		const bool rotateRightPressed = Keyboard::IsPressed(GLFW_KEY_D);
		if (rotateRightPressed) {
			rotation -= rotationSpeed;
		}
		// rotate camera to the left
		const bool rotateLeftPressed = Keyboard::IsPressed(GLFW_KEY_A);
		if (rotateLeftPressed) {
			rotation += rotationSpeed;
		}
		// rotate camera to the right or left
		if (!(rotateRightPressed || rotateLeftPressed) && abs(mouseDirection.x) > 0.001f) {
			rotation = -mouseDirection.x * delta;
		}
		if (abs(rotation) > 0.00000001f) {
			double oldDirX = dir.x;
			dir.x = dir.x * cos(rotation) - dir.y * sin(rotation);
			dir.y = oldDirX * sin(rotation) + dir.y * cos(rotation);
			double oldPlaneX = plane.x;
			plane.x = plane.x * cos(rotation) - plane.y * sin(rotation);
			plane.y = oldPlaneX * sin(rotation) + plane.y * cos(rotation);
		}

		// update sprites order depending on player's position
		if (currentTime - lastFpsTick >= 1 || !initialSpriteFill)
		{
			initialSpriteFill = true;
			// sort sprites
			std::sort(sortedSprites.begin(), sortedSprites.end(), [pos](auto& a, auto& b) {
				const float distA = glm::distance(pos, (glm::vec2)a);
				const float distB = glm::distance(pos, (glm::vec2)b);
				return distA > distB;
				});

			spritecastInputBuffer->SetData(sortedSprites);
		}

		previousTime = currentTime;
		mouseDirection = { 0, 0 };
#pragma endregion

	}

	currentMap->Destroy();
	VAOEmpty.reset();
	Renderer::Close();
	Window::Destroy();
}