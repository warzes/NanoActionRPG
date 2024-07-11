#pragma once

//==============================================================================
// BASE HEADER
//==============================================================================
#pragma region Base Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4099)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4005)
#	pragma warning(disable : 4244)
#	pragma warning(disable : 4619)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 5039)
#	pragma warning(disable : 5219)
#endif

#define _USE_MATH_DEFINES

#include <cassert>
#include <cstring>
#include <cmath>
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
//#define GLM_FORCE_LEFT_HANDED
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

#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/ai_assert.h>
#include <assimp/texture.h>
#include <assimp/Vertex.h>
#include <assimp/Bitmap.h>

#include <physx/PxPhysicsAPI.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion

//==============================================================================
// CORE
//==============================================================================
#pragma region Core

class Time final
{
public:
	constexpr Time() = default;
	template <typename Rep, typename Period>
	constexpr Time(const std::chrono::duration<Rep, Period>& duration);

	[[nodiscard]] constexpr float AsSeconds() const;
	[[nodiscard]] constexpr int32_t AsMilliseconds() const;
	[[nodiscard]] constexpr int64_t AsMicroseconds() const;

	[[nodiscard]] constexpr std::chrono::microseconds ToDuration() const;
	template <typename Rep, typename Period>
	constexpr operator std::chrono::duration<Rep, Period>() const;

private:
	std::chrono::microseconds m_microseconds{};
};

class Clock final
{
public:
	[[nodiscard]] Time GetElapsedTime() const;

	[[nodiscard]] bool IsRunning() const;

	void Start();
	void Stop();
	Time Restart();
	Time Reset();
private:
	using ClockImpl = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

	ClockImpl::time_point m_refPoint{ ClockImpl::now() };
	ClockImpl::time_point m_stopPoint;
};

#pragma endregion

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
// Math
//==============================================================================
#pragma region Math

[[nodiscard]] inline int NumMipmap(int width, int height)
{
	return static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
}

[[nodiscard]] inline glm::mat4 MatrixCast(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

class AABB final
{
public:
	AABB() = default;
	AABB(const glm::vec3& inMin, const glm::vec3& inMax);
	AABB(const std::vector<glm::vec3>& points);

	[[nodiscard]] float GetVolume() const;

	[[nodiscard]] glm::vec3 GetCenter() const;
	[[nodiscard]] glm::vec3 GetHalfSize() const;
	[[nodiscard]] glm::vec3 GetDiagonal() const;

	[[nodiscard]] float GetSurfaceArea() const;

	[[nodiscard]] void Combine(const AABB& anotherAABB);
	[[nodiscard]] void Combine(const glm::vec3& point);
	[[nodiscard]] bool Overlaps(const AABB& anotherAABB);
	[[nodiscard]] bool Inside(const glm::vec3& point);		

	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

class OBB final
{
public:
	glm::mat4 tm = glm::mat4(1.0f);
	glm::vec3 halfExtents = glm::vec3(0.0f);
};

class Plane final
{
public:
	// The form is ax + by + cz + d = 0
	// where: d = dot(n, p)
	glm::vec3 n = glm::vec3(0.0f);
	float d = 0.0f;
};

class Frustum final
{
public:
	Plane planes[6] = {};
};

class Sphere final
{
public:
	Sphere() = default;

	[[nodiscard]] float GetVolume() const;

	[[nodiscard]] bool Inside(const glm::vec3& point);

	glm::vec3 center = glm::vec3(0.0f);
	float radius = 0.0f;
};

class Transform final
{
public:
	Transform() = default;
	Transform(const glm::vec3& position, const glm::mat3& orientation);
	Transform(const glm::vec3& position, const glm::quat& orientation);

	[[nodiscard]] const glm::vec3& GetPosition() const;
	[[nodiscard]] const glm::quat& GetOrientation() const;

	void SetIdentity();
	void SetPosition(const glm::vec3& position);
	void SetOrientation(const glm::quat& orientation);

	[[nodiscard]] Transform GetInverse() const;

	glm::vec3 operator*(const glm::vec3& v) const;
	Transform operator*(const Transform& tr) const;

private:
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
};

#pragma endregion

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

std::string LoadShaderTextFile(const std::string& fileName);

#pragma endregion

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

	[[nodiscard]] GLint GetVertexUniform(const std::string& name) const;
	[[nodiscard]] GLint GetGeometryUniform(const std::string& name) const;
	[[nodiscard]] GLint GetFragmentUniform(const std::string& name) const;
	[[nodiscard]] GLint GetComputeUniform(const std::string& name) const;

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

	[[nodiscard]] size_t GetElementSize() const { return m_elementSize; }
	[[nodiscard]] size_t GetElementCount() const { return m_elementCount; }
	
private:
	void createHandle();
	void destroyHandle();

	GLuint m_handle = 0;
	size_t m_elementSize = 0;
	size_t m_elementCount = 0;
};
using GLBufferRef = std::shared_ptr<GLBuffer>;

class GLVertexArray final
{
public:
	GLVertexArray();
	GLVertexArray(GLBufferRef vbo, const std::vector<AttribFormat>& attribFormats);
	GLVertexArray(GLBufferRef vbo, GLBufferRef ibo, const std::vector<AttribFormat>& attribFormats);
	template<typename T>
	GLVertexArray(const std::vector<T>& vertices, const std::vector<AttribFormat>& attribFormats);
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

	void DrawTriangles();

private:
	void createHandle();
	void destroyHandle();
	void setAttribFormats(const std::vector<AttribFormat>& attribFormats);
	void setVertexBuffer(GLBufferRef vbo);
	void setVertexBuffer(GLuint bindingIndex, GLBufferRef vbo, GLintptr offset = 0, size_t stride = 1);
	void setIndexBuffer(GLBufferRef ibo);

	GLuint m_handle = 0;
	GLBufferRef m_vbo = nullptr;
	GLBufferRef m_ibo = nullptr;
};
using GLVertexArrayRef = std::shared_ptr<GLVertexArray>;

// TODO: возможно объединить с GLBuffer
class GLShaderStorageBuffer final
{
public:
	GLShaderStorageBuffer() = delete;
	GLShaderStorageBuffer(size_t size, GLenum usage = GL_STREAM_COPY);
	template<typename T>
	GLShaderStorageBuffer(const std::vector<T>& buff, GLenum usage = GL_STREAM_COPY);
	~GLShaderStorageBuffer();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void BindBase(uint32_t index);

	template<typename T>
	void SetData(const std::vector<T>& buff);

private:
	void createHandle();
	void destroyHandle();

	GLuint m_handle = 0;
	GLenum m_usage = 0;
};
using GLShaderStorageBufferRef = std::shared_ptr<GLShaderStorageBuffer>;

class GLTexture2D final
{
public:
	GLTexture2D() = delete;
	GLTexture2D(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, bool generateMipMaps = false);
	GLTexture2D(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, const glm::vec4& borderColor = glm::vec4(0.0f), bool generateMipMaps = false);
	GLTexture2D(std::string_view filepath, int comp = STBI_rgb_alpha, bool generateMipMaps = false);
	~GLTexture2D();

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

	void Bind(GLuint slot);
	void BindImage(uint32_t index, uint32_t level = 0, bool write = false, std::optional<int> layer = std::nullopt);

private:
	void createHandle();
	void destroyHandle();
	void createTexture(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data = nullptr, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT, const glm::vec4& borderColor = glm::vec4(0.0f), bool generateMipMaps = false);

	GLuint m_handle = 0;
	GLenum m_internalFormat = 0;
};
using GLTexture2DRef = std::shared_ptr<GLTexture2D>;

class GLTexture2DArray final
{
public:
	GLTexture2DArray(const std::vector<std::string_view>& filepath, GLenum internalFormat, glm::ivec3 size, int comp = STBI_rgb_alpha, size_t levels = 1, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT);
	~GLTexture2DArray();

	void BindImage(uint32_t index, uint32_t level = 0, bool write = false, std::optional<int> layer = std::nullopt);

	[[nodiscard]] operator GLuint() const noexcept { return m_handle; }
	[[nodiscard]] bool IsValid() const noexcept { return m_handle != 0; }

private:
	void createHandle();
	void destroyHandle();
	void fillSubImage(int level, glm::ivec3 offset, glm::ivec3 size, GLenum format, GLenum dataType, const void* data);

	GLuint m_handle = 0;
	GLenum m_internalFormat = 0;
};
using GLTexture2DArrayRef = std::shared_ptr<GLTexture2DArray>;

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
// Renderer
//==============================================================================
#pragma region Renderer

namespace Renderer
{
	bool Init();
	void Close();

	void MainFrameBuffer();
	void BlitFrameBuffer(GLFramebufferRef readFramebuffer, GLFramebufferRef drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

	void Clear(bool color, bool depth = false, bool stencil = false);
	void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void SetScissor(GLint x, GLint y, GLsizei width, GLsizei height);
}

#pragma endregion

//==============================================================================
// Graphics
//==============================================================================
#pragma region Graphics

class Node
{
public:
	virtual ~Node() = default;

	void SetParent(Node* parent);

	virtual void SetTransform(const Transform& transform);
	virtual void SetSize(const glm::vec3& size);
	virtual void AddChild(Node* child);

	virtual void Draw();

	Node* GetParent();
	std::vector<Node*> GetChildren();

	virtual Transform GetTransform() = 0;
	virtual glm::vec3 GetSize();

	static Transform GetFinalTransform(Node* node, Transform tr = {});

protected:
	Node* m_parent = nullptr;
	std::vector<Node*> m_children;
};

struct MaterialTexture final
{
	GLTexture2DRef texture = nullptr;
	std::string path;
};

struct MaterialProperties final
{
	glm::vec3 diffuseColor;
	glm::vec3 ambientColor;
	glm::vec3 specularColor;
	float shininess = 0.0f;
	float refracti = 0.0f;
};

constexpr size_t MAX_NUM_BONES_PER_VERTEX = 4;

struct MeshVertex final
{
	void AddBoneData(uint32_t boneID, float weight);

	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoords;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	uint32_t boneIDs[MAX_NUM_BONES_PER_VERTEX] = {}; // Bone indexes which will influence this vertex
	float weights[MAX_NUM_BONES_PER_VERTEX] = {};    // Weights from each bone
};

constexpr inline std::vector<AttribFormat> GetMeshVertexFormat();

class Mesh final
{
public:
	Mesh() = delete;
	Mesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<MaterialTexture>& textures, const MaterialProperties& materialProperties);

	[[nodiscard]] AABB GetBounding() const;
	[[nodiscard]] std::vector<glm::vec3> GetTriangle() const;
	[[nodiscard]] GLVertexArrayRef GetVAO();

	void Draw(const GLProgramPipelineRef& program);

private:
	void init();

	std::vector<MeshVertex> m_vertices;
	std::vector<MaterialTexture> m_textures;
	std::vector<uint32_t> m_indices;
	AABB m_bounding;
	MaterialProperties m_materialProp;
	GLVertexArrayRef m_vao = nullptr;
	int m_diffuseColorLoc = -1;
	int m_ambientColorLoc = -1;
	int m_specularColorLoc = -1;
	int m_shininessLoc = -1;
	int m_refractiLoc = -1;

};
using MeshRef = std::shared_ptr<Mesh>;

struct Keyframe final
{
	std::vector<float> posStamps;
	std::vector<float> rotStamps;
	std::vector<float> scaleStamps;

	std::vector<glm::vec3> positions;
	std::vector<glm::quat> rotations;
	std::vector<glm::vec3> scales;
};

class Animation final
{
public:
	enum class State
	{
		Stopped,
		Playing,
		Paused
	};

	Animation(const std::string& name);

	void SetName(const std::string& name);
	void SetTPS(float tps);
	void SetDuration(float duration);
	void SetIsRepeated(bool repeat);
	void SetIsBlending(bool blending);
	void SetLastTime(float lastTime);

	void AddKeyframe(const std::string& name, const Keyframe& keyframe);

	void Play(float time = 0);
	void Pause();
	void Stop();

	std::unordered_map<std::string, std::pair<Transform, glm::vec3>> Update();

	std::unordered_map<std::string, Keyframe>& GetKeyframes();
	std::string GetName() const;

	bool IsRepeated() const;
	bool IsBlending() const;
	float GetTime() const;
	float GetLastTime() const;
	float GetDuration() const;
	float GetTPS() const;

	State GetState() const;

private:
	std::string m_name;
	State m_state = State::Stopped;
	bool m_repeat = true;
	bool m_blending = false;

	float m_duration = 0.0f;
	float m_tps = 30.0f;
	float m_lastTime = 0.0f;
	std::unordered_map<std::string, Keyframe> m_keyframes;

	Clock m_time;
};
using AnimationRef = std::shared_ptr<Animation>;

class Bone final : public Node
{
public:
	Bone() = delete;
	Bone(int id, const std::string& name, const glm::mat4& offset);

	void SetTransform(const Transform& tr) final;

	void SetPosition(const glm::vec3& pos);
	void SetOrientation(const glm::quat& orient);
	void SetSize(const glm::vec3& size) final;

	void Move(const glm::vec3& vec);
	void Rotate(const glm::quat& quat);
	void Expand(const glm::vec3& vec);

	void SavePoseAsIdle();

	int GetID();
	std::string GetName();

	glm::vec3 GetPosition();
	glm::quat GetOrientation();
	glm::vec3 GetSize() override;

	glm::mat4 GetOffset();
	Transform GetTransform() override;
	Transform GetIdle();

private:
	int m_id = 0;
	std::string m_name = "";
	Transform m_transform;
	Transform m_idle;
	glm::mat4 m_offset;
	glm::vec3 m_size;
};
using BoneRef = std::shared_ptr<Bone>;

class Model final : public Node
{
public:
	Model() = delete;
	Model(const std::string& modelPath, bool flipUV = true);

	void Draw(const GLProgramPipelineRef& program);

	[[nodiscard]] AABB GetBounding() const;
	[[nodiscard]] std::vector<glm::vec3> GetTriangle() const;
	std::vector<AnimationRef> GetAnimations() const;
	std::vector<BoneRef> GetBones() const;
	std::vector<glm::mat4>& GetPose();

	[[nodiscard]] MeshRef operator[](size_t idx);

	void SetTransform(const Transform& transform) final;
	void SetPosition(const glm::vec3& position);
	void SetOrientation(const glm::quat& orientation);
	void AddChild(Node* child) final;

	Transform GetTransform() final;
	glm::vec3 GetPosition();
	glm::quat GetOrientation();

	void UpdateAnim(); // TODO: временно пока делаю
	void DefaultPose();

private:
	void loadAssimpModel(const std::string& modelPath, bool flipUV);
	void loadAnimations(const aiScene* scene);
	void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
	MeshRef processMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
	void processVertex(const aiMesh* AiMesh, const glm::mat4& transform, std::vector<MeshVertex>& vertices);
	void processIndices(const aiMesh* AiMesh, std::vector<uint32_t>& indices);
	void processBones(const aiMesh* mesh, std::vector<MeshVertex>& vertices);
	void findBoneNodes(aiNode* node, std::vector<BoneRef>& bones);
	bool processBone(aiNode* node, BoneRef& out);
	void processTextures(const aiMesh* AiMesh, const aiScene* scene, std::vector<MaterialTexture>& textures);
	void loadTextureFromMaterial(aiTextureType textureType, const aiMaterial* mat, std::vector<MaterialTexture>& textures);
	void processMatProperties(const aiMesh* AiMesh, const aiScene* scene, MaterialProperties& meshMatProperties);
	void computeAABB();

	void calculatePose(Bone* bone);

	int m_meshCount = -1;
	std::vector<MaterialTexture> m_loadedTextures;
	std::vector<MeshRef> m_meshes;
	// animations data
	std::unordered_map<std::string, std::pair<int, glm::mat4>> m_bonemap;
	std::vector<glm::mat4> m_pose; // TODO: вместо вектора массив из 64
	std::vector<AnimationRef> m_animations;
	std::vector<BoneRef> m_bones;
	std::vector<BoneRef> m_bonesChildren;

	std::string m_directory;
	AABB m_bounding;

	glm::mat4 m_globalInverseTransform;

	Transform m_transform;

};
using ModelRef = std::shared_ptr<Model>;

#pragma endregion

//==============================================================================
// Scene
//==============================================================================
#pragma region Scene

// TODO: shapes перенести в Model как статические методы создания Model, а не отдельный класс

class QuadShape final
{
public:
	QuadShape();

	void Draw();
private:
	std::vector<MeshVertex> getData();
	GLVertexArrayRef m_vao;
};
using QuadShapeRef = std::shared_ptr<QuadShape>;

class CubeShape final
{
public:
	CubeShape();

	void Draw();
private:
	std::vector<MeshVertex> getData();
	GLVertexArrayRef m_vao;
};
using CubeShapeRef = std::shared_ptr<CubeShape>;

class SphereShape final
{
public:
	SphereShape();

	void Draw();
private:
	GLVertexArrayRef m_vao;
};
using SphereShapeRef = std::shared_ptr<SphereShape>;

constexpr auto CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
#ifdef GLM_FORCE_LEFT_HANDED
constexpr auto CAMERA_FRONT = glm::vec3(0.0f, 0.0f, 1.0f);
#else
constexpr auto CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
#endif
constexpr auto CAMERA_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr float CAMERA_YAW = -90.0f;
constexpr float CAMERA_PITCH = 0.0f;
constexpr float CAMERA_SPEED = 10.0f;
constexpr float CAMERA_SENSITIVITY = 0.1f;

class Camera final
{
public:
	enum MovementDir
	{
		Forward,
		Backward,
		Left,
		Right
	};

	void Set(
		glm::vec3 position = glm::vec3(0.0f),
		glm::vec3 up = CAMERA_UP,
		float yaw = CAMERA_YAW,
		float pitch = CAMERA_PITCH);

	void Move(MovementDir direction, float deltaTime);
	void Rotate(float xOffset, float yOffset); // TODO: дельтатайм нужна?

	[[nodiscard]] const glm::mat4& GetViewMatrix() const;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 front = CAMERA_FRONT;
	glm::vec3 up = CAMERA_UP;
	glm::vec3 right = CAMERA_RIGHT;
	glm::vec3 worldUp = CAMERA_UP;

	float yaw = CAMERA_YAW;
	float pitch = CAMERA_PITCH;

	float movementSpeed = CAMERA_SPEED;
	float mouseSensitivity = CAMERA_SENSITIVITY;

private:
	void update();
	glm::mat4 m_view;
};

#pragma endregion

//==============================================================================
// Window
//==============================================================================
#pragma region Window

namespace Window
{
	bool Create(const char* title, int width, int height);
	void Destroy();

	bool ShouldClose();
	void Update();
	void Swap();

	GLFWwindow* GetWindow();
	int GetWidth();
	int GetHeight();
	bool IsResize();
}

#pragma endregion

//==============================================================================
// Input
//==============================================================================
#pragma region Input

namespace Keyboard
{
	bool IsPressed(int key);
}

namespace Mouse
{
	enum class Button
	{
		Left = GLFW_MOUSE_BUTTON_LEFT,
		Right = GLFW_MOUSE_BUTTON_RIGHT,
		Middle = GLFW_MOUSE_BUTTON_MIDDLE,
	};

	enum class CursorMode
	{
		Disabled,
		Hidden,
		Normal
	};

	bool IsPressed(Button button);
	glm::ivec2 GetPosition();
	glm::ivec2 GetDelta();
	void SetPosition(const glm::ivec2& position);

	void SetCursorMode(CursorMode mode);
}

#pragma endregion

//==============================================================================
// IMGUI
//==============================================================================
#pragma region IMGUI

namespace IMGUI
{
	bool Init();
	void Close();

	void Update();
	void Draw();
}
#pragma endregion


//==============================================================================
// include inline header
//==============================================================================
#include "NanoEngine.inl"