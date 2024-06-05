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
	Fatal("unsupported type");
}

template<typename T>
inline constexpr AttribFormat CreateAttribFormat(GLuint attribIndex, GLuint relativeOffset)
{
	const auto [compCount, type] = TypeToSizeEnum<T>();
	return { attribIndex, compCount, type, relativeOffset };
}

#pragma endregion
//==============================================================================
// END Render Core
//==============================================================================

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
	if (m_vertexShader) m_vertexShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetGeometryUniform(GLint location, const T& value)
{
	if (m_geometryShader) m_geometryShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetFragmentUniform(GLint location, const T& value)
{
	if (m_fragmentShader) m_fragmentShader->SetUniform<T>(location, value);
}
template <typename T>
inline void GLProgramPipeline::SetComputeUniform(GLint location, const T& value)
{
	if (m_computeShader) m_computeShader->SetUniform<T>(location, value);
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
		std::make_shared<GLBuffer>(indices), IndexFormat::UInt8,
		attribFormats)
{
}

template<typename T>
inline GLVertexArray::GLVertexArray(const std::vector<T>& vertices, const std::vector<uint16_t>& indices, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(
		std::make_shared<GLBuffer>(vertices), sizeof(T),
		std::make_shared<GLBuffer>(indices), IndexFormat::UInt16,
		attribFormats)
{
}

template<typename T>
inline GLVertexArray::GLVertexArray(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(
		std::make_shared<GLBuffer>(vertices), sizeof(T),
		std::make_shared<GLBuffer>(indices), IndexFormat::UInt32,
		attribFormats)
{
}

#pragma endregion
//==============================================================================
// END Render Resources
//==============================================================================