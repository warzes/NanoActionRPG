#include "NanoEngine.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 1);
#if defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Game", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	gladLoadGL(glfwGetProcAddress);

	Renderer3D renderer;

#if defined(_DEBUG)
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	//glDebugMessageCallback(gl3d::glDebugOutput, &renderer);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

#pragma region deltaTime
	int fpsCount = 0;
	float timeFpsCount = 0;
	int timeBeg = clock();
#pragma endregion

#pragma region windowSize
	int windowWidth = 0;
	int windowHeight = 0;
#pragma endregion

	while (!glfwWindowShouldClose(window))
	{
#pragma region window metrics
		glfwGetWindowSize(window, &windowWidth, &windowHeight);
		windowWidth = std::max(windowWidth, 1);
		windowHeight = std::max(windowHeight, 1);
#pragma endregion

#pragma region deltatime
		int timeEnd = clock();
		float deltaTime = (timeEnd - timeBeg) / 1000.f;
		timeBeg = clock();

		timeFpsCount += deltaTime;
		fpsCount++;
		if (timeFpsCount > 1)
		{
			timeFpsCount -= 1;
			std::string name = std::to_string(fpsCount);
			name = "fps: " + name;
			glfwSetWindowTitle(window, name.c_str());

			fpsCount = 0;
		}
#pragma endregion


#pragma region render and events
		//renderer.render(deltaTime);

		glfwSwapBuffers(window);
		glfwPollEvents();
		glViewport(0, 0, windowWidth, windowHeight);
		//renderer.updateWindowMetrics(w, h);
#pragma endregion
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}
//-----------------------------------------------------------------------------