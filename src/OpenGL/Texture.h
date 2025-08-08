#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Texture
{
public:
	bool LoadTexture(std::string textureFilename, bool flipImage = true);
	void Bind(int texIndex = 0);
	void Unbind();
	void Cleanup();

	int GetTexId() const { return m_texture; }

private:
	GLuint m_texture = 0;
	int m_texWidth = 0;
	int m_texHeight = 0;
	int m_numberOfChannels = 0;
	std::string m_textureName;
};
