#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Logger.h"
#include "Texture.h"

class Cell {
public:
    Cell(bool isObs, glm::vec3 col)
        : isObstacle(isObs), color(col)
    {
        SetUpVAO();
    }

    Cell() : isObstacle(false), color(glm::vec3(0.0f, 1.0f, 0.0f))
    {
        SetUpVAO();
    }

    void SetUpVAO() {
        glGenVertexArrays(1, &cellVAO);
        glGenBuffers(1, &cellVBO);

        glBindVertexArray(cellVAO);

        glBindBuffer(GL_ARRAY_BUFFER, cellVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cellVertices), cellVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    bool LoadTexture(std::string textureFilename)
    {
        if (!mTex.loadTexture(textureFilename, false)) {
            Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
            return false;
        }
        Logger::log(1, "%s: Crosshair texture successfully loaded\n", __FUNCTION__, textureFilename);
        return true;
    }

    void BindVAO() const {
        glBindVertexArray(cellVAO);
    }

    bool IsObstacle() const {
		return isObstacle;
	}

	void SetObstacle(bool obs) {
		isObstacle = obs;
	}

    bool IsCover() const {
        return isCover;
    }

    void SetCover(bool cover) {
        isCover = cover;
    }

    glm::vec3 GetColor() const {
		return color;
	}

	void SetColor(glm::vec3 col) {
		color = col;
	}

    Texture mTex{};
private:
    GLuint cellVAO = 0;
    GLuint cellVBO = 0;

    // Vertex data for a cell
    float cellVertices[30] = {
        // positions         // texture coords
         0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         1.0f, 0.0f, 1.0f,   1.0f, 1.0f,

         0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         1.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.0f, 0.0f, 1.0f,   0.0f, 1.0f
    };

    bool isObstacle;
    glm::vec3 color;
    bool isCover;

};