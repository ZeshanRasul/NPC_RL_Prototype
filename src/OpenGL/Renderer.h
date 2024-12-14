#pragma once
#include "RenderData.h"
#include "src/Camera.h"
#include "src/GameObjects/GameObject.h"
#include "src/GameObjects/Quad.h"
#include "Shader.h"
#include "Cubemap.h"

#include <GLFW/glfw3.h>

class Renderer
{
public:
	Renderer(GLFWwindow* window);

	bool Init(unsigned int width, unsigned int height);
	void SetUpMinimapFBO(unsigned int width, unsigned int height);
	void SetUpShadowMapFBO(unsigned int width, unsigned int height);

	void SetScene(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 cmapView, DirLight light);
	void Draw(GameObject* gameObj, glm::mat4 viewMat, glm::mat4 proj, glm::vec3 camPos, bool shadowMap,
	          glm::mat4 lightSpaceMat);
	void DrawCubemap(Cubemap* cubemap);
	void DrawMinimap(Quad* minimapQuad, Shader* minimapShader);
	void DrawShadowMap(Quad* shadowMapQuad, Shader* shadowMapShader);

	void BindMinimapFbo(unsigned int width, unsigned int height);
	void UnbindMinimapFbo();
	void BindShadowMapFbo(unsigned int width, unsigned int height);
	void UnbindShadowMapFbo();

	GLuint GetShadowMapTexture() { return m_shadowMapTex; }

	void ResetRenderStates();
	void RemoveDepthAndSetBlending();

	void ResetViewport(unsigned int width, unsigned int height);
	void Clear();

	void Cleanup();

private:
	RenderData m_renderData{};

	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);
	glm::mat4 m_cubemapView = glm::mat4(1.0f);

	DirLight m_sun;
	GLuint m_minimapFbo;
	GLuint m_minimapColorTex;
	GLuint m_minimapRbo;
	GLuint m_shadowMapFbo;
	GLuint m_shadowMapTex;
};
