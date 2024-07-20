#include "Renderer.h"
#include "src/Tools/Logger.h"

Renderer::Renderer(GLFWwindow* window)
{
	mRenderData.rdWindow = window;
}

bool Renderer::init(unsigned int width, unsigned int height)
{
	mRenderData.rdWidth = width;
	mRenderData.rdHeight = height;

	// Load all OpenGL function pointers with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		Logger::log(1, "%s error: failed to initialize GLAD\n", __FUNCTION__);
		return false;
	}

	if (!GLAD_GL_VERSION_3_3) {
		Logger::log(1, "%s error: failed to get at least OpenGL 3.3\n", __FUNCTION__);
		return false;
	}

	GLint majorVersion, minorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	Logger::log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

	glViewport(0, 0, width, height);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	return false;
}

void Renderer::draw()
{
}

void Renderer::cleanup()
{
}
