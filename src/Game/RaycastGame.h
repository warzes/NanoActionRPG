#pragma once

#include <iostream>

namespace raycast
{
#pragma region VertexShader
	const char* vertexShader = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };
out vec2 TexCoords;

// full screen quad vertices
const vec2 verts[] = { vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,-1.0f) };
const vec2 uvs[] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };
const uint index[] = { 0, 3, 2, 2, 1, 0 };

void main()
{
	TexCoords = uvs[index[gl_VertexID]];
	gl_Position = vec4(verts[index[gl_VertexID]], 0.0, 1.0);
}
)";
#pragma endregion

#pragma region raycasterShader
	const char* raycasterShader = R"(
#version 460

// Raycaster based on https://lodev.org/cgtutor/raycasting.html
// this file only calculates the line heights, which are stored into a shared buffer xD

struct xdata {
	ivec2 draw;
	int side;
	uint textureNum;
	int texX;
	float step;
	float texPos;
	float distWall;
	vec2 floorWall;
};

layout(local_size_x=1, local_size_y=1) in;
layout(r8ui, binding=1) uniform uimage2D map;
layout(std430, binding=2) buffer dataOutput {
	restrict xdata res[10000];
};
layout(location=1) uniform vec2 position;
layout(location=2) uniform vec2 direction;
layout(location=3) uniform vec2 plane;
layout(location=4) uniform ivec2 screenSize;

void main()
{
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
	uint x = pixelCoords.x;
	uint w = screenSize.x;
	int height = screenSize.y;
	ivec3 pixel = ivec3(0, 0, 0);
	ivec2 mapSize = imageSize(map);

	// x-coord in camera space
	float cameraX = 2 * float(x) / float(w) - 1;
	vec2 rayDir = direction + plane * cameraX;

	// where we are now (the box from the map)
	ivec2 mapPos = ivec2(position);
	// length of the ray from one x/y-side to the next x/y-side (simplified formula)
	vec2 deltaDist = vec2(abs(1 / rayDir.x), abs(1 / rayDir.y));
	// direction of the ray
	ivec2 step;
	// length of the ray from current position to the next x/y-side
	vec2 sideDist;

	// calculate initial step and sideDist values
	if(rayDir.x < 0) {
		step.x = -1;
		sideDist.x = (position.x - mapPos.x) * deltaDist.x;
	} else {
		step.x = 1;
		sideDist.x = (mapPos.x + 1.0f - position.x) * deltaDist.x;
	}

	if(rayDir.y < 0) {
		step.y = -1;
		sideDist.y = (position.y - mapPos.y) * deltaDist.y;
	} else {
		step.y = 1;
		sideDist.y = (mapPos.y + 1.0f - position.y) * deltaDist.y;
	}

	// perform DDA
	int side = 0;
	bool hit = false;
	while(!hit) {
		if(sideDist.x < sideDist.y) {
			sideDist.x += deltaDist.x;
			mapPos.x += step.x;
			side = 0;
		} else {
			sideDist.y += deltaDist.y;
			mapPos.y += step.y;
			side = 1;
		}

		// map coords are reversed!
		uint mapValue = imageLoad(map, mapPos.yx).r;
		hit = mapValue > 0;
	}

	// distance between the camera and the wall (perpendicullar not euclidean)
	float perpWallDist;
	if(side == 0) {
		perpWallDist = (mapPos.x - position.x + (1.0f - step.x) / 2.0f) / rayDir.x;
	} else {
		perpWallDist = (mapPos.y - position.y + (1.0f - step.y) / 2.0f) / rayDir.y;
	}

	// calculate the height of the line to draw
	int lineHeight = int(height / perpWallDist);

	// calculate lowest and highest pixel to fill in current stripe
	int drawStart = -lineHeight / 2 + height / 2;
	if(drawStart < 0) {
		drawStart = 0;
	}
	int drawEnd = lineHeight / 2 + height / 2;
	if(drawEnd >= height) {
		drawEnd = height - 1;
	}

	// texturing calculations (map coords are reversed)
	uint texNum = imageLoad(map, mapPos.yx).r - 1;

	// calculate value of wallX - where exactly the wall was hit
	float wallX;
	if(side == 0) {
		wallX = position.y + perpWallDist * rayDir.y;
	} else {
		wallX = position.x + perpWallDist * rayDir.x;
	}
	wallX -= floor(wallX);

	// x coordinate on the texture
	int texWidth = 64; // TODO not hardcode texWidth and texHeight
	int texHeight = 64;
	int texX = int(wallX * float(texWidth));
	if(side == 0 && rayDir.x > 0) texX = texWidth - texX - 1;
	if(side == 1 && rayDir.y < 0) texX = texWidth - texX - 1;

	// floow/ceil casting (vertical version to take advantage of this loop)
	vec2 floorWall;

	// 4 different wall directions possible
	if(side == 0 && rayDir.x > 0) {
		floorWall.x = mapPos.x;
		floorWall.y = mapPos.y + wallX;
	} else if(side == 0 && rayDir.x < 0) {
		floorWall.x = mapPos.x + 1.0f;
		floorWall.y = mapPos.y + wallX;
	} else if(side == 1 && rayDir.y > 0) {
		floorWall.x = mapPos.x + wallX;
		floorWall.y = mapPos.y;
	} else {
		floorWall.x = mapPos.x + wallX;
		floorWall.y = mapPos.y + 1.0f;
	}

	float distWall = perpWallDist;

	// store information for the fragment shader
	res[x].draw = ivec2(drawStart, drawEnd);
	res[x].side = side;
	res[x].textureNum = texNum;
	res[x].texX = texX;
	res[x].step = float(texHeight) / float(lineHeight);
	res[x].texPos = float(drawStart - height / 2 + lineHeight / 2) * res[x].step;
	res[x].distWall = distWall; // <- zbuffer
	res[x].floorWall = floorWall;
}
)";
#pragma endregion

#pragma region spritecasterShader
	const char* spritecasterShader = R"(
#version 460

// Raycaster for sprites based on https://lodev.org/cgtutor/raycasting3.html
// this file calculates some values for drawing sprites in the next step

struct spritedata {
	int spriteWidth;
	int spriteHeight;
	float transformY;
	int spriteScreenX;
	ivec2 drawX;
	ivec2 drawY;
	int vMoveScreen;
	uint texture;
};

struct sprite {
	float x;
	float y;
	uint texture;
	int uDiv;
	int vDiv;
	float vMove;
};

layout(local_size_x=1, local_size_y=1) in;
layout(std430, binding=1) buffer dataInput {
	restrict sprite sprites[100];
};
layout(std430, binding=2) buffer dataOutput {
	restrict spritedata spriteResults[100];
};
layout(location=1) uniform vec2 position;
layout(location=2) uniform vec2 direction;
layout(location=3) uniform vec2 plane;
layout(location=4) uniform ivec2 screenSize;

void main()
{
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
	uint spriteNum = pixelCoords.x;
	sprite sprite = sprites[spriteNum];

	// translate sprite position to relative to camera
	vec2 spritePos = vec2(sprite.x, sprite.y) - position;

	// transform sprite with the inverse camera matrix
	float invDet = 1.0f / (plane.x * direction.y - direction.x * plane.y);
	vec2 transform = vec2(
		invDet * (direction.y * spritePos.x - direction.x * spritePos.y),
		// this is actually the depth inside the screen, that what Z is in 3D
		invDet * (-plane.y * spritePos.x + plane.x * spritePos.y)
	);

	int spriteScreenX = int((screenSize.x * 0.5f) * (1.f + transform.x / transform.y));
	int vMoveScreen = int(sprite.vMove / transform.y);

	// calculate height of the sprite on screen
	//  using 'transformY' instead of the real distance prevents fisheye
	int spriteHeight = abs(int(screenSize.y / transform.y)) / sprite.vDiv;
	// calculate lowest and highest pixel to fill in current stripe
	ivec2 drawY = ivec2(
		-spriteHeight * 0.5 + screenSize.y * 0.5 + vMoveScreen,
		spriteHeight * 0.5 + screenSize.y * 0.5 + vMoveScreen
	);
	if(drawY.x < 0) drawY.x = 0;
	if(drawY.y >= screenSize.y) drawY.y = screenSize.y - 1;

	// calculate width of the sprite on screen
	int spriteWidth = abs(int(screenSize.y / transform.y)) / sprite.uDiv;
	ivec2 drawX = ivec2(
		-spriteWidth * 0.5 + spriteScreenX,
		spriteWidth * 0.5 + spriteScreenX
	);
	if(drawX.x < 0) drawX.x = 0;
	if(drawX.y >= screenSize.x) drawX.y = screenSize.x - 1;

	spriteResults[spriteNum].spriteWidth = spriteWidth;
	spriteResults[spriteNum].spriteHeight = spriteHeight;
	spriteResults[spriteNum].transformY = transform.y;
	spriteResults[spriteNum].spriteScreenX = spriteScreenX;
	spriteResults[spriteNum].drawX = drawX;
	spriteResults[spriteNum].drawY = drawY;
	spriteResults[spriteNum].vMoveScreen = int(sprite.vMove / transform.y);
	spriteResults[spriteNum].texture = sprite.texture;
}
)";
#pragma endregion

#pragma region raycasterDrawerShader
	const char* raycasterDrawerShader = R"(
#version 460

// Raycaster based on https://lodev.org/cgtutor/raycasting.html
// this file only grabs the calculated line heights, and draws them
// Raycaster for sprites based on https://lodev.org/cgtutor/raycasting3.html
// this file only grabs the calculated sizes and positions and draws them

struct xdata {
	ivec2 draw;
	int side;
	uint textureNum;
	int texX;
	float step;
	float texPos;
	float distWall;
	vec2 floorWall;
};

struct spritedata {
	int spriteWidth;
	int spriteHeight;
	float transformY;
	int spriteScreenX;
	ivec2 drawX;
	ivec2 drawY;
	int vMoveScreen;
	uint texture;
};

in vec2 uvCoord;

layout (location = 0) out vec4 FragColor;

layout(std430, binding=2) buffer raycasterOutput {
	readonly xdata res[10000];
};
layout(std430, binding=3) buffer dataOutput {
	readonly spritedata spriteResults[100];
};
layout(rgba32f, binding=1) uniform image2DArray textures;
layout(location=1) uniform ivec2 screenSize;
layout(location=2) uniform vec2 position;
layout(location=3) uniform uint spriteCount;
layout(location=4) uniform vec4 floorTex;
layout(location=5) uniform vec4 ceilTex;

void drawSprite(int spriteNum, float distWall, float widthf, float heightf) 
{
	spritedata spriteData = spriteResults[spriteNum];

	bool insideX = spriteData.drawX.x <= widthf && widthf <= spriteData.drawX.y;
	bool insideY = spriteData.drawY.x <= heightf && heightf <= spriteData.drawY.y;
	bool validZBuffer = spriteData.transformY > 0 && spriteData.transformY < distWall;
	if(insideX && insideY && validZBuffer) 
	{
		ivec2 texSize = imageSize(textures).xy;
		// here I'm using float calculations because is a bit faster
		int texX = int((widthf - (-spriteData.spriteWidth * 0.5 + spriteData.spriteScreenX)) * texSize.x / spriteData.spriteWidth);
		int vMoveScreen = spriteData.vMoveScreen;
		float d = (heightf - vMoveScreen) - screenSize.y * 0.5 + spriteData.spriteHeight * 0.5;
		int texY = texSize.y - int((d * texSize.y) / spriteData.spriteHeight);

		vec4 color = imageLoad(textures, ivec3(texX, texY, spriteData.texture));
		// i don't know if there is a better way to check if this is black
		if(length(color.rgb) > 0.001) {
			FragColor = color;
		}
	}
}

void main()
{
	xdata data = res[int(uvCoord.x * screenSize.x)];
	float heightf = float(screenSize.y) * uvCoord.y;

	// how much to increase the texture coordinate per screen pixel
	float step = data.step;
	// starting texture coordinate
	float texPos = data.texPos + step * (heightf - data.draw.x);

	if(data.draw.x <= heightf && heightf <= data.draw.y) 
	{
		vec4 color;
		int texHeight = 64;
		// coordinates here are Y-inverted !!
		int texY = texHeight - int(texPos) % texHeight;

		color = imageLoad(textures, ivec3(data.texX, texY, data.textureNum));

		// make color darker for y-sides
		if(data.side == 1) color *= 0.75;
		FragColor = color;
	} else {
		if(data.draw.y < 0)
		data.draw.y = screenSize.y;

		// in fact it is not ceil, is floor, because of Y-inverted stuff on OpenGL
		bool isCeil = heightf < data.draw.y;
		// texture here are inverted because of the previous comment about isCeil
		vec4 tex = isCeil ? floorTex : ceilTex;
		if(tex.a == 0.0f) {
			FragColor = vec4(tex.rgb, 1.0f);
		} else {
			float currentDist;
			if(isCeil)
				currentDist = float(screenSize.y) / (2.0f * (float(screenSize.y) - heightf) - float(screenSize.y));
			else
				currentDist = float(screenSize.y) / (2.0f * heightf - float(screenSize.y));
			//float weight = (currentDist - distPlayer) / (distWall - distPlayer); // atm distPlayer is 0.0f
			float weight = currentDist / data.distWall;
			vec2 currentFloor = vec2(
				weight * data.floorWall.x + (1.0f - weight) * position.x,
				weight * data.floorWall.y + (1.0f - weight) * position.y
			);

			// coordinates here are Y-inverted !!
			ivec2 floorTex = ivec2(
				int(currentFloor.x * 64) % 64,
				64 - int(currentFloor.y * 64) % 64
			);

			FragColor = imageLoad(textures, ivec3(floorTex, int(tex.a)));
		}
	}

	// draws sprites after drawing the rest - this is really slow :/
	heightf = float(screenSize.y) * uvCoord.y;
	float widthf = float(screenSize.x) * uvCoord.x;
	for(int spriteNum = 0; spriteNum < spriteCount; spriteNum += 1) 
	{
		drawSprite(spriteNum, data.distWall, widthf, heightf);
	}
}
)";
#pragma endregion

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

			for (uint32_t x = 0; x < mapWidth; x += 1)
			{
				for (uint32_t y = 0; y < mapHeight; y += 1)
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

	auto raycasterComputeProgram = std::make_shared<GLProgramPipeline>(raycast::raycasterShader);
	auto spritecasterComputeProgram = std::make_shared<GLProgramPipeline>(raycast::spritecasterShader);
	auto raycasterDrawProgram = std::make_shared<GLProgramPipeline>(raycast::vertexShader, raycast::raycasterDrawerShader);

	auto currentMap = raycast::Map::Load();

	glm::vec2 pos = currentMap->initialPos;
	glm::vec2 dir = currentMap->initialDir;
	glm::vec2 plane = currentMap->initialPlane;

	auto raycastResultBuffer = std::make_shared<GLShaderStorageBuffer>(10000 * (5 * sizeof(int32_t) + 5 * sizeof(float)));
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

	while (!Window::ShouldClose())
	{
		Window::Update();

		if (Window::IsResize())
		{
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		}

#pragma region render
		// старт рейкастинга на вычислительном шейдере
		{
			currentMap->texture->BindImage(1);
			raycastResultBuffer->BindBase(2);

			raycasterComputeProgram->Bind();
			raycasterComputeProgram->SetComputeUniform(1, pos);
			raycasterComputeProgram->SetComputeUniform(2, dir);
			raycasterComputeProgram->SetComputeUniform(3, plane);
			raycasterComputeProgram->SetComputeUniform(4, glm::ivec2(Window::GetWidth(), Window::GetHeight())); // TODO: only resize window events
			glDispatchCompute(Window::GetWidth(), 1, 1);
		}

		// старт рендера спрайтов на вычислительном шейдере
		{
			spritecastInputBuffer->BindBase(1);
			spritecastResultBuffer->BindBase(2);

			spritecasterComputeProgram->Bind();
			spritecasterComputeProgram->SetComputeUniform(1, pos);
			spritecasterComputeProgram->SetComputeUniform(2, dir);
			spritecasterComputeProgram->SetComputeUniform(3, plane);
			spritecasterComputeProgram->SetComputeUniform(4, glm::ivec2(Window::GetWidth(), Window::GetHeight())); // TODO: only resize window events
			glDispatchCompute(currentMap->sprites.size(), 1, 1);
		}

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//ожидание завершени€ работы вычислительных шейдеров
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// отрисовка результата на экран через пиксельный шейдер
		{
			raycasterDrawProgram->Bind();
			textures->BindImage(1, 0, false, 0);
			raycastResultBuffer->BindBase(2);
			spritecastResultBuffer->BindBase(3);

			raycasterDrawProgram->SetFragmentUniform(1, glm::ivec2(Window::GetWidth(), Window::GetHeight())); // TODO: only resize window events
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
			ImGui::Begin((const char*)u8"“ест");
			ImGui::Text((const char*)u8"Test/“ест/%s", u8"тест 2");
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