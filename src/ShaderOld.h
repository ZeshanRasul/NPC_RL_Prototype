#pragma once

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ShaderOld
{
public:
	unsigned int ID;

	ShaderOld(const char* vertexPath, const char* fragmentPath);

	void use();

	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setMat4(const std::string& name, glm::mat4 value) const;
	void setVec3(const std::string& name, glm::vec3& value) const;
	void setVec3(const std::string& name, float x, float y, float z) const;
};