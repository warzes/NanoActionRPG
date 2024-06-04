#pragma once

//==============================================================================
// BASE HEADER
//==============================================================================
#pragma region Base Header

#if defined(_MSC_VER)
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

	operator GLuint() const noexcept { return m_handle; }
	bool IsValid() const noexcept { return m_handle != 0; }

	template <typename T>
	void SetUniform(GLint location, const T& value)
	{
		if (!IsValid())
		{
			Error("shader is null");
			return;
		}
		if constexpr (std::is_same_v<T, GLint>) glProgramUniform1i(m_handle, location, value);
		else if constexpr (std::is_same_v<T, GLuint>) glProgramUniform1ui(m_handle, location, value);
		else if constexpr (std::is_same_v<T, bool>) glProgramUniform1ui(m_handle, location, value);
		else if constexpr (std::is_same_v<T, GLfloat>) glProgramUniform1f(m_handle, location, value);
		else if constexpr (std::is_same_v<T, GLdouble>) glProgramUniform1d(m_handle, location, value);
		else if constexpr (std::is_same_v<T, glm::vec2>) glProgramUniform2fv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::vec3>) glProgramUniform3fv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::vec4>) glProgramUniform4fv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::ivec2>) glProgramUniform2iv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::ivec3>) glProgramUniform3iv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::ivec4>) glProgramUniform4iv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::uvec2>) glProgramUniform2uiv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::uvec3>) glProgramUniform3uiv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::uvec4>) glProgramUniform4uiv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::quat>) glProgramUniform4fv(m_handle, location, 1, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::mat3>) glProgramUniformMatrix3fv(m_handle, location, 1, GL_FALSE, glm::value_ptr(value));
		else if constexpr (std::is_same_v<T, glm::mat4>) glProgramUniformMatrix4fv(m_handle, location, 1, GL_FALSE, glm::value_ptr(value));
		else Fatal("unsupported type");
	}

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
	~GLProgramPipeline();

	operator GLuint() const noexcept { return m_handle; }
	bool IsValid() const noexcept { return m_handle != 0; }

	GLSeparableShaderProgramRef GetVertexShader() { return m_vertexShader; }
	GLSeparableShaderProgramRef GetGeometryShader() { return m_geometryShader; }
	GLSeparableShaderProgramRef GetFragmentShader() { return m_fragmentShader; }
	GLSeparableShaderProgramRef GetComputeShader() { return m_computeShader; }

	template <typename T>
	void SetVertexUniform(GLint location, const T& value)
	{
		if (m_vertexShader) m_vertexShader->SetUniform<T>(location, value);
	}
	template <typename T>
	void SetGeometryUniform(GLint location, const T& value)
	{
		if (m_geometryShader) m_geometryShader->SetUniform<T>(location, value);
	}
	template <typename T>
	void SetFragmentUniform(GLint location, const T& value)
	{
		if (m_fragmentShader) m_fragmentShader->SetUniform<T>(location, value);
	}
	template <typename T>
	void SetComputeUniform(GLint location, const T& value)
	{
		if (m_computeShader) m_computeShader->SetUniform<T>(location, value);
	}

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

inline bool IsValid(GLSeparableShaderProgramRef resource) noexcept { return resource && resource->IsValid(); }
inline bool IsValid(GLProgramPipelineRef resource) noexcept { return resource && resource->IsValid(); }
//inline bool IsValid(GLBufferRef resource) const noexcept { return resource && resource->IsValid(); }
//inline bool IsValid(GLVertexArrayRef resource) const noexcept { return resource && resource->IsValid(); }
//inline bool IsValid(GLGeometryRef resource) const noexcept { return resource && IsValid(resource->vao); }
//inline bool IsValid(GLTextureRef resource) const noexcept { return resource && resource->IsValid(); }
//inline bool IsValid(GLFramebufferRef resource) const noexcept { return resource && resource->IsValid(); }


#pragma endregion
//==============================================================================
// END Render Resources
//==============================================================================


//==============================================================================
// Renderer3D
//==============================================================================
#pragma region Renderer3D
struct Renderer3D
{

};
#pragma endregion
//==============================================================================
// END Renderer3D
//==============================================================================