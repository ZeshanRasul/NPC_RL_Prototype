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

	if (!GLAD_GL_VERSION_4_6) {
		Logger::log(1, "%s error: failed to get at least OpenGL 3.3\n", __FUNCTION__);
		return false;
	}

	GLint majorVersion, minorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	Logger::log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

	glViewport(0, 0, width, height);

//	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	return true;
}

void Renderer::setScene(glm::mat4 viewMat, glm::mat4 proj, DirLight light)
{
	view = viewMat;
	projection = proj;
	sun = light;
}

void Renderer::draw(GameObject* gameObj, glm::mat4 viewMat, glm::mat4 proj)
{
	Shader* shader = gameObj->GetShader();
	shader->use();
	shader->setVec3("dirLight.direction", sun.direction);
	shader->setVec3("dirLight.ambient", sun.ambient);
	shader->setVec3("dirLight.diffuse", sun.diffuse);
	shader->setVec3("dirLight.specular", sun.specular);


//	gameObj->ApplySkinning();
	gameObj->Draw(viewMat, proj);
}

void Renderer::clear()
{
	glClearColor(0.2f, 0.3f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::cleanup()
{
}
