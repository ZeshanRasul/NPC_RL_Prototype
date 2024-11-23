#pragma once
#include "RenderData.h"
#include "src/Camera.h"
#include "src/GameObjects/GameObject.h"
#include "src/GameObjects/Quad.h"
#include "Shader.h"
#include "Cubemap.h"

#include <GLFW/glfw3.h>

class Renderer {
public:
	Renderer(GLFWwindow* window);

	bool init(unsigned int width, unsigned int height);
	void SetUpMinimapFBO(unsigned int width, unsigned int height);
	void SetUpShadowMapFBO(unsigned int width, unsigned int height);

	void setScene(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 cmapView, DirLight light);
	void draw(GameObject* gameObj, glm::mat4 viewMat, glm::mat4 proj, glm::vec3 camPos, bool shadowMap, glm::mat4 lightSpaceMat);
	void drawCubemap(Cubemap* cubemap);
	void drawMinimap(Quad* minimapQuad, Shader* minimapShader);
	void drawShadowMap(Quad* shadowMapQuad, Shader* shadowMapShader);

	void bindMinimapFBO(unsigned int width, unsigned int height);
	void unbindMinimapFBO();
	void bindShadowMapFBO(unsigned int width, unsigned int height);
	void unbindShadowMapFBO();

	GLuint GetShadowMapTexture() { return shadowMapTex; }

	void ResetViewport(unsigned int width, unsigned int height);
	void clear();

	void cleanup();

private:
	RenderData mRenderData{};

	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 cubemapView = glm::mat4(1.0f);
	
	DirLight sun;
	GLuint minimapFBO;
	GLuint minimapColorTex;
	GLuint minimapRBO;
	GLuint shadowMapFBO;
	GLuint shadowMapTex;
};