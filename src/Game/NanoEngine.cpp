#include "NanoEngine.h"

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
		GLFWwindow* window = nullptr;
		int windowWidth = 0;
		int windowHeight = 0;
		bool IsResize = false;
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
// END Render Core
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

void GLProgramPipeline::Bind()
{
	glBindProgramPipeline(m_handle);
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

void GLVertexArray::Bind()
{
	glBindVertexArray(m_handle);
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

#pragma region GLTexture2D

GLTexture2D::GLTexture2D(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, bool generateMipMaps)
	: GLTexture2D(internalFormat, format, GL_UNSIGNED_BYTE, width, height, data, filter, repeat, generateMipMaps)
{
}

GLTexture2D::GLTexture2D(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, bool generateMipMaps)
{
	createHandle();
	createTexture(internalFormat, format, dataType, width, height, data, filter, repeat, generateMipMaps);
}

GLTexture2D::GLTexture2D(std::string_view filepath, int comp, bool generateMipMaps)
{
	if (!std::filesystem::exists(filepath.data()))
	{
		Error("File '" + std::string(filepath.data()) + "' does not exist.");
		return;
	}

	int w, h, c;
	const auto data = stbi_load(filepath.data(), &w, &h, &c, comp);

	const auto [internalFormat, format] = STBImageToOpenGLFormat((comp));

	createHandle();
	createTexture(internalFormat, format, GL_UNSIGNED_BYTE, w, h, data, GL_LINEAR, GL_REPEAT, generateMipMaps);
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

void GLTexture2D::createTexture(GLenum internalFormat, GLenum format, GLenum dataType, GLsizei width, GLsizei height, void* data, GLint filter, GLint repeat, bool generateMipMaps)
{
	int levels = 1;
	if (generateMipMaps)
		levels = 1 + (int)std::floor(std::log2(std::min(width, height)));

	glTextureStorage2D(m_handle, levels, internalFormat, width, height);

	if (generateMipMaps)
	{
		if (filter == GL_LINEAR)
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else if (filter == GL_NEAREST)
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else
			Fatal("Unsupported filter");
	}
	else
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filter);

	glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, filter);

	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, repeat);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, repeat);
	glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, repeat);

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
// END Render Resources
//==============================================================================

//==============================================================================
// Renderer3D
//==============================================================================
#pragma region Renderer3D

void Renderer3D::MainFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer3D::BlitFrameBuffer(
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

void Renderer3D::Clear(bool color, bool depth, bool stencil)
{
	GLbitfield mask = 0;
	if (color) mask |= GL_COLOR_BUFFER_BIT;
	if (depth) mask |= GL_DEPTH_BUFFER_BIT;
	if (stencil) mask |= GL_STENCIL_BUFFER_BIT;
	glClear(mask);
}

void Renderer3D::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

void Renderer3D::SetScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glScissor(x, y, width, height);
}

#pragma endregion
//==============================================================================
// END Renderer3D
//==============================================================================

//==============================================================================
// RenderWorld
//==============================================================================
#pragma region RenderWorld

#pragma endregion

//==============================================================================
// END RenderWorld
//==============================================================================

//==============================================================================
// Window
//==============================================================================
#pragma region Window

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
	glfwMakeContextCurrent(Engine.window);
	glfwSwapInterval(0);

	glfwSetInputMode(Engine.window, GLFW_STICKY_KEYS, GLFW_TRUE); // сохраняет событие клавиши до его опроса через glfwGetKey()
	glfwSetInputMode(Engine.window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE); // сохраняет событие кнопки мыши до его опроса через glfwGetMouseButton()

	gladLoadGL(glfwGetProcAddress);

	return true;
}

void Window::Destroy()
{
	glfwDestroyWindow(Engine.window);
	glfwTerminate();
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(Engine.window);
}

void Window::Update()
{
	Engine.IsResize = false;
	glfwPollEvents();

	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(Engine.window, &width, &height);

	if (Engine.windowWidth != width || Engine.windowHeight != height)
		Engine.IsResize = true;

	Engine.windowWidth = std::max(width, 1);
	Engine.windowHeight = std::max(height, 1);
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
	return false;
}

#pragma endregion

//==============================================================================
// END Window
//==============================================================================

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
	double xpos, ypos;
	glfwGetCursorPos(Engine.window, &xpos, &ypos);
	return { xpos, ypos };
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
// END Input
//==============================================================================