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

void Renderer::SetUpMinimapFBO(unsigned int width, unsigned int height)
{
	glGenFramebuffers(1, &minimapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);

	glGenTextures(1, &minimapColorTex);
	glBindTexture(GL_TEXTURE_2D, minimapColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, minimapColorTex, 0);

	glGenRenderbuffers(1, &minimapRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, minimapRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, minimapRBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::log(1, "%s error: framebuffer is not complete\n", __FUNCTION__);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderer::SetUpShadowMapFBO(unsigned int width, unsigned int height)
{
	glGenFramebuffers(1, &shadowMapFBO);

	// create depth texture
	glGenTextures(1, &shadowMapTex);
	glBindTexture(GL_TEXTURE_2D, shadowMapTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setScene(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 cmapView, DirLight light)
{
	view = viewMat;
	projection = proj;
	cubemapView = cmapView;
	sun = light;
}

void Renderer::draw(GameObject* gameObj, glm::mat4 viewMat, glm::mat4 proj, glm::vec3 camPos, bool shadowMap)
{
	Shader* shader;
	if (shadowMap)
	{
		shader = gameObj->GetShadowShader();
	}
	else
	{
		shader = gameObj->GetShader();
	}

	shader->use();
	shader->setVec3("dirLight.direction", sun.direction);
	shader->setVec3("dirLight.ambient", sun.ambient);
	shader->setVec3("dirLight.diffuse", sun.diffuse);
	shader->setVec3("dirLight.specular", sun.specular);
	shader->setVec3("lightPos", sun.direction);
	shader->setVec3("cameraPos", camPos);
	gameObj->Draw(viewMat, proj, shadowMap);
}

void Renderer::drawCubemap(Cubemap* cubemap)
{
	cubemap->GetShader()->use();
	cubemap->GetShader()->setMat4("view", cubemapView);
	cubemap->GetShader()->setMat4("projection", projection);

	cubemap->Draw();

}

void Renderer::drawMinimap(Quad* minimapQuad, Shader* minimapShader)
{
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, minimapColorTex);
	minimapShader->use();
	minimapShader->setInt("minimapTex", 0);

	minimapQuad->Draw();
	glEnable(GL_DEPTH_TEST);
}

void Renderer::drawShadowMap(Quad* shadowMapQuad, Shader* shadowMapShader)
{
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowMapTex);
	shadowMapShader->use();
	shadowMapShader->setInt("shadowMap", 0);
	glm::vec2 ndcOffset = glm::vec2(0.0f, 0.5f);
	shadowMapShader->setVec2("ndcOffset", ndcOffset.x, ndcOffset.y);
	shadowMapQuad->Draw();
	glEnable(GL_DEPTH_TEST);
}

void Renderer::bindMinimapFBO(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::unbindMinimapFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::bindShadowMapFBO(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glViewport(0, 0, width, height);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::unbindShadowMapFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ResetViewport(unsigned int width, unsigned int height)
{
	glViewport(0, 0, width, height);
}

void Renderer::clear()
{
	glClearColor(0.2f, 0.3f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::cleanup()
{
}
