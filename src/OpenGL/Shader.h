#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader {
public:
    bool loadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName);
    void use() const;
    void cleanup() const;

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, glm::mat4 value) const;
    void setVec3(const std::string& name, glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec2(const std::string& name, float x, float y) const;

private:
    GLuint mShaderProgram = 0;

    bool createShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName);
    GLuint loadShader(std::string shaderFileName, GLuint shaderType);
    std::string loadFileToString(std::string filename);
    bool checkCompileStats(std::string shaderFileName, GLuint shader);
    bool checkLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram);
};