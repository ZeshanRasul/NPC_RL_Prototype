#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <tiny_gltf.h>

#include "Texture.h"

#include "src/OpenGL/RenderData.h"
#include "Model/GltfNode.h"

class GltfModel {
public:
    bool loadModel(RenderData& renderData, std::string modelFilename,
        std::string textureFilename);
    void draw();
    void cleanup();
    void uploadVertexBuffers();
    void uploadIndexBuffer();

private:
    void createVertexBuffers();
    void createIndexBuffer();
    int getTriangleCount();

    std::shared_ptr<GltfNode> mRootNode = nullptr;
    std::shared_ptr<tinygltf::Model> mModel = nullptr;

    GLuint mVAO = 0;
    std::vector<GLuint> mVertexVBO{};
    GLuint mIndexVBO = 0;
    std::map<std::string, GLint> attributes =
    { {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2} };

    Texture mTex{};
};