#include "Renderer.h"
#include "src/Tools/Logger.h"

Renderer::Renderer(GLFWwindow* window)
{
	m_renderData.m_rdWindow = window;
}

bool Renderer::Init(unsigned int width, unsigned int height)
{
	m_renderData.m_rdWidth = width;
	m_renderData.m_rdHeight = height;

	// Load all OpenGL function pointers with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		Logger::Log(1, "%s error: failed to initialize GLAD\n", __FUNCTION__);
		return false;
	}

	if (!GLAD_GL_VERSION_4_6)
	{
		Logger::Log(1, "%s error: failed to get at least OpenGL 3.3\n", __FUNCTION__);
		return false;
	}

	GLint majorVersion, minorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	Logger::Log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

	glViewport(0, 0, width, height);

	//	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	return true;
}

void Renderer::SetUpMinimapFBO(unsigned int width, unsigned int height)
{
	glGenFramebuffers(1, &m_minimapFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_minimapFbo);

	glGenTextures(1, &m_minimapColorTex);
	glBindTexture(GL_TEXTURE_2D, m_minimapColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_minimapColorTex, 0);

	glGenRenderbuffers(1, &m_minimapRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_minimapRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_minimapRbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::Log(1, "%s error: framebuffer is not complete\n", __FUNCTION__);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderer::SetUpShadowMapFBO(unsigned int width, unsigned int height)
{
	glGenFramebuffers(1, &m_shadowMapFbo);

	// create depth texture
	glGenTextures(1, &m_shadowMapTex);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SetScene(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 cmapView, DirLight light)
{
	m_view = viewMat;
	m_projection = proj;
	m_cubemapView = cmapView;
	m_sun = light;
}

void Renderer::Draw(GameObject* gameObj, glm::mat4 viewMat, glm::mat4 proj, glm::vec3 camPos, bool shadowMap,
                    glm::mat4 lightSpaceMat)
{
	Shader* shader;
	if (shadowMap)
	{
		shader = gameObj->GetShadowShader();
		glCullFace(GL_FRONT);
	}
	else
	{
		glCullFace(GL_NONE);
		shader = gameObj->GetShader();
	}

	shader->Use();
	shader->SetVec3("dirLight.direction", m_sun.m_direction);
	shader->SetVec3("dirLight.m_ambient", m_sun.m_ambient);
	shader->SetVec3("dirLight.diffuse", m_sun.m_diffuse);
	shader->SetVec3("dirLight.specular", m_sun.m_specular);
	shader->SetVec3("lightPos", m_sun.m_direction);
	shader->SetVec3("cameraPos", camPos);
	gameObj->Draw(viewMat, proj, shadowMap, lightSpaceMat, m_shadowMapTex);
}

void Renderer::DrawCubemap(Cubemap* cubemap)
{
	cubemap->GetShader()->Use();
	cubemap->GetShader()->SetMat4("view", m_cubemapView);
	cubemap->GetShader()->SetMat4("projection", m_projection);

	cubemap->Draw();
}

void Renderer::DrawMinimap(Quad* minimapQuad, Shader* minimapShader)
{
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, m_minimapColorTex);
	minimapShader->Use();
	minimapShader->SetInt("minimapTex", 0);

	minimapQuad->Draw();
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawShadowMap(Quad* shadowMapQuad, Shader* shadowMapShader)
{
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
	shadowMapShader->Use();
	shadowMapShader->SetInt("shadowMap", 0);
	auto ndcOffset = glm::vec2(0.0f, 0.5f);
	shadowMapShader->SetVec2("ndcOffset", ndcOffset.x, ndcOffset.y);
	shadowMapQuad->Draw();
	glEnable(GL_DEPTH_TEST);
}

void Renderer::BindMinimapFbo(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_minimapFbo);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::UnbindMinimapFbo()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BindShadowMapFbo(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFbo);
	glViewport(0, 0, width, height);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::UnbindShadowMapFbo()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ResetRenderStates()
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void Renderer::RemoveDepthAndSetBlending()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::ResetViewport(unsigned int width, unsigned int height)
{
	glViewport(0, 0, width, height);
}

void Renderer::Clear()
{
	glClearColor(0.2f, 0.3f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::Cleanup()
{
}
