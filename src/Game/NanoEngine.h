#pragma once

//==============================================================================
// BASE HEADER
//==============================================================================
#pragma region Base Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4005)
#	pragma warning(disable : 4244)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 5039)
#endif

#include <cassert>
#include <string>
#include <random>
#include <ratio>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <fstream>
#include <sstream>
#include <variant>
#include <queue>
#include <array>
#include <span>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stb/stb_image.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <Profiler.h>

/*
Left handed
	Y   Z
	|  /
	| /
	|/___X
*/
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_INLINE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/normal.hpp>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion
//==============================================================================
// END BASE HEADER
//==============================================================================

//==============================================================================
// LOG
//==============================================================================
#pragma region Log
void Print(const std::string& text);
void Warning(const std::string& text);
void Error(const std::string& text);
void Fatal(const std::string& text);
#pragma endregion
//==============================================================================
// END LOG
//==============================================================================

//==============================================================================
// Render Core
//==============================================================================
#pragma region Render Core

enum class IndexFormat : uint8_t
{
	UInt8,
	UInt16,
	UInt32
};

struct AttribFormat final
{
	GLuint attribIndex = 0;
	GLint size = 0;
	GLenum type = 0;
	GLuint relativeOffset = 0;
};

template<typename T>
constexpr std::pair<GLint, GLenum> TypeToSizeEnum();

template<typename T>
constexpr inline AttribFormat CreateAttribFormat(GLuint attribIndex, GLuint relativeOffset);

const std::pair<GLenum, GLenum> STBImageToOpenGLFormat(int comp);

#pragma endregion
//==============================================================================
// END Render Core
//==============================================================================

//==============================================================================
// Render Resources
//==============================================================================
#pragma region Render Resources

// ref ARB_separate_shader_objects 
class GLSeparableShaderProgram final
{
public:
	GLSeparableShaderProgram() = delete;
	GLSeparableShaderProgram(GLenum shaderType, std::string_view sourceCode);	
	~GLSeparableShaderProgram();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	template <typename T>
	void SetUniform(GLint location, const T& value);

private:
	void createHandle(GLenum shaderType, std::string_view sourceCode);
	void destroyHandle();
	void validate(std::string_view sourceCode);

	GLuint m_handle = 0;
};
using GLSeparableShaderProgramRef = std::shared_ptr<GLSeparableShaderProgram>;

class GLProgramPipeline final
{
public:
	GLProgramPipeline() = delete;
	GLProgramPipeline(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef fragmentShader);
	GLProgramPipeline(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef geometryShader, GLSeparableShaderProgramRef fragmentShader);
	GLProgramPipeline(GLSeparableShaderProgramRef computeShader);
	
	GLProgramPipeline(std::string_view vertexShaderCode, std::string_view fragmentShaderCode);
	GLProgramPipeline(std::string_view vertexShaderCode, std::string_view geometryShaderCode, std::string_view fragmentShaderCode);
	GLProgramPipeline(std::string_view computeShaderCode);

	~GLProgramPipeline();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	[[nodiscard]] GLSeparableShaderProgramRef GetVertexShader() noexcept { return m_vertexShader; }
	[[nodiscard]] GLSeparableShaderProgramRef GetGeometryShader() noexcept { return m_geometryShader; }
	[[nodiscard]] GLSeparableShaderProgramRef GetFragmentShader() noexcept { return m_fragmentShader; }
	[[nodiscard]] GLSeparableShaderProgramRef GetComputeShader() noexcept { return m_computeShader; }

	template <typename T>
	void SetVertexUniform(GLint location, const T& value);
	template <typename T>
	void SetGeometryUniform(GLint location, const T& value);
	template <typename T>
	void SetFragmentUniform(GLint location, const T& value);
	template <typename T>
	void SetComputeUniform(GLint location, const T& value);

	void Bind();

private:
	void createHandle();
	void destroyHandle();
	void setSeparableShaders(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef geometryShader, GLSeparableShaderProgramRef fragmentShader, GLSeparableShaderProgramRef computeShader);

	GLuint m_handle = 0;
	GLSeparableShaderProgramRef m_vertexShader = nullptr;
	GLSeparableShaderProgramRef m_geometryShader = nullptr;
	GLSeparableShaderProgramRef m_fragmentShader = nullptr;
	GLSeparableShaderProgramRef m_computeShader = nullptr;

};
using GLProgramPipelineRef = std::shared_ptr<GLProgramPipeline>;

// это Storage Buffer. возможно сделать возможность создания простого или такого буффера
// https://steps3d.narod.ru/tutorials/buffer-storage-tutorial.html
class GLBuffer final
{
public:
	GLBuffer() = delete;
	template<typename T>
	GLBuffer(const std::vector<T>& buff, GLenum flags = GL_DYNAMIC_STORAGE_BIT);
	~GLBuffer();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }
		
	template<typename T>
	void SetData(const std::vector<T>& buff, GLenum usage = GL_DYNAMIC_DRAW); // TODO: проверить
	template<typename T>
	void SetSubData(GLintptr offset, const std::vector<T>& buff);

	[[nodiscard]] void* MapRange(GLintptr offset, GLsizeiptr size, GLbitfield accessFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	[[nodiscard]] void* Map(GLbitfield access = GL_READ_WRITE);// TODO: проверить
	void FlushMappedRange(GLintptr offset, GLsizeiptr size);
	void Unmap();

private:
	void createHandle();
	void destroyHandle();

	GLuint m_handle = 0;
};
using GLBufferRef = std::shared_ptr<GLBuffer>;

class GLVertexArray final
{
public:
	GLVertexArray() = delete;
	GLVertexArray(GLBufferRef vbo, size_t vertexSize, const std::vector<AttribFormat>& attribFormats);
	GLVertexArray(GLBufferRef vbo, size_t vertexSize, GLBufferRef ibo, IndexFormat indexFormat, const std::vector<AttribFormat>& attribFormats);
	template<typename T>
	GLVertexArray(const std::vector<T>& vertices, const std::vector<uint8_t>& indices, const std::vector<AttribFormat>& attribFormats);
	template<typename T>
	GLVertexArray(const std::vector<T>& vertices, const std::vector<uint16_t>& indices, const std::vector<AttribFormat>& attribFormats);
	template<typename T>
	GLVertexArray(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const std::vector<AttribFormat>& attribFormats);

	~GLVertexArray();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void Bind();

private:
	void createHandle();
	void destroyHandle();
	void setAttribFormats(const std::vector<AttribFormat>& attribFormats);
	void setVertexBuffer(GLBufferRef vbo, size_t vertexSize);
	void setVertexBuffer(GLuint bindingIndex, GLBufferRef vbo, GLintptr offset = 0, size_t stride = 1);
	void setIndexBuffer(GLBufferRef ibo, IndexFormat indexFormat);

	GLuint m_handle = 0;
	GLBufferRef m_vbo = nullptr;
	GLBufferRef m_ibo = nullptr;
	size_t m_vertexSize = 0;
	IndexFormat m_indexFormat = IndexFormat::UInt32;
};
using GLVertexArrayRef = std::shared_ptr<GLVertexArray>;

class GLTexture2D final
{
public:
	GLTexture2D() = delete;
	GLTexture2D(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, bool generateMipMaps = false);
	GLTexture2D(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, bool generateMipMaps = false);
	GLTexture2D(std::string_view filepath, int comp = STBI_rgb_alpha, bool generateMipMaps = false);

	~GLTexture2D();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void Bind(GLuint slot);

private:
	void createHandle();
	void destroyHandle();
	void createTexture(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, bool generateMipMaps = false);

	GLuint m_handle = 0;
};
using GLTexture2DRef = std::shared_ptr<GLTexture2D>;

class GLTextureCube final
{
public:
	GLTextureCube() = delete;
	template<typename T = nullptr_t>
	GLTextureCube(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, const std::array<T*, 6>& data);
	GLTextureCube(const std::array<std::string_view, 6>& filepath, int comp = STBI_rgb_alpha);

	~GLTextureCube();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void Bind(GLuint slot);

private:
	void createHandle();
	void destroyHandle();
	template<typename T = nullptr_t>
	void createTexture(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, const std::array<T*, 6>& data);

	GLuint m_handle = 0;
};
using GLTextureCubeRef = std::shared_ptr<GLTextureCube>;

class GLFramebuffer final
{
public:
	GLFramebuffer() = delete;
	GLFramebuffer(const std::vector<GLTexture2DRef>& colors, GLTexture2DRef depth = nullptr);
	~GLFramebuffer();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void ClearFramebuffer(GLenum buffer, GLint drawbuffer, const GLfloat* value);
	void Bind();

private:
	void createHandle();
	void destroyHandle();
	void setTextures(const std::vector<GLTexture2DRef>& colors, GLTexture2DRef depth);

	GLuint m_handle = 0;
	std::vector<GLTexture2DRef> m_colorTextures;
	GLTexture2DRef m_depthTexture = nullptr;
};
using GLFramebufferRef = std::shared_ptr<GLFramebuffer>;

[[nodiscard]] inline bool IsValid(GLSeparableShaderProgramRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLProgramPipelineRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLBufferRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLVertexArrayRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLTexture2DRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLTextureCubeRef resource) noexcept { return resource && resource->IsValid(); }
[[nodiscard]] inline bool IsValid(GLFramebufferRef resource) noexcept { return resource && resource->IsValid(); }

#pragma endregion
//==============================================================================
// END Render Resources
//==============================================================================

//==============================================================================
// Renderer3D
//==============================================================================
#pragma region Renderer3D
class Renderer3D final
{
public:
	void MainFrameBuffer();
	void BlitFrameBuffer(GLFramebufferRef readFramebuffer, GLFramebufferRef drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

	void Clear(bool color, bool depth = false, bool stencil = false);
	void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void SetScissor(GLint x, GLint y, GLsizei width, GLsizei height);
};
#pragma endregion
//==============================================================================
// END Renderer3D
//==============================================================================

//==============================================================================
// include inline header
//==============================================================================
#include "NanoEngine.inl"