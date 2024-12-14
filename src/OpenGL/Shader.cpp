#include <vector>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "src/Tools/Logger.h"

bool Shader::LoadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName)
{
	Logger::Log(1, "%s: loading vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__,
	            vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

	if (!CreateShaderProgram(vertexShaderFileName, fragmentShaderFileName))
	{
		Logger::Log(1, "%s error: shader program creation failed\n", __FUNCTION__);
		return false;
	}

	return true;
}

void Shader::Cleanup() const
{
	glDeleteProgram(m_shaderProgram);
}

void Shader::Use() const
{
	glUseProgram(m_shaderProgram);
}

GLuint Shader::LoadShader(std::string shaderFileName, GLuint shaderType)
{
	std::string shaderAsText;
	shaderAsText = LoadFileToString(shaderFileName);
	Logger::Log(4, "%s: loaded shader file '%s', size %i\n", __FUNCTION__, shaderFileName.c_str(), shaderAsText.size());

	const char* shaderSource = shaderAsText.c_str();
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, nullptr);
	glCompileShader(shader);

	if (!CheckCompileStats(shaderFileName, shader))
	{
		Logger::Log(1, "%s error: compiling shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
		return 0;
	}

	Logger::Log(1, "%s: shader %#x loaded and compiled\n", __FUNCTION__, shader);
	return shader;
}

bool Shader::CreateShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName)
{
	GLuint vertexShader = LoadShader(vertexShaderFileName, GL_VERTEX_SHADER);
	if (!vertexShader)
	{
		Logger::Log(1, "%s: loading of vertex shader '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str());
		return false;
	}

	GLuint fragmentShader = LoadShader(fragmentShaderFileName, GL_FRAGMENT_SHADER);
	if (!fragmentShader)
	{
		Logger::Log(1, "%s: loading of fragment shader '%s' failed\n", __FUNCTION__, fragmentShaderFileName.c_str());
		return false;
	}

	m_shaderProgram = glCreateProgram();

	glAttachShader(m_shaderProgram, vertexShader);
	glAttachShader(m_shaderProgram, fragmentShader);

	glLinkProgram(m_shaderProgram);

	if (!CheckLinkStats(vertexShaderFileName, fragmentShaderFileName, m_shaderProgram))
	{
		Logger::Log(1, "%s error: program linking from vertex shader '%s' / fragment shader '%s' failed\n",
		            __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return false;
	}

	/* it is safe to delete the original shaders here */
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	Logger::Log(1, "%s: shader program %#x successfully compiled from vertex shader '%s' and fragment shader '%s'\n",
	            __FUNCTION__, m_shaderProgram, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
	return true;
}

bool Shader::CheckCompileStats(std::string shaderFileName, GLuint shader)
{
	GLint isShaderCompiled;
	int logMessageLength;
	std::vector<char> shaderLog;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
	if (!isShaderCompiled)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logMessageLength);
		shaderLog = std::vector<char>(logMessageLength + 1);
		glGetShaderInfoLog(shader, logMessageLength, &logMessageLength, shaderLog.data());
		shaderLog.at(logMessageLength) = '\0';
		Logger::Log(1, "%s error: shader compile of shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
		Logger::Log(1, "%s compile Log:\n%s\n", __FUNCTION__, shaderLog.data());
		return false;
	}

	return true;
}

bool Shader::CheckLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram)
{
	GLint isProgramLinked;
	int logMessageLength;
	std::vector<char> programLog;

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isProgramLinked);
	if (!isProgramLinked)
	{
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logMessageLength);
		programLog = std::vector<char>(logMessageLength + 1);
		glGetProgramInfoLog(shaderProgram, logMessageLength, &logMessageLength, programLog.data());
		programLog.at(logMessageLength) = '\0';
		Logger::Log(1, "%s error: program linking of shaders '%s' and '%s' failed\n", __FUNCTION__,
		            vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
		Logger::Log(1, "%s compile Log:\n%s\n", __FUNCTION__, programLog.data());
		return false;
	}

	return true;
}

std::string Shader::LoadFileToString(std::string fileName)
{
	std::ifstream inFile(fileName);
	std::string str;

	if (inFile.is_open())
	{
		str.clear();
		// allocate string data (no slower realloc)
		inFile.seekg(0, std::ios::end);
		str.reserve(inFile.tellg());
		inFile.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(inFile)),
		           std::istreambuf_iterator<char>());
		inFile.close();
	}
	else
	{
		Logger::Log(1, "%s error: could not open file %s\n", __FUNCTION__, fileName.c_str());
		// TODO:   Logger::log(1, "%s error: system says '%s'\n", __FUNCTION__, strerror(errno));
		return std::string();
	}

	if (inFile.bad() || inFile.fail())
	{
		Logger::Log(1, "%s error: error while reading file %s\n", __FUNCTION__, fileName.c_str());
		inFile.close();
		return std::string();
	}

	inFile.close();
	Logger::Log(1, "%s: file %s successfully read to string\n", __FUNCTION__, fileName.c_str());
	return str;
}

void Shader::SetBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(m_shaderProgram, name.c_str()), static_cast<int>(value));
}

void Shader::SetInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(m_shaderProgram, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(m_shaderProgram, name.c_str()), value);
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) const
{
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, value_ptr(value));
}

void Shader::SetVec3(const std::string& name, glm::vec3& value) const
{
	glUniform3fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, &value[0]);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(m_shaderProgram, name.c_str()), x, y, z);
}

void Shader::SetVec2(const std::string& name, float x, float y) const
{
	glUniform2f(glGetUniformLocation(m_shaderProgram, name.c_str()), x, y);
}
