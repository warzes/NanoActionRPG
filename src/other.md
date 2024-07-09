================================================================================
texture image in glsl
================================================================================
Позволяет работать с координатами текстуры в пикселях, а не UV

создание текстуры
	texture = std::make_shared<GLTexture2D>(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, width, height, data, GL_NEAREST, GL_REPEAT)

бинд текстуры как image
	texture->BindImage(1);

В шейдере:
layout(r8ui, binding=1) uniform uimage2D texData;

uvec2 texSize = imageSize(texData); // размер текстуры
uint value = imageLoad(texData, Pos.xy).r; // значение