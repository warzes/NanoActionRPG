//==============================================================================
// Math
//==============================================================================
#pragma region Math

inline AABB::AABB(const glm::vec3& inMin, const glm::vec3& inMax) 
	: min(inMin), max(inMax)
{
}

inline AABB::AABB(const std::vector<glm::vec3>& points)
{
	for (size_t i = 0; i < points.size(); i++)
		Combine(points[i]);
}

inline float AABB::GetVolume() const
{
	glm::vec3 diagonal = max - min;
	return diagonal.x * diagonal.y * diagonal.z;
}

inline glm::vec3 AABB::GetCenter() const
{
	return (min + max) * 0.5f;
}

inline glm::vec3 AABB::GetHalfSize() const
{
	return (max - min) * 0.5f;
}

inline glm::vec3 AABB::GetDiagonal() const
{
	return max - min;
}

inline float AABB::GetSurfaceArea() const
{
	glm::vec3 diagonal = max - min;
	return 2.0f * (diagonal.x * diagonal.y + diagonal.y * diagonal.z + diagonal.z * diagonal.x);
}

inline void AABB::Combine(const AABB& anotherAABB)
{
	min = glm::min(min, anotherAABB.min);
	max = glm::max(max, anotherAABB.max);
}

inline void AABB::Combine(const glm::vec3& point)
{
	min = glm::min(min, point);
	max = glm::max(max, point);
}

inline bool AABB::Overlaps(const AABB& anotherAABB)
{
	return max.x > anotherAABB.min.x && min.x < anotherAABB.max.x
		&& max.y > anotherAABB.min.y && min.y < anotherAABB.max.y
		&& max.z > anotherAABB.min.z && min.z < anotherAABB.max.z;
}

inline bool AABB::Inside(const glm::vec3& point)
{
	return max.x > point.x && min.x < point.x
		&& max.y > point.y && min.y < point.y
		&& max.z > point.z && min.z < point.x;
}

#pragma endregion

//==============================================================================
// Render Core
//==============================================================================
#pragma region Render Core

template<typename T>
inline constexpr std::pair<GLint, GLenum> TypeToSizeEnum()
{
	if constexpr (std::is_same_v<T, float>) return std::make_pair(1, GL_FLOAT);
	if constexpr (std::is_same_v<T, int>) return std::make_pair(1, GL_INT);
	if constexpr (std::is_same_v<T, unsigned>) return std::make_pair(1, GL_UNSIGNED_INT);
	if constexpr (std::is_same_v<T, glm::vec2>) return std::make_pair(2, GL_FLOAT);
	if constexpr (std::is_same_v<T, glm::vec3>) return std::make_pair(3, GL_FLOAT);
	if constexpr (std::is_same_v<T, glm::vec4>) return std::make_pair(4, GL_FLOAT);
}

template<typename T>
inline constexpr AttribFormat CreateAttribFormat(GLuint attribIndex, GLuint relativeOffset)
{
	const auto [compCount, type] = TypeToSizeEnum<T>();
	return { attribIndex, compCount, type, relativeOffset };
}

#pragma endregion

//==============================================================================
// Render Resources
//==============================================================================
#pragma region Render Resources

template<typename T>
inline void GLSeparableShaderProgram::SetUniform(GLint location, const T& value)
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

template <typename T>
inline void GLProgramPipeline::SetVertexUniform(GLint location, const T& value)
{
	if (m_vertexShader && location >= 0) 
		m_vertexShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetGeometryUniform(GLint location, const T& value)
{
	if (m_geometryShader && location >= 0) 
		m_geometryShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetFragmentUniform(GLint location, const T& value)
{
	if (m_fragmentShader && location >= 0) 
		m_fragmentShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetComputeUniform(GLint location, const T& value)
{
	if (m_computeShader && location >= 0) 
		m_computeShader->SetUniform<T>(location, value);
}

template<typename T>
inline GLBuffer::GLBuffer(const std::vector<T>& buff, GLenum flags)
{
	createHandle();
	glNamedBufferStorage(m_handle, sizeof(typename std::vector<T>::value_type) * buff.size(), buff.data(), flags);
}

template<typename T>
inline void GLBuffer::SetData(const std::vector<T>& buff, GLenum usage)
{
	glNamedBufferData(m_handle, sizeof(typename std::vector<T>::value_type) * buff.size(), buff.data(), usage);
}

template<typename T>
inline void GLBuffer::SetSubData(GLintptr offset, const std::vector<T>& buff)
{
	glNamedBufferSubData(m_handle, offset, sizeof(typename std::vector<T>::value_type) * buff.size(), buff.data());
}

template<typename T>
inline GLVertexArray::GLVertexArray(const std::vector<T>& vertices, const std::vector<uint8_t>& indices, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(
		std::make_shared<GLBuffer>(vertices), sizeof(T),
		std::make_shared<GLBuffer>(indices), indices.size(), IndexFormat::UInt8,
		attribFormats)
{
}

template<typename T>
inline GLVertexArray::GLVertexArray(const std::vector<T>& vertices, const std::vector<uint16_t>& indices, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(
		std::make_shared<GLBuffer>(vertices), sizeof(T),
		std::make_shared<GLBuffer>(indices), indices.size(), IndexFormat::UInt16,
		attribFormats)
{
}

template<typename T>
inline GLVertexArray::GLVertexArray(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(
		std::make_shared<GLBuffer>(vertices), sizeof(T),
		std::make_shared<GLBuffer>(indices), indices.size(), IndexFormat::UInt32,
		attribFormats)
{
}

template<typename T>
inline GLTextureCube::GLTextureCube(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, const std::array<T*, 6>& data)
{
	createTexture(internalFormat.format, width, height, data);
}

template<typename T>
inline void GLTextureCube::createTexture(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, const std::array<T*, 6>& data)
{
	createHandle();
	glTextureStorage2D(m_handle, 1, internalFormat, width, height);
	for (size_t i = 0; i < 6; i++)
	{
		if (data[i])
			glTextureSubImage3D(m_handle, 0, 0, 0, (GLint)i, width, height, 1, format, GL_UNSIGNED_BYTE, data[i]);
	}

	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

#pragma endregion

//==============================================================================
// Renderer
//==============================================================================
#pragma region Renderer
#pragma endregion

//==============================================================================
// RenderWorld
//==============================================================================
#pragma region RenderWorld

inline constexpr std::vector<AttribFormat> GetMeshVertexFormat()
{
	return
	{
		CreateAttribFormat<glm::vec3>(0, offsetof(MeshVertex, position)),
		CreateAttribFormat<glm::vec3>(1, offsetof(MeshVertex, color)),
		CreateAttribFormat<glm::vec3>(2, offsetof(MeshVertex, normal)),
		CreateAttribFormat<glm::vec2>(3, offsetof(MeshVertex, texCoords)),
		CreateAttribFormat<glm::vec3>(4, offsetof(MeshVertex, tangent)),
	};
}

#pragma endregion