#include <vector>
#include <iostream>
#include <fstream>
#include <cerrno>  
#include <cstring> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "src/Tools/Logger.h"

bool Shader::loadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName) {
    Logger::log(1, "%s: loading vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

    if (!createShaderProgram(vertexShaderFileName, fragmentShaderFileName)) {
        Logger::log(1, "%s error: shader program creation failed\n", __FUNCTION__);
        return false;
    }

    return true;
}

void Shader::cleanup() const {
    glDeleteProgram(mShaderProgram);
}

void Shader::use() const {
    glUseProgram(mShaderProgram);
}

GLuint Shader::loadShader(std::string shaderFileName, GLuint shaderType) {
    std::string shaderAsText;
    shaderAsText = loadFileToString(shaderFileName);
    Logger::log(4, "%s: loaded shader file '%s', size %i\n", __FUNCTION__, shaderFileName.c_str(), shaderAsText.size());

    const char* shaderSource = shaderAsText.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const GLchar**)&shaderSource, 0);
    glCompileShader(shader);

    if (!checkCompileStats(shaderFileName, shader)) {
        Logger::log(1, "%s error: compiling shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
        return 0;
    }

    Logger::log(1, "%s: shader %#x loaded and compiled\n", __FUNCTION__, shader);
    return shader;
}

bool Shader::createShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName) {
    GLuint vertexShader = loadShader(vertexShaderFileName, GL_VERTEX_SHADER);
    if (!vertexShader) {
        Logger::log(1, "%s: loading of vertex shader '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str());
        return false;
    }

    GLuint fragmentShader = loadShader(fragmentShaderFileName, GL_FRAGMENT_SHADER);
    if (!fragmentShader) {
        Logger::log(1, "%s: loading of fragment shader '%s' failed\n", __FUNCTION__, fragmentShaderFileName.c_str());
        return false;
    }

    mShaderProgram = glCreateProgram();

    glAttachShader(mShaderProgram, vertexShader);
    glAttachShader(mShaderProgram, fragmentShader);

    glLinkProgram(mShaderProgram);

    if (!checkLinkStats(vertexShaderFileName, fragmentShaderFileName, mShaderProgram)) {
        Logger::log(1, "%s error: program linking from vertex shader '%s' / fragment shader '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return false;
    }

    /* it is safe to delete the original shaders here */
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    Logger::log(1, "%s: shader program %#x successfully compiled from vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__, mShaderProgram, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
    return true;
}

bool Shader::checkCompileStats(std::string shaderFileName, GLuint shader) {
    GLint isShaderCompiled;
    int logMessageLength;
    std::vector<char> shaderLog;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logMessageLength);
        shaderLog = std::vector<char>(logMessageLength + 1);
        glGetShaderInfoLog(shader, logMessageLength, &logMessageLength, shaderLog.data());
        shaderLog.at(logMessageLength) = '\0';
        Logger::log(1, "%s error: shader compile of shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
        Logger::log(1, "%s compile log:\n%s\n", __FUNCTION__, shaderLog.data());
        return false;
    }

    return true;
}

bool Shader::checkLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram) {
    GLint isProgramLinked;
    int logMessageLength;
    std::vector<char> programLog;

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isProgramLinked);
    if (!isProgramLinked) {
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logMessageLength);
        programLog = std::vector<char>(logMessageLength + 1);
        glGetProgramInfoLog(shaderProgram, logMessageLength, &logMessageLength, programLog.data());
        programLog.at(logMessageLength) = '\0';
        Logger::log(1, "%s error: program linking of shaders '%s' and '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
        Logger::log(1, "%s compile log:\n%s\n", __FUNCTION__, programLog.data());
        return false;
    }

    return true;
}

std::string Shader::loadFileToString(std::string fileName) {
    std::ifstream inFile(fileName);
    std::string str;

    if (inFile.is_open()) {
        str.clear();
        // allocate string data (no slower realloc)
        inFile.seekg(0, std::ios::end);
        str.reserve(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        str.assign((std::istreambuf_iterator<char>(inFile)),
            std::istreambuf_iterator<char>());
        inFile.close();
    }
    else {
        Logger::log(1, "%s error: could not open file %s\n", __FUNCTION__, fileName.c_str());
     // TODO:   Logger::log(1, "%s error: system says '%s'\n", __FUNCTION__, strerror(errno));
        return std::string();
    }

    if (inFile.bad() || inFile.fail()) {
        Logger::log(1, "%s error: error while reading file %s\n", __FUNCTION__, fileName.c_str());
        inFile.close();
        return std::string();
    }

    inFile.close();
    Logger::log(1, "%s: file %s successfully read to string\n", __FUNCTION__, fileName.c_str());
    return str;
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(mShaderProgram, name.c_str()), int(value));
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(mShaderProgram, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(mShaderProgram, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, glm::mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(mShaderProgram, name.c_str()), 1, GL_FALSE, value_ptr(value));
}

void Shader::setVec3(const std::string& name, glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(mShaderProgram, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(mShaderProgram, name.c_str()), x, y, z);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(mShaderProgram, name.c_str()), x, y);

}
