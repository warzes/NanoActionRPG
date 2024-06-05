#include "NanoEngine.h"

//==============================================================================
// GPU Config
//==============================================================================
#pragma region Gpu Config
// Use discrete GPU by default.
extern "C"
{
	// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

	// https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
	__declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#pragma endregion
//==============================================================================
// GPU Config
//==============================================================================

//==============================================================================
// GLOBAL VARS
//==============================================================================
#pragma region Global Vars
namespace 
{
	struct
	{
		bool IsEnd = false;
	} Engine;
}

void ResetGlobalVars()
{
	Engine.IsEnd = false;
}
#pragma endregion
//==============================================================================
// END GLOBAL VARS
//==============================================================================

//==============================================================================
// LOG
//==============================================================================
#pragma region Log
void Print(const std::string& text)
{
	puts(text.c_str());
}
void Warning(const std::string& text)
{
	Print("WARNING: " + text);
}
void Error(const std::string& text)
{
	Print("ERROR: " + text);
}
void Fatal(const std::string& text)
{
	Print("FATAL: " + text);
	Engine.IsEnd = true;
}
#pragma endregion
//==============================================================================
// END LOG
//==============================================================================

//==============================================================================
// Render Resources
//==============================================================================
#pragma region Render Resources

#pragma region GLSeparableShaderProgram

GLSeparableShaderProgram::GLSeparableShaderProgram(GLenum shaderType, std::string_view sourceCode)
{
	createHandle(shaderType, sourceCode);
}

GLSeparableShaderProgram::~GLSeparableShaderProgram()
{
	destroyHandle();
}

void GLSeparableShaderProgram::createHandle(GLenum shaderType, std::string_view sourceCode)
{
	const auto srcPtr = sourceCode.data();
	m_handle = glCreateShaderProgramv(shaderType, 1, &srcPtr);
	glProgramParameteri(m_handle, GL_PROGRAM_SEPARABLE, GL_TRUE);
	validate(sourceCode);
}

void GLSeparableShaderProgram::destroyHandle()
{
	if (m_handle != 0)
		glDeleteProgram(m_handle);
	m_handle = 0;
}

void GLSeparableShaderProgram::validate(std::string_view sourceCode)
{
	GLint compiled = 0;
	glGetProgramiv(m_handle, GL_LINK_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		std::array<char, 1024> compiler_log;
		glGetProgramInfoLog(m_handle, (GLsizei)compiler_log.size(), nullptr, compiler_log.data());
		glDeleteShader(m_handle);

		std::ostringstream message;
		message << "shader contains error(s):\n\n" << sourceCode.data() << "\n\n" << compiler_log.data() << '\n';
		Error(message.str());
		glDeleteProgram(m_handle);
		m_handle = 0;
	}
}

#pragma endregion

#pragma region GLProgramPipeline

GLProgramPipeline::GLProgramPipeline(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef fragmentShader)
{
	if (!::IsValid(vertexShader))
	{
		Error("vertexShader is null");
		return;
	}
	if (!::IsValid(fragmentShader))
	{
		Error("fragmentShader is null");
		return;
	}

	createHandle();
	setSeparableShaders(vertexShader, nullptr, fragmentShader, nullptr);
}

GLProgramPipeline::GLProgramPipeline(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef geometryShader, GLSeparableShaderProgramRef fragmentShader)
{
	if (!::IsValid(vertexShader))
	{
		Error("vertexShader is null");
		return;
	}
	if (!::IsValid(geometryShader))
	{
		Error("geometryShader is null");
		return;
	}
	if (!::IsValid(fragmentShader))
	{
		Error("fragmentShader is null");
		return;
	}

	createHandle();
	setSeparableShaders(vertexShader, geometryShader, fragmentShader, nullptr);
}

GLProgramPipeline::GLProgramPipeline(GLSeparableShaderProgramRef computeShader)
{
	if (!::IsValid(computeShader))
	{
		Error("computeShader is null");
		return;
	}

	createHandle();
	setSeparableShaders(nullptr, nullptr, nullptr, computeShader);
}

GLProgramPipeline::GLProgramPipeline(std::string_view vertexShaderCode, std::string_view fragmentShaderCode) 
	: GLProgramPipeline(
		std::make_shared<GLSeparableShaderProgram>((GLenum)GL_VERTEX_SHADER, vertexShaderCode),
		std::make_shared<GLSeparableShaderProgram>((GLenum)GL_FRAGMENT_SHADER, fragmentShaderCode))
{
}

GLProgramPipeline::GLProgramPipeline(std::string_view vertexShaderCode, std::string_view geometryShaderCode, std::string_view fragmentShaderCode)
	: GLProgramPipeline(
		std::make_shared<GLSeparableShaderProgram>((GLenum)GL_VERTEX_SHADER, vertexShaderCode),
		std::make_shared<GLSeparableShaderProgram>((GLenum)GL_GEOMETRY_SHADER, geometryShaderCode),
		std::make_shared<GLSeparableShaderProgram>((GLenum)GL_FRAGMENT_SHADER, fragmentShaderCode))
{
}

GLProgramPipeline::GLProgramPipeline(std::string_view computeShaderCode)
	: GLProgramPipeline(std::make_shared<GLSeparableShaderProgram>((GLenum)GL_COMPUTE_SHADER, computeShaderCode))
{
}

GLProgramPipeline::~GLProgramPipeline()
{
	destroyHandle();
}

void GLProgramPipeline::createHandle()
{
	glCreateProgramPipelines(1, &m_handle);
}

void GLProgramPipeline::destroyHandle()
{
	if (m_handle != 0)
		glDeleteProgramPipelines(1, &m_handle);
	m_handle = 0;

	m_vertexShader.reset();
	m_geometryShader.reset();
	m_fragmentShader.reset();
	m_computeShader.reset();
}

void GLProgramPipeline::setSeparableShaders(GLSeparableShaderProgramRef vertexShader, GLSeparableShaderProgramRef geometryShader, GLSeparableShaderProgramRef fragmentShader, GLSeparableShaderProgramRef computeShader)
{
	if (vertexShader)
	{
		glUseProgramStages(m_handle, GL_VERTEX_SHADER_BIT, *vertexShader);
		m_vertexShader = vertexShader;
	}
	if (geometryShader)
	{
		glUseProgramStages(m_handle, GL_GEOMETRY_SHADER_BIT, *geometryShader);
		m_geometryShader = geometryShader;
	}
	if (fragmentShader)
	{
		glUseProgramStages(m_handle, GL_FRAGMENT_SHADER_BIT, *fragmentShader);
		m_fragmentShader = fragmentShader;
	}
	if (computeShader)
	{
		glUseProgramStages(m_handle, GL_COMPUTE_SHADER_BIT, *computeShader);
		m_computeShader = computeShader;
	}
}

#pragma endregion

#pragma region GLBuffer

GLBuffer::~GLBuffer()
{
	destroyHandle();
}

void* GLBuffer::MapRange(GLintptr offset, GLsizeiptr size, GLbitfield accessFlags)
{
	assert(m_handle);
	return glMapNamedBufferRange(m_handle, offset, size, accessFlags);
}

void* GLBuffer::Map(GLbitfield access)
{
	assert(m_handle);
	return glMapNamedBuffer(m_handle, access);
}

void GLBuffer::FlushMappedRange(GLintptr offset, GLsizeiptr size)
{
	assert(m_handle);
	glFlushMappedNamedBufferRange(m_handle, offset, size);
}

void GLBuffer::Unmap()
{
	assert(m_handle);
	glUnmapNamedBuffer(m_handle);
}

void GLBuffer::createHandle()
{
	glCreateBuffers(1, &m_handle);
}

void GLBuffer::destroyHandle()
{
	if (m_handle != 0)
		glDeleteBuffers(1, &m_handle);
	m_handle = 0;
}

#pragma endregion

#pragma region GLVertexArray

GLVertexArray::GLVertexArray(GLBufferRef vbo, size_t vertexSize, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(vbo, vertexSize, nullptr, {}, attribFormats)
{
}

GLVertexArray::GLVertexArray(GLBufferRef vbo, size_t vertexSize, GLBufferRef ibo, IndexFormat indexFormat, const std::vector<AttribFormat>& attribFormats)
{
	createHandle();
	setAttribFormats(attribFormats);

	if (::IsValid(vbo) && vertexSize) setVertexBuffer(vbo, vertexSize);
	if (::IsValid(ibo)) setIndexBuffer(ibo, indexFormat);
}

GLVertexArray::~GLVertexArray()
{
	destroyHandle();
}

void GLVertexArray::createHandle()
{
	glCreateVertexArrays(1, &m_handle);
}

void GLVertexArray::destroyHandle()
{
	if (m_handle != 0)
		glDeleteVertexArrays(1, &m_handle);
	m_handle = 0;
}

void GLVertexArray::setAttribFormats(const std::vector<AttribFormat>& attribFormats)
{
	for (const auto& format : attribFormats)
	{
		glEnableVertexArrayAttrib(m_handle, format.attribIndex);
		glVertexArrayAttribFormat(m_handle, format.attribIndex, format.size, format.type, GL_FALSE, format.relativeOffset);
		glVertexArrayAttribBinding(m_handle, format.attribIndex, 0);
	}
}

void GLVertexArray::setVertexBuffer(GLBufferRef vbo, size_t vertexSize)
{
	setVertexBuffer(0, vbo, 0, vertexSize);
}

void GLVertexArray::setVertexBuffer(GLuint bindingIndex, GLBufferRef vbo, GLintptr offset, size_t stride)
{
	glVertexArrayVertexBuffer(m_handle, bindingIndex, *vbo, offset, (GLsizei)stride);
	m_vertexSize = stride; // TODO; это правильно?
	m_vbo = vbo;
}

void GLVertexArray::setIndexBuffer(GLBufferRef ibo, IndexFormat indexFormat)
{
	glVertexArrayElementBuffer(m_handle, *ibo);
	m_ibo = ibo;
	m_indexFormat = indexFormat;
}

#pragma endregion

#pragma endregion
//==============================================================================
// END Render Resources
//==============================================================================