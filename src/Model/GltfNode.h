#pragma once
#include <vector>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class GltfNode
{
public:
	static std::shared_ptr<GltfNode> CreateRoot(int rootNodeNum);
	void AddChilds(std::vector<int> childNodes);
	std::vector<std::shared_ptr<GltfNode>> GetChilds();
	int GetNodeNum();

	void SetNodeName(std::string name);
	std::string GetNodeName();

	void SetScale(glm::vec3 scale);
	void SetTranslation(glm::vec3 translation);
	void SetRotation(glm::quat rotation);

	void BlendScale(glm::vec3 scale, float blendFactor);
	void BlendTranslation(glm::vec3 translation, float blendFactor);
	void BlendRotation(glm::quat rotation, float blendFactor);

	void CalculateLocalTrsMatrix();
	void CalculateNodeMatrix(glm::mat4 parentNodeMatrix);
	glm::mat4 GetNodeMatrix();

	void PrintTree();

private:
	void PrintNodes(std::shared_ptr<GltfNode> startNode, int indent);

	int m_nodeNum = 0;
	std::string m_nodeName;

	std::vector<std::shared_ptr<GltfNode>> m_childNodes{};

	glm::vec3 m_blendScale = glm::vec3(1.0f);
	glm::vec3 m_blendTranslation = glm::vec3(0.0f);
	glm::quat m_blendRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	glm::vec3 m_scale = glm::vec3(1.0f);
	glm::vec3 m_translation = glm::vec3(0.0f);
	glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	glm::mat4 m_localTrsMatrix = glm::mat4(1.0f);
	glm::mat4 m_nodeMatrix = glm::mat4(1.0f);
	glm::mat4 m_inverseBindMatrix = glm::mat4(1.0f);
};
