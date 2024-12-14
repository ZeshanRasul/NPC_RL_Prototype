#include <stb_image.h>

#include "Texture.h"
#include "src/Tools/Logger.h"

void Texture::Cleanup()
{
	glDeleteTextures(1, &m_texture);
}

bool Texture::LoadTexture(std::string textureFilename, bool flipImage)
{
	m_textureName = textureFilename;

	stbi_set_flip_vertically_on_load(flipImage);
	unsigned char* textureData = stbi_load(textureFilename.c_str(), &m_texWidth, &m_texHeight, &m_numberOfChannels, 0);

	if (!textureData)
	{
		Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__, m_textureName.c_str());
		stbi_image_free(textureData);
		return false;
	}

	GLenum format;
	if (m_numberOfChannels == 1)
		format = GL_RED;
	else if (m_numberOfChannels == 3)
		format = GL_RGB;
	else if (m_numberOfChannels == 4)
		format = GL_RGBA;


	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, format, m_texWidth, m_texHeight, 0, format, GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(textureData);

	Logger::Log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, m_textureName.c_str(), m_texWidth,
	            m_texHeight, m_numberOfChannels);
	return true;
}

void Texture::Bind(int texIndex)
{
	glActiveTexture(GL_TEXTURE0 + texIndex);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}
