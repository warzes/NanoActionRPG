#include "NanoEngine.h"

//==============================================================================
// Lib
//==============================================================================
#pragma region Lib
#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#	pragma comment( lib, "assimp-vc143-mt.lib" )
#endif
#pragma endregion

//==============================================================================
// GPU Config
//==============================================================================
#pragma region GPU Config
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
// GLOBAL VARS
//==============================================================================
#pragma region Global Vars
namespace 
{
	struct
	{
		GLFWwindow* window = nullptr;
		int windowWidth = 0;
		int windowHeight = 0;
		bool IsResize = false;
		bool IsEnd = false;
	} Engine;

	struct
	{
		glm::ivec2 position;
		glm::ivec2 lastPosition;
		glm::ivec2 delta;
	} MouseState;

	struct
	{
		ImFont* defaultFont = nullptr;
	} imgui;
}

void ResetGlobalVars()
{
	Engine.IsEnd = false;

}
#pragma endregion

//==============================================================================
// CORE
//==============================================================================
#pragma region Core

#pragma region Clock

Time Clock::GetElapsedTime() const
{
	if (IsRunning())
		return std::chrono::duration_cast<std::chrono::microseconds>(ClockImpl::now() - m_refPoint);
	return std::chrono::duration_cast<std::chrono::microseconds>(m_stopPoint - m_refPoint);
}

bool Clock::IsRunning() const
{
	return m_stopPoint == ClockImpl::time_point();
}

void Clock::Start()
{
	if (!IsRunning())
	{
		m_refPoint += ClockImpl::now() - m_stopPoint;
		m_stopPoint = {};
	}
}

void Clock::Stop()
{
	if (IsRunning())
		m_stopPoint = ClockImpl::now();
}

Time Clock::Restart()
{
	const Time elapsed = GetElapsedTime();
	m_refPoint = ClockImpl::now();
	m_stopPoint = {};
	return elapsed;
}

inline Time Clock::Reset()
{
	const Time elapsed = GetElapsedTime();
	m_refPoint = ClockImpl::now();
	m_stopPoint = m_refPoint;
	return elapsed;
}

#pragma endregion

#pragma endregion

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
// Math
//==============================================================================
#pragma region Math

#pragma endregion

//==============================================================================
// Render Core
//==============================================================================
#pragma region Render Core

const std::pair<GLenum, GLenum> STBImageToOpenGLFormat(int comp)
{
	switch (comp)
	{
	case STBI_rgb_alpha:  return std::make_pair<GLenum, GLenum>(GL_RGBA8, GL_RGBA);
	case STBI_rgb:        return std::make_pair<GLenum, GLenum>(GL_RGB8, GL_RGB);
	case STBI_grey:       return std::make_pair<GLenum, GLenum>(GL_R8, GL_RED);
	case STBI_grey_alpha: return std::make_pair<GLenum, GLenum>(GL_RG8, GL_RG);
	default: Fatal("invalid format"); return std::make_pair<GLenum, GLenum>(GL_RGBA8, GL_RGBA);
	}
}

#pragma endregion

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

void GLProgramPipeline::Bind()
{
	glBindProgramPipeline(m_handle);
}

GLint GLProgramPipeline::GetVertexUniform(const std::string& name) const
{
	if (!m_vertexShader) return -1;
	return glGetUniformLocation(*m_vertexShader, name.c_str());
}

GLint GLProgramPipeline::GetGeometryUniform(const std::string& name) const
{
	if (!m_geometryShader) return -1;
	return glGetUniformLocation(*m_geometryShader, name.c_str());
}

GLint GLProgramPipeline::GetFragmentUniform(const std::string& name) const
{
	if (!m_fragmentShader) return -1;
	return glGetUniformLocation(*m_fragmentShader, name.c_str());
}

GLint GLProgramPipeline::GetComputeUniform(const std::string& name) const
{
	if (!m_computeShader) return -1;
	return glGetUniformLocation(*m_computeShader, name.c_str());
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

GLVertexArray::GLVertexArray()
{
	createHandle();
}

GLVertexArray::GLVertexArray(GLBufferRef vbo, const std::vector<AttribFormat>& attribFormats)
	: GLVertexArray(vbo, nullptr, attribFormats)
{
}

GLVertexArray::GLVertexArray(GLBufferRef vbo, GLBufferRef ibo, const std::vector<AttribFormat>& attribFormats)
{
	createHandle();
	setAttribFormats(attribFormats);

	if (::IsValid(vbo)) setVertexBuffer(vbo);
	if (::IsValid(ibo)) setIndexBuffer(ibo);
}

GLVertexArray::~GLVertexArray()
{
	destroyHandle();
}

void GLVertexArray::Bind()
{
	glBindVertexArray(m_handle);
}

void GLVertexArray::DrawTriangles()
{
	Bind();

	if (!m_ibo)
	{
		glDrawArrays(GL_TRIANGLES, 0, m_vbo->GetElementCount());
	}
	else
	{
		const GLenum type = (m_ibo->GetElementSize() == sizeof(uint8_t) ? GL_UNSIGNED_BYTE
			: (m_ibo->GetElementSize() == sizeof(uint16_t) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT));
		glDrawElements(GL_TRIANGLES, m_ibo->GetElementCount(), type, nullptr);
	}
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

void GLVertexArray::setVertexBuffer(GLBufferRef vbo)
{
	setVertexBuffer(0, vbo, 0, vbo->GetElementSize());
}

void GLVertexArray::setVertexBuffer(GLuint bindingIndex, GLBufferRef vbo, GLintptr offset, size_t stride)
{
	glVertexArrayVertexBuffer(m_handle, bindingIndex, *vbo, offset, (GLsizei)stride);
	m_vbo = vbo;
}

void GLVertexArray::setIndexBuffer(GLBufferRef ibo)
{
	glVertexArrayElementBuffer(m_handle, *ibo);
	m_ibo = ibo;
}

#pragma endregion

#pragma region GLTexture2D

GLTexture2D::GLTexture2D(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, bool generateMipMaps)
	: GLTexture2D(internalFormat, format, GL_UNSIGNED_BYTE, width, height, data, filter, repeat, glm::vec4(0.0f), generateMipMaps)
{
}

GLTexture2D::GLTexture2D(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, const glm::vec4& borderColor, bool generateMipMaps)
{
	createHandle();
	createTexture(internalFormat, format, dataType, width, height, data, filter, repeat, borderColor, generateMipMaps);
}

GLTexture2D::GLTexture2D(std::string_view filepath, int comp, bool generateMipMaps)
{
	if (!std::filesystem::exists(filepath))
	{
		Error("File '" + std::string(filepath.data()) + "' does not exist.");
		return;
	}

	int w, h, c;
	auto data = stbi_load(filepath.data(), &w, &h, &c, comp);
	if (!data)
	{
		Error("File '" + std::string(filepath.data()) + "' does not load.");
		return;
	}

	const auto [internalFormat, format] = STBImageToOpenGLFormat((comp));

	createHandle();
	createTexture(internalFormat, format, GL_UNSIGNED_BYTE, w, h, data, GL_LINEAR, GL_REPEAT, glm::vec4(0.0f), generateMipMaps);
	stbi_image_free(data);
}

GLTexture2D::~GLTexture2D()
{
	destroyHandle();
}

void GLTexture2D::Bind(GLuint slot)
{
	glBindTextureUnit(slot, m_handle);
}

void GLTexture2D::BindImage(uint32_t index, uint32_t level, bool write, std::optional<int> layer)
{
	glBindImageTexture(
		index,
		m_handle,
		level,
		(bool)layer,
		layer.value_or(0),
		write ? GL_READ_WRITE : GL_READ_ONLY,
		m_internalFormat
	);
}

void GLTexture2D::createHandle()
{
	glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
}

void GLTexture2D::destroyHandle()
{
	if (m_handle != 0)
		glDeleteTextures(1, &m_handle);
	m_handle = 0;
}

void GLTexture2D::createTexture(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, const glm::vec4& borderColor, bool generateMipMaps)
{
	m_internalFormat = internalFormat;

	const int maxAnisotropy = 16;
	int levels = 1;
	if (generateMipMaps)
		levels = NumMipmap(width, height);

	GLint minFilter = filter;
	if (generateMipMaps)
	{
		if (filter == GL_LINEAR) minFilter = GL_LINEAR_MIPMAP_LINEAR;
		else if (filter == GL_NEAREST) minFilter = GL_NEAREST_MIPMAP_NEAREST;
		else Fatal("Unsupported filter");
	}

	glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, filter);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, repeat);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, repeat);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, repeat);
	glTextureParameterfv(m_handle, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(borderColor));
	glTextureParameteri(m_handle, GL_TEXTURE_MAX_LEVEL, levels - 1);
	glTextureParameteri(m_handle, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

	glTextureStorage2D(m_handle, levels, internalFormat, width, height);

	if (data)
		glTextureSubImage2D(m_handle, 0, 0, 0, width, height, format, dataType, data);

	if (generateMipMaps)
		glGenerateTextureMipmap(m_handle);
}

#pragma endregion

#pragma region GLTextureCube

GLTextureCube::GLTextureCube(const std::array<std::string_view, 6>& filepath, int comp)
{
	int x, y, c;
	std::array<stbi_uc*, 6> faces;

	const auto [internalFormat, format] = STBImageToOpenGLFormat(comp);
	for (size_t i = 0; i < 6; i++)
		faces[i] = stbi_load(filepath[i].data(), &x, &y, &c, comp);

	createHandle();
	createTexture(internalFormat, format, x, y, faces);
	for (size_t i = 0; i < 6; i++)
		stbi_image_free(faces[i]);
}

GLTextureCube::~GLTextureCube()
{
	destroyHandle();
}

void GLTextureCube::Bind(GLuint slot)
{
	glBindTextureUnit(slot, m_handle);
}

void GLTextureCube::createHandle()
{
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_handle);
}

void GLTextureCube::destroyHandle()
{
	if (m_handle != 0)
		glDeleteTextures(1, &m_handle);
	m_handle = 0;
}

#pragma endregion

#pragma region GLFramebuffer

GLFramebuffer::GLFramebuffer(const std::vector<GLTexture2DRef>& colors, GLTexture2DRef depth)
{
	createHandle();
	setTextures(colors, depth);
}

GLFramebuffer::~GLFramebuffer()
{
	destroyHandle();
}

void GLFramebuffer::ClearFramebuffer(GLenum buffer, GLint drawbuffer, const GLfloat* value)
{
	glClearNamedFramebufferfv(m_handle, buffer, drawbuffer, value);
}

void GLFramebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void GLFramebuffer::createHandle()
{
	glCreateFramebuffers(1, &m_handle);
}

void GLFramebuffer::destroyHandle()
{
	if (m_handle != 0)
		glDeleteFramebuffers(1, &m_handle);
	m_handle = 0;
	m_colorTextures.clear();
	m_depthTexture.reset();
}

void GLFramebuffer::setTextures(const std::vector<GLTexture2DRef>& colors, GLTexture2DRef depth)
{
	for (size_t i = 0; i < colors.size(); i++)
		glNamedFramebufferTexture(m_handle, GL_COLOR_ATTACHMENT0 + (GLenum)i, *colors[i], 0);

	std::array<GLenum, 32> drawBuffs{};
	for (size_t i = 0; i < colors.size(); i++)
		drawBuffs[i] = GL_COLOR_ATTACHMENT0 + (GLenum)i;

	glNamedFramebufferDrawBuffers(m_handle, (GLsizei)colors.size(), drawBuffs.data());

	if (depth)
		glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, *depth, 0);

	if (glCheckNamedFramebufferStatus(m_handle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		Fatal("incomplete framebuffer");
}

#pragma endregion

#pragma endregion

//==============================================================================
// Renderer
//==============================================================================
#pragma region Renderer

#if defined(_DEBUG)
void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*user_param*/) noexcept
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::string msgSource;
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             msgSource = "GL_DEBUG_SOURCE_API";             break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: msgSource = "GL_DEBUG_SOURCE_SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     msgSource = "GL_DEBUG_SOURCE_THIRD_PARTY";     break;
	case GL_DEBUG_SOURCE_APPLICATION:     msgSource = "GL_DEBUG_SOURCE_APPLICATION";     break;
	case GL_DEBUG_SOURCE_OTHER:           msgSource = "GL_DEBUG_SOURCE_OTHER";           break;
	}

	std::string msgType;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               msgType = "GL_DEBUG_TYPE_ERROR";               break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msgType = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  msgType = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";  break;
	case GL_DEBUG_TYPE_PORTABILITY:         msgType = "GL_DEBUG_TYPE_PORTABILITY";         break;
	case GL_DEBUG_TYPE_PERFORMANCE:         msgType = "GL_DEBUG_TYPE_PERFORMANCE";         break;
	case GL_DEBUG_TYPE_OTHER:               msgType = "GL_DEBUG_TYPE_OTHER";               break;
	}

	std::string msgSeverity = "DEFAULT";
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:    msgSeverity = "GL_DEBUG_SEVERITY_LOW";    break;
	case GL_DEBUG_SEVERITY_MEDIUM: msgSeverity = "GL_DEBUG_SEVERITY_MEDIUM"; break;
	case GL_DEBUG_SEVERITY_HIGH:   msgSeverity = "GL_DEBUG_SEVERITY_HIGH";   break;
	}

	std::string logMsg = "glDebugMessage: " + std::string(message) + ", type = " + msgType + ", source = " + msgSource + ", severity = " + msgSeverity;

	if (type == GL_DEBUG_TYPE_ERROR) Warning(logMsg);
	else                             Error(logMsg);
}
#endif

bool Renderer::Init()
{
#if defined(_DEBUG)
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

	Print("OpenGL: OpenGL device information:");
	Print("    > Vendor:   " + std::string((const char*)glGetString(GL_VENDOR)));
	Print("    > Renderer: " + std::string((const char*)glGetString(GL_RENDERER)));
	Print("    > Version:  " + std::string((const char*)glGetString(GL_VERSION)));
	Print("    > GLSL:     " + std::string((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)));

	return true;
}

void Renderer::Close()
{
}

void Renderer::MainFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BlitFrameBuffer(
	GLFramebufferRef readFramebuffer, GLFramebufferRef drawFramebuffer, 
	GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, 
	GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, 
	GLbitfield mask, GLenum filter)
{
	GLuint readFramebufferId = 0;
	GLuint drawFramebufferId = 0;
	if (readFramebuffer) readFramebufferId = *readFramebuffer;
	if (drawFramebuffer) drawFramebufferId = *drawFramebuffer;

	glBlitNamedFramebuffer(
		readFramebufferId,
		drawFramebufferId,
		srcX0, srcY0, srcX1, srcY1,
		dstX0, dstY0, dstX1, dstY1,
		mask, filter
	);
}

void Renderer::Clear(bool color, bool depth, bool stencil)
{
	GLbitfield mask = 0;
	if (color) mask |= GL_COLOR_BUFFER_BIT;
	if (depth) mask |= GL_DEPTH_BUFFER_BIT;
	if (stencil) mask |= GL_STENCIL_BUFFER_BIT;
	glClear(mask);
}

void Renderer::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

void Renderer::SetScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glScissor(x, y, width, height);
}

#pragma endregion

//==============================================================================
// Graphics
//==============================================================================
#pragma region Graphics

constexpr const char* UniformDiffuseColorName = "uDiffuseColor";
constexpr const char* UniformAmbientColorName = "uAmbientColor";
constexpr const char* UniformSpecularColorName = "uSpecularColor";
constexpr const char* UniformShininessName = "uShininess";
constexpr const char* UniformRefractiName = "uRefracti";

#pragma region Node

void Node::SetParent(Node* parent)
{
	if (m_parent == parent) m_parent = nullptr;
	else m_parent = parent;
}

void Node::SetTransform(const Transform&)
{
}

void Node::SetSize(const glm::vec3&)
{
}

void Node::AddChild(Node* child)
{
	auto it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end())
		m_children.erase(it);
	else
		m_children.push_back(child);
}

void Node::Draw()
{
	for (auto i : m_children)
		i->Draw();
}

Node* Node::GetParent()
{
	return m_parent;
}

std::vector<Node*> Node::GetChildren()
{
	return m_children;
}

glm::vec3 Node::GetSize()
{
	return glm::vec3(1.0f);
}

Transform Node::GetFinalTransform(Node* node, Transform tr)
{
	if (node->GetParent())
		tr = Node::GetFinalTransform(node->GetParent(), node->GetParent()->GetTransform()) * tr;
	return tr;
}

#pragma endregion

#pragma region MeshVertex

void MeshVertex::AddBoneData(uint32_t boneID, float weight)
{
	for (uint32_t i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++)
	{
		if (weights[i] == 0.0f)
		{
			boneIDs[i] = boneID;
			weights[i] = weight;
			return;
		}
	}
	Warning("Vertex has more than " + std::to_string(MAX_NUM_BONES_PER_VERTEX) + " bones!");
}

#pragma endregion

#pragma region Mesh

Mesh::Mesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<MaterialTexture>& textures, const MaterialProperties& materialProperties)
	: m_vertices(vertices)
	, m_indices(indices)
	, m_textures(textures)
	, m_materialProp(materialProperties)
{
	init();
}

AABB Mesh::GetBounding() const
{
	return m_bounding;
}

std::vector<glm::vec3> Mesh::GetTriangle() const
{
	std::vector<glm::vec3> triangles;
	for (size_t i = 0; i < m_vertices.size(); i++)
		triangles.push_back(m_vertices[i].position);
	return triangles;
}

GLVertexArrayRef Mesh::GetVAO()
{
	return m_vao;
}

void Mesh::Draw(const GLProgramPipelineRef& program)
{
	assert(::IsValid(program));
	assert(::IsValid(m_vao));
	for (size_t i = 0; i < m_textures.size(); i++)
	{
		if (m_textures[i].texture)
			m_textures[i].texture->Bind(i);
	}

	{
		if (m_diffuseColorLoc == -1) m_diffuseColorLoc = glGetUniformLocation(*program->GetFragmentShader(), UniformDiffuseColorName);
		if (m_ambientColorLoc == -1) m_ambientColorLoc = glGetUniformLocation(*program->GetFragmentShader(), UniformAmbientColorName);
		if (m_specularColorLoc == -1) m_specularColorLoc = glGetUniformLocation(*program->GetFragmentShader(), UniformSpecularColorName);
		if (m_shininessLoc == -1) m_shininessLoc = glGetUniformLocation(*program->GetFragmentShader(), UniformShininessName);
		if (m_refractiLoc == -1) m_refractiLoc = glGetUniformLocation(*program->GetFragmentShader(), UniformRefractiName);
	}

	program->SetFragmentUniform(m_diffuseColorLoc, m_materialProp.diffuseColor);
	program->SetFragmentUniform(m_ambientColorLoc, m_materialProp.ambientColor);
	program->SetFragmentUniform(m_specularColorLoc, m_materialProp.specularColor);
	program->SetFragmentUniform(m_shininessLoc, m_materialProp.shininess);
	program->SetFragmentUniform(m_refractiLoc, m_materialProp.refracti);

	m_vao->DrawTriangles();
}

void Mesh::init()
{
	std::vector<glm::vec3> points;
	for (size_t i = 0; i < m_vertices.size(); i++)
		points.push_back(m_vertices[i].position);
	m_bounding = AABB(points);

	m_vao = std::make_shared<GLVertexArray>(m_vertices, m_indices, GetMeshVertexFormat());
}

#pragma endregion

#pragma region Animation

std::pair<int, float> TimeFraction(std::vector<float>& times, float dt)
{
	if (times.empty()) return { 1, 1.0 };

	int seg = 0;
	while (dt > times[seg]) seg++;
	float frac = (dt - times[seg - 1]) / (times[seg] - times[seg - 1]);

	return { seg, frac };
}

Animation::Animation(const std::string& name)
	: m_name(name)
{
}

void Animation::SetName(const std::string& name)
{
	m_name = name;
}

void Animation::SetTPS(float tps)
{
	m_tps = tps;
}

void Animation::SetDuration(float duration)
{
	m_duration = duration;
}

void Animation::SetIsRepeated(bool repeat)
{
	m_repeat = repeat;
}

void Animation::SetIsBlending(bool blending)
{
	m_blending = blending;
}

void Animation::SetLastTime(float lastTime)
{
	m_lastTime = lastTime;
}

void Animation::AddKeyframe(const std::string& name, const Keyframe& keyframe)
{
	m_keyframes[name] = keyframe;
}

void Animation::Play(float time)
{
	if (m_state != State::Paused)
	{
		m_lastTime = time;
		if (time < 0)
			m_blending = true;
	}

	m_state = State::Playing;
	m_time.Restart();
}

void Animation::Pause()
{
	m_state = State::Paused;
	m_lastTime = GetTime() + m_lastTime;
}

void Animation::Stop()
{
	m_state = State::Stopped;
	m_lastTime = 0.01;
}

std::unordered_map<std::string, std::pair<Transform, glm::vec3>> Animation::Update()
{
	std::unordered_map<std::string, std::pair<Transform, glm::vec3>> actions;

	if (m_state != State::Stopped)
		for (auto& [name, keyframe] : m_keyframes)
		{
			float time = 0;
			if (m_state == State::Playing)
			{
				time = GetTime();
				if (m_lastTime != 0)
					time += m_lastTime;
			}
			else time = m_lastTime;

			if (m_state == State::Playing && time >= m_duration)
			{
				if (m_repeat)
				{
					time = m_time.Restart().AsSeconds() * m_tps;
					m_lastTime = 0;
				}
				else
				{
					time = m_duration;
					m_state = State::Paused;
					m_lastTime = m_duration;
				}
			}

			float dt = fmod(time == 0 ? time + 0.01 : (time == m_duration ? time - 0.01 : time), m_duration);

			auto posFraction = TimeFraction(keyframe.posStamps, dt);
			auto rotFraction = TimeFraction(keyframe.rotStamps, dt);
			auto scaleFraction = TimeFraction(keyframe.scaleStamps, dt);

			glm::vec3 pos = glm::mix(keyframe.positions[posFraction.first - 1], keyframe.positions[posFraction.first], posFraction.second);
			glm::quat rot = glm::slerp(keyframe.rotations[rotFraction.first - 1], keyframe.rotations[rotFraction.first], rotFraction.second);
			glm::vec3 scale = glm::mix(keyframe.scales[scaleFraction.first - 1], keyframe.scales[scaleFraction.first], scaleFraction.second);

			actions[name] = { { { pos.x, pos.y, pos.z }, { rot.x, rot.y, rot.z, rot.w } }, { scale.x, scale.y, scale.z } };
		}

	return actions;
}

std::unordered_map<std::string, Keyframe>& Animation::GetKeyframes()
{
	return m_keyframes;
}

std::string Animation::GetName() const
{
	return m_name;
}

bool Animation::IsRepeated() const
{
	return m_repeat;
}

bool Animation::IsBlending() const
{
	return m_blending;
}

float Animation::GetTime() const
{
	return m_time.GetElapsedTime().AsSeconds() * m_tps;
}

float Animation::GetLastTime() const
{
	return m_lastTime;
}

float Animation::GetDuration() const
{
	return m_duration;
}

float Animation::GetTPS() const
{
	return m_tps;
}

Animation::State Animation::GetState() const
{
	return m_state;
}

#pragma endregion

#pragma region Bone

Bone::Bone(int id, const std::string& name, const glm::mat4& offset) : m_id(id), m_name(name)
{
	m_offset = offset;
	m_size = glm::vec3(1.0f);
}

void Bone::SetTransform(const Transform& tr)
{
	m_transform = tr;
}

void Bone::SetPosition(const glm::vec3& pos)
{
	SavePoseAsIdle();
	m_transform.SetPosition(pos);
}

void Bone::SetOrientation(const glm::quat& orient)
{
	SavePoseAsIdle();
	m_transform.SetOrientation(orient);
}

void Bone::SetSize(const glm::vec3& size)
{
	m_size = size;
}

void Bone::Move(const glm::vec3& vec)
{
	SavePoseAsIdle();
	m_transform.SetPosition(m_transform.GetPosition() + vec);
}

void Bone::Rotate(const glm::quat& quat)
{
	SavePoseAsIdle();
	m_transform.SetOrientation(quat * m_transform.GetOrientation());
}

void Bone::Expand(const glm::vec3& vec)
{
	m_size += vec;
}

void Bone::SavePoseAsIdle()
{
	m_idle = m_transform;
}

int Bone::GetID()
{
	return m_id;
}

std::string Bone::GetName()
{
	return m_name;
}

glm::vec3 Bone::GetPosition()
{
	return m_transform.GetPosition();
}

glm::quat Bone::GetOrientation()
{
	return m_transform.GetOrientation();
}

glm::vec3 Bone::GetSize()
{
	return m_size;
}

glm::mat4 Bone::GetOffset()
{
	return m_offset;
}

Transform Bone::GetTransform()
{
	return m_transform;
}

Transform Bone::GetIdle()
{
	return m_idle;
}

#pragma endregion

#pragma region Model

Model::Model(const std::string& modelPath, bool flipUV)
{
	std::string extension = strrchr(modelPath.c_str(), '.'); // TODO: найти лучшее решение
	//if (extension == "obj")
	
	loadAssimpModel(modelPath, flipUV);
}

void Model::Draw(const GLProgramPipelineRef& program)
{
	for (size_t i = 0; i < m_meshes.size(); i++)
		m_meshes[i]->Draw(program);
}

AABB Model::GetBounding() const
{
	return m_bounding;
}

std::vector<glm::vec3> Model::GetTriangle() const
{
	std::vector<glm::vec3> Triangle;
	for (size_t i = 0; i < m_meshes.size(); ++i)
	{
		std::vector<glm::vec3> temp = m_meshes[i]->GetTriangle();
		Triangle.insert(Triangle.end(), temp.begin(), temp.end());
	}
	return Triangle;
}

std::vector<AnimationRef> Model::GetAnimations() const
{
	return m_animations;
}

std::vector<BoneRef> Model::GetBones() const
{
	return m_bones;
}

std::vector<glm::mat4>& Model::GetPose()
{
	return m_pose;
}

MeshRef Model::operator[](size_t idx)
{
	assert(idx < m_meshes.size());
	return m_meshes[idx];
}

void Model::SetTransform(const Transform& transform)
{
	m_transform = transform;
}

void Model::SetPosition(const glm::vec3& position)
{
	m_transform.SetPosition(position);
}

void Model::SetOrientation(const glm::quat& orientation)
{
	m_transform.SetOrientation(orientation);
}

void Model::AddChild(Node* child)
{
	for (auto i : m_bones)
		if (i.get() == child)
		{
			auto it = std::find(m_children.begin(), m_children.end(), child);
			if (it != m_children.end())
			{
				(*it)->SetParent(nullptr);
				m_children.erase(it);
				return;
			}
			child->SetParent(this);
			m_children.push_back(child);
			return;
		}

	auto it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end())
	{
		m_children.erase(it);
	}
	else
	{
		m_children.push_back(child);
	}
}

Transform Model::GetTransform()
{
	return m_transform;
}

glm::vec3 Model::GetPosition()
{
	return m_transform.GetPosition();
}

glm::quat Model::GetOrientation()
{
	return m_transform.GetOrientation();
}

void Model::UpdateAnim()
{
	for (auto& i : m_bones)
		calculatePose(i.get());
}

void Model::DefaultPose()
{
	for (auto& i : m_bones)
		i->SetTransform(i->GetIdle());
	for (auto& i : m_bonesChildren)
		i->SetTransform(i->GetIdle());
}

void Model::loadAssimpModel(const std::string& modelPath, bool flipUV)
{
	unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
#if defined(GLM_FORCE_LEFT_HANDED)
	flags |= aiProcess_MakeLeftHanded;
#endif
	if (flipUV) flags |= aiProcess_FlipUVs;

	Assimp::Importer modelImporter;
	modelImporter.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 1);
	const aiScene* scene = modelImporter.ReadFile(modelPath, flags);
	if (!scene || !scene->mRootNode || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE)
	{
		Error("Load Model error: " + std::string(modelImporter.GetErrorString()));
		return;
	}
	m_directory = modelPath.substr(0, modelPath.find_last_of('/'));
	loadAnimations(scene);
	processNode(scene->mRootNode, scene, glm::mat4(1.0));
	findBoneNodes(scene->mRootNode, m_bones);
	computeAABB();

	m_pose.resize(64);

	Print("Model " + modelPath + " loaded:\n" +
		"        Meshes: " + std::to_string(m_meshes.size()) + '\n' +
		"        Bones: " + std::to_string(m_bones.size() + m_bonesChildren.size()) + '\n' +
		"        Animations: " + std::to_string(m_animations.size()));
}

constexpr glm::vec3 toglm(const aiVector3D& vec) { return glm::vec3(vec.x, vec.y, vec.z); }
constexpr glm::vec2 toglm(const aiVector2D& vec) { return glm::vec2(vec.x, vec.y); }
constexpr glm::quat toglm(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }

constexpr glm::mat4 toglm(const aiMatrix4x4& mat)
{
	glm::mat4 ret;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			ret[j][i] = mat[i][j];
	return ret;
}

std::pair<Transform, glm::vec3> To3DTransform(const glm::mat4& tr)
{
	Transform ret;
	glm::vec3 size;

	glm::vec3 pos, scale, skew;
	glm::vec4 perspective;
	glm::quat orient;

	glm::decompose(tr, scale, orient, pos, skew, perspective);
	orient = glm::conjugate(orient);

	size = { scale.x, scale.y, scale.z };

	ret.SetPosition({ pos.x, pos.y, pos.z });
	ret.SetOrientation({ orient.x, orient.y, orient.z, orient.w });

	return { ret, size };
}

void Model::loadAnimations(const aiScene* scene)
{
	for (int i = 0; i < scene->mNumAnimations; i++)
	{
		auto anim = scene->mAnimations[i];
		auto tmp = std::make_shared<Animation>(anim->mName.C_Str());
		float tps = anim->mTicksPerSecond;
		tmp->SetTPS(tps > 0 ? tps : 30.0);
		tmp->SetDuration(anim->mDuration);

		for (int j = 0; j < anim->mNumChannels; j++)
		{
			auto channel = anim->mChannels[j];
			Keyframe kf;
			for (int k = 0; k < channel->mNumPositionKeys; k++)
			{
				kf.posStamps.push_back(channel->mPositionKeys[k].mTime);
				kf.positions.push_back(toglm(channel->mPositionKeys[k].mValue));
			}
			for (int k = 0; k < channel->mNumRotationKeys; k++)
			{
				kf.rotStamps.push_back(channel->mRotationKeys[k].mTime);
				kf.rotations.push_back(toglm(channel->mRotationKeys[k].mValue));
			}
			for (int k = 0; k < channel->mNumScalingKeys; k++)
			{
				kf.scaleStamps.push_back(channel->mScalingKeys[k].mTime);
				kf.scales.push_back(toglm(channel->mScalingKeys[k].mValue));
			}
			tmp->AddKeyframe(channel->mNodeName.C_Str(), kf);
		}
		m_animations.push_back(tmp);
	}
}

void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
{
	glm::mat4 nodeTransform = MatrixCast(node->mTransformation);
	glm::mat4 totalTransform = parentTransform * nodeTransform;

	for (unsigned i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(processMesh(mesh, scene, totalTransform));
	}
	for (unsigned i = 0; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene, totalTransform);
	}
}

MeshRef Model::processMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
{
	_ASSERT(mesh);
	std::vector<MeshVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<MaterialTexture> textures;
	MaterialProperties matProperties;
	processVertex(mesh, transform, vertices);
	processIndices(mesh, indices);
	processBones(mesh, vertices);
	processTextures(mesh, scene, textures);
	processMatProperties(mesh, scene, matProperties);
	return std::make_shared<Mesh>(vertices, indices, textures, matProperties);
}

void Model::processVertex(const aiMesh* mesh, const glm::mat4& transform, std::vector<MeshVertex>& vertices)
{
	for (unsigned i = 0; i < mesh->mNumVertices; ++i)
	{
		auto aiVert = mesh->mVertices;
		auto aiClr = mesh->mColors[0];
		auto aiNormals = mesh->mNormals;
		auto aiTexCoords = mesh->mTextureCoords[0];
		auto aiTangents = mesh->mTangents;
		auto aiBitangents = mesh->mBitangents;

		MeshVertex vertex;

		vertex.position = transform * glm::vec4(aiVert[i].x, aiVert[i].y, aiVert[i].z, 1.0f);

		if (mesh->HasVertexColors(0)) 
			vertex.color = glm::vec3(aiClr[i].r, aiClr[i].g, aiClr[i].b);
		else 
			vertex.color = glm::vec3(1.0f);

		if (mesh->HasNormals())
			vertex.normal = transform * glm::vec4(aiNormals[i].x, aiNormals[i].y, aiNormals[i].z, 0);
		else
			vertex.normal = glm::vec3(0.0f);

		if (mesh->HasTextureCoords(0))
			vertex.texCoords = glm::vec2(aiTexCoords[i].x, aiTexCoords[i].y);
		else
			vertex.texCoords = glm::vec2(0.0f);

		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = glm::vec3(aiTangents[i].x, aiTangents[i].y, aiTangents[i].z);
			vertex.bitangent = glm::vec3(aiBitangents[i].x, aiBitangents[i].y, aiBitangents[i].z);
		}
		else
		{
			vertex.tangent = glm::vec3(0.0f);
			vertex.bitangent = glm::vec3(0.0f);
		}

		vertices.push_back(vertex);
	}
}

void Model::processIndices(const aiMesh* mesh, std::vector<uint32_t>& indices)
{
	for (unsigned i = 0; i < mesh->mNumFaces; ++i)
	{
		auto face = mesh->mFaces[i];
		for (unsigned k = 0; k < face.mNumIndices; ++k)
			indices.push_back(face.mIndices[k]);
	}
}

void Model::processBones(const aiMesh* mesh, std::vector<MeshVertex>& vertices)
{
	for (size_t i = 0; i < mesh->mNumBones; i++)
	{
		auto bone = mesh->mBones[i];

		if (m_bonemap.find(bone->mName.C_Str()) == m_bonemap.end())
			m_bonemap[bone->mName.C_Str()] = { i, toglm(bone->mOffsetMatrix) };

		std::vector<int> nbones;
		nbones.resize(mesh->mNumVertices, 0);
		for (size_t j = 0; j < bone->mNumWeights ? 1 : 0; j++)
		{
			int id = bone->mWeights[j].mVertexId;
			float weight = bone->mWeights[j].mWeight;
			vertices[id].boneIDs[nbones[id]] = i;
			vertices[id].weights[nbones[id]] = weight;
			nbones[id]++;
		}
	}
}

void Model::findBoneNodes(aiNode* node, std::vector<BoneRef>& bones)
{
	BoneRef tmp = nullptr;
	if (processBone(node, tmp))
	{
		if (tmp)
		{
			bones.emplace_back(tmp);
			AddChild(tmp.get());
		}
		return;
	}
	for (size_t i = 0; i < node->mNumChildren; i++)
		findBoneNodes(node->mChildren[i], bones);
}

bool Model::processBone(aiNode* node, BoneRef& out)
{
	if (m_bonemap.find(node->mName.C_Str()) != m_bonemap.end())
	{
		auto it = std::find_if(m_bones.begin(), m_bones.end(), [&](auto& b) { return b->GetName() == std::string(node->mName.C_Str()); });
		if (it != m_bones.end())
			return false;
		out = std::make_shared<Bone>(m_bonemap[node->mName.C_Str()].first, node->mName.C_Str(), m_bonemap[node->mName.C_Str()].second);
		if (!m_animations.empty())
			if (m_animations[0]->GetKeyframes().find(out->GetName()) != m_animations[0]->GetKeyframes().end())
			{
				Keyframe kf = m_animations[0]->GetKeyframes()[out->GetName()];
				auto pos = kf.positions[0];
				auto rot = kf.rotations[0];
				Transform tr;
				tr.SetPosition({ pos.x, pos.y, pos.z });
				tr.SetOrientation({ rot.x, rot.y, rot.z, rot.w });
				out->SetTransform(tr);
			}
			else out->SetTransform(To3DTransform(toglm(node->mTransformation)).first);

		out->SavePoseAsIdle();

		for (int i = 0; i < node->mNumChildren; i++)
		{
			std::shared_ptr<Bone> child = nullptr;
			if (processBone(node->mChildren[i], child))
			{
				m_bonesChildren.push_back(child);
				child->SetParent(out.get());
				out->AddChild(child.get());
			}
		}
		return true;
	}
	return false;
}

void Model::processTextures(const aiMesh* mesh, const aiScene* scene, std::vector<MaterialTexture>& textures)
{
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	loadTextureFromMaterial(aiTextureType_DIFFUSE, material, textures);
	loadTextureFromMaterial(aiTextureType_NORMALS, material, textures);
	loadTextureFromMaterial(aiTextureType_SPECULAR, material, textures);
	loadTextureFromMaterial(aiTextureType_HEIGHT, material, textures);
	loadTextureFromMaterial(aiTextureType_SHININESS, material, textures);
	loadTextureFromMaterial(aiTextureType_AMBIENT, material, textures);
}

void Model::loadTextureFromMaterial(aiTextureType textureType, const aiMaterial* mat, std::vector<MaterialTexture>& textures)
{
	// TODO: генерировать дефолтную текстуру если нет своей
	// TODO: кеширование текстур

	_ASSERT(mat);
	GLint TextureCount = mat->GetTextureCount(textureType);
	int TextureIndex = -1;
	if (TextureCount <= 0)
	{
		MaterialTexture MeshTexture;
		textures.push_back(MeshTexture); // TODO: зачем и как должно быть
	}
	for (GLint i = 0; i < TextureCount; ++i)
	{
		aiString Str;
		mat->GetTexture(textureType, i, &Str);
		std::string TextureName = Str.C_Str();
		std::string TexturePath = m_directory + "/" + TextureName;
		GLboolean Skip = GL_FALSE;
		GLint LoadedTextureCount = static_cast<int>(m_loadedTextures.size());
		for (int k = 0; k < LoadedTextureCount; ++k)
		{
			if (TexturePath == m_loadedTextures[k].path)
			{
				Skip = GL_TRUE;
				textures.push_back(m_loadedTextures[k]);
				break;
			}
		}
		if (!Skip)
		{
			MaterialTexture MeshTexture;
			MeshTexture.texture = std::make_shared<GLTexture2D>(TexturePath, STBI_rgb_alpha, true);
			MeshTexture.path = TexturePath;
			textures.push_back(MeshTexture);
			m_loadedTextures.push_back(MeshTexture);
		}
	}
}

void Model::processMatProperties(const aiMesh* mesh, const aiScene* scene, MaterialProperties& meshMatProperties)
{
	aiMaterial* pAiMat = scene->mMaterials[mesh->mMaterialIndex];
	aiColor3D AmbientColor, DiffuseColor, SpecularColor;
	float Shininess = 0.0f, Refracti = 0.0f;
	pAiMat->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor);
	pAiMat->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor);
	pAiMat->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor);
	pAiMat->Get(AI_MATKEY_SHININESS, Shininess);
	pAiMat->Get(AI_MATKEY_REFRACTI, Refracti);
	meshMatProperties.diffuseColor = { DiffuseColor.r, DiffuseColor.g, DiffuseColor.b };
	meshMatProperties.ambientColor = { AmbientColor.r, AmbientColor.g, AmbientColor.b };
	meshMatProperties.specularColor = { SpecularColor.r, SpecularColor.g, SpecularColor.b };
	meshMatProperties.shininess = Shininess;
	meshMatProperties.refracti = Refracti;
}

void Model::computeAABB()
{
	for (size_t i = 0; i < m_meshes.size(); ++i)
		m_bounding.Combine(m_meshes[i]->GetBounding());
}

void Model::calculatePose(Bone* bone)
{
	auto it = std::find_if(m_bones.begin(), m_bones.end(), [&](auto b) { return b.get() == bone; });
	auto it1 = std::find_if(m_bonesChildren.begin(), m_bonesChildren.end(), [&](auto b) { return b.get() == bone; });
	if (it == m_bones.end() && it1 == m_bonesChildren.end())
		return;

	Transform tmp = m_transform;
	m_transform = Transform{};

	auto finalTransform = Node::GetFinalTransform(bone) * bone->GetTransform();

	glm::mat4 finaltr(1.0);
	finaltr = glm::translate(finaltr, finalTransform.GetPosition());
	finaltr = finaltr * glm::toMat4(finalTransform.GetOrientation());
	finaltr = glm::scale(finaltr, bone->GetSize());

	m_pose[bone->GetID()] = finaltr * bone->GetOffset();

	for (auto child : bone->GetChildren())
		calculatePose(dynamic_cast<Bone*>(child));

	m_transform = tmp;
}

#pragma endregion

#pragma endregion

//==============================================================================
// Scene
//==============================================================================
#pragma region Scene

#pragma region QuadShape

QuadShape::QuadShape()
{
	m_vao = std::make_shared<GLVertexArray>(getData(), GetMeshVertexFormat());
}

void QuadShape::Draw()
{
	m_vao->DrawTriangles();
}

std::vector<MeshVertex> QuadShape::getData()
{
	// TODO: указать правильный Tangent
	return
	{
		// Positions            // Color            // Normals          // Texcoords  // Tangent
		{ {1.0f,  0.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
		{ {-1.0f, 0.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
		{ {-1.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} },
		{ {1.0f,  0.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
		{ {-1.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} },
		{ {1.0f,  0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f} }
	};
}

#pragma endregion

#pragma region CubeShape

CubeShape::CubeShape()
{
	m_vao = std::make_shared<GLVertexArray>(getData(), GetMeshVertexFormat());
}

void CubeShape::Draw()
{
	m_vao->DrawTriangles();
}

std::vector<MeshVertex> CubeShape::getData()
{




	// TODO: указать правильный Tangent
	return
	{
		// Positions              // Color            // Normals              // Texcoords   // Tangent
		// Back face
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {0.0f, 0.0f} }, // bottom-left
		{ { 1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {1.0f, 1.0f} }, // top-right
		{ { 1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {1.0f, 0.0f} }, // bottom-right
		{ { 1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {1.0f, 1.0f} }, // top-right
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {0.0f, 0.0f} }, // bottom-left
		{ {-1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f,}, {0.0f, 1.0f} }, // top-left
		// Front face
		{ {-1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {0.0f, 0.0f} }, // bottom-left
		{ { 1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {1.0f, 0.0f} }, // bottom-right
		{ { 1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {1.0f, 1.0f} }, // top-right
		{ { 1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {1.0f, 1.0f} }, // top-right
		{ {-1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {0.0f, 1.0f} }, // top-left
		{ {-1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f,}, {0.0f, 0.0f} }, // bottom-left
		// Left face
		{ {-1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {1.0f, 0.0f} }, // top-right
		{ {-1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {1.0f, 1.0f} }, // top-left
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {0.0f, 1.0f} }, // bottom-left
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {0.0f, 1.0f} }, // bottom-left
		{ {-1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {0.0f, 0.0f} }, // bottom-right
		{ {-1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f,}, {1.0f, 0.0f} }, // top-right
		// Right face
		{ { 1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {1.0f, 0.0f} }, // top-left
		{ { 1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {0.0f, 1.0f} }, // bottom-right
		{ { 1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {1.0f, 1.0f} }, // top-right
		{ { 1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {0.0f, 1.0f} }, // bottom-right
		{ { 1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {1.0f, 0.0f} }, // top-left
		{ { 1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f,}, {0.0f, 0.0f} }, // bottom-left
		// Bottom face
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {0.0f, 1.0f} }, // top-right
		{ { 1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {1.0f, 1.0f} }, // top-left
		{ { 1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {1.0f, 0.0f} }, // bottom-left
		{ { 1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {1.0f, 0.0f} }, // bottom-left
		{ {-1.0f, -1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {0.0f, 0.0f} }, // bottom-right
		{ {-1.0f, -1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f,}, {0.0f, 1.0f} }, // top-right
		// Top face
		{ {-1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {0.0f, 1.0f} }, // top-left
		{ { 1.0f,  1.0f , 1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {1.0f, 0.0f} }, // bottom-right
		{ { 1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {1.0f, 1.0f} }, // top-right
		{ { 1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {1.0f, 0.0f} }, // bottom-right
		{ {-1.0f,  1.0f, -1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {0.0f, 1.0f} }, // top-left
		{ {-1.0f,  1.0f,  1.0f,}, {1.0f, 1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f,}, {0.0f, 0.0f} }  // bottom-left
	};
}

#pragma endregion

#pragma region SphereShape

SphereShape::SphereShape()
{
	std::vector<MeshVertex> vertices;
	std::vector<uint16_t> indices;

	constexpr unsigned int X_SEGMENTS = 64;
	constexpr unsigned int Y_SEGMENTS = 64;
	const float PI = glm::pi<float>();
	for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
	{
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			MeshVertex vertex;
			vertex.position = glm::vec3(xPos, yPos, zPos);
			vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
			vertex.normal = glm::vec3(xPos, yPos, zPos);
			vertex.texCoords = glm::vec2(xSegment, ySegment);
			vertex.tangent = glm::vec3(0.0f); // TODO: исправить правильный tangent
			vertices.emplace_back(vertex);
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	m_vao = std::make_shared<GLVertexArray>(vertices, indices, GetMeshVertexFormat());
}

void SphereShape::Draw()
{
	m_vao->DrawTriangles();
}

#pragma endregion

#pragma region Camera

void Camera::Set(glm::vec3 Position, glm::vec3 Up, float Yaw, float Pitch)
{
	position = Position;
	front = CAMERA_FRONT;
	up = CAMERA_UP;
	right = CAMERA_RIGHT;
	worldUp = Up;
	yaw = Yaw;
	pitch = Pitch;
	update();
}

void Camera::Move(MovementDir direction, float deltaTime)
{
	const float velocity = movementSpeed * deltaTime;
	if (direction == Forward) position += front * velocity;
	else if (direction == Backward) position -= front * velocity;
	else if (direction == Left) position -= right * velocity;
	else if (direction == Right) position += right * velocity;
	update();
}

void Camera::Rotate(float xOffset, float yOffset)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;
	update();
}

const glm::mat4& Camera::GetViewMatrix() const
{
	return m_view;
}

void Camera::update()
{
	front = glm::normalize(
		glm::vec3
		(
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)), // x
			sin(glm::radians(pitch)), // y
			sin(glm::radians(yaw)) * cos(glm::radians(pitch)) // z
		)
	);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));

	m_view = glm::lookAt(position, position + front, up);
}

#pragma endregion

#pragma endregion

//==============================================================================
// Window
//==============================================================================
#pragma region Window

void framesizeCallback(GLFWwindow* /*window*/, int width, int height) noexcept
{
	width = std::max(width, 1);
	height = std::max(height, 1);

	if (Engine.windowWidth != width || Engine.windowHeight != height)
		Engine.IsResize = true;

	Engine.windowWidth = width;
	Engine.windowHeight = height;
}

void mouseCallback(GLFWwindow* /*window*/, double xPosIn, double yPosIn) noexcept
{
	MouseState.position = glm::ivec2{ xPosIn,yPosIn };
}

bool Window::Create(const char* title, int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 1);
#if defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	Engine.window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwGetFramebufferSize(Engine.window, &Engine.windowWidth, &Engine.windowHeight);

	glfwMakeContextCurrent(Engine.window);
	glfwSwapInterval(0);

	glfwSetInputMode(Engine.window, GLFW_STICKY_KEYS, GLFW_TRUE); // сохраняет событие клавиши до его опроса через glfwGetKey()
	glfwSetInputMode(Engine.window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE); // сохраняет событие кнопки мыши до его опроса через glfwGetMouseButton()

	glfwSetCursorPosCallback(Engine.window, mouseCallback);
	glfwSetFramebufferSizeCallback(Engine.window, framesizeCallback);

	gladLoadGL(glfwGetProcAddress);

	MouseState.lastPosition = MouseState.position = Mouse::GetPosition();

	return true;
}

void Window::Destroy()
{
	glfwDestroyWindow(Engine.window);
	glfwTerminate();
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(Engine.window) == GLFW_TRUE;
}

void Window::Update()
{
	Engine.IsResize = false;
	glfwPollEvents();

	MouseState.delta = MouseState.position - MouseState.lastPosition;
	MouseState.lastPosition = MouseState.position;
}

void Window::Swap()
{
	glfwSwapBuffers(Engine.window);
}

GLFWwindow* Window::GetWindow()
{
	return Engine.window;
}

int Window::GetWidth()
{
	return Engine.windowWidth;
}

int Window::GetHeight()
{
	return Engine.windowHeight;
}

bool Window::IsResize()
{
	return Engine.IsResize;
}

#pragma endregion

//==============================================================================
// Input
//==============================================================================
#pragma region Input

bool Keyboard::IsPressed(int key)
{
	return glfwGetKey(Engine.window, key) == GLFW_PRESS;
}

bool Mouse::IsPressed(Button button)
{
	return glfwGetMouseButton(Engine.window, static_cast<int>(button)) == GLFW_PRESS;
}

glm::ivec2 Mouse::GetPosition()
{
	return MouseState.position;
}

glm::ivec2 Mouse::GetDelta()
{
	return MouseState.delta;
}

void Mouse::SetPosition(const glm::ivec2& position)
{
	glfwSetCursorPos(Engine.window, position.x, position.y);
}

void Mouse::SetCursorMode(CursorMode mode)
{
	int mod = GLFW_CURSOR_NORMAL;

	if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_DISABLED;
	else if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_HIDDEN;

	glfwSetInputMode(Engine.window, GLFW_CURSOR, mod);
}

#pragma endregion

//==============================================================================
// IMGUI
//==============================================================================
#pragma region IMGUI

bool IMGUI::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.IniFilename = nullptr;

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	//ImGuiStyle& style = ImGui::GetStyle();

	ImGui_ImplGlfw_InitForOpenGL(Engine.window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	imgui.defaultFont = io.Fonts->AddFontFromFileTTF("Data/fonts/roboto-regular.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

	return true;
}

void IMGUI::Close()
{
	ImGui::GetIO().Fonts->Clear();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void IMGUI::Update()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void IMGUI::Draw()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#pragma endregion