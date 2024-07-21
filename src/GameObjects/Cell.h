#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

// Vertex data for a cell
static float cellVertices[] = {
    // positions         // texture coords
     0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
     1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
     1.0f, 0.0f, 1.0f,   1.0f, 1.0f,

     0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
     1.0f, 0.0f, 1.0f,   1.0f, 1.0f,
     0.0f, 0.0f, 1.0f,   0.0f, 1.0f
};

// Define cell structure
struct Cell {
    bool isObstacle;
    glm::vec3 color;

    unsigned int cellVAO, cellVBO;

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

    void BindVAO() {
        glBindVertexArray(cellVAO);
    }
};