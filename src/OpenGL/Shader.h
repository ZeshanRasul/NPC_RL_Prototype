#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
	bool LoadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName);
	void Use() const;
	void Cleanup() const;

	void SetBool(const std::string& name, bool value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetMat4(const std::string& name, glm::mat4 value) const;
	void SetVec3(const std::string& name, glm::vec3& value) const;
	void SetVec3(const std::string& name, float x, float y, float z) const;
	void SetVec2(const std::string& name, float x, float y) const;

	GLuint GetProgram() const { return m_shaderProgram; }

private:
	GLuint m_shaderProgram = 0;

	bool CreateShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName);
	GLuint LoadShader(std::string shaderFileName, GLuint shaderType);
	std::string LoadFileToString(std::string filename);
	bool CheckCompileStats(std::string shaderFileName, GLuint shader);
	bool CheckLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram);
};
