#include <algorithm>

#include "GltfNode.h"
#include "Logger.h"

std::shared_ptr<GltfNode> GltfNode::CreateRoot(int rootNodeNum)
{
	auto mParentNode = std::make_shared<GltfNode>();
	mParentNode->m_nodeNum = rootNodeNum;
	return mParentNode;
}

void GltfNode::AddChilds(std::vector<int> childNodes)
{
	for (const int childNode : childNodes)
	{
		auto child = std::make_shared<GltfNode>();
		child->m_nodeNum = childNode;

		m_childNodes.push_back(child);
	}
}

std::vector<std::shared_ptr<GltfNode>> GltfNode::GetChilds()
{
	return m_childNodes;
}

int GltfNode::GetNodeNum()
{
	return m_nodeNum;
}

void GltfNode::SetNodeName(std::string name)
{
	m_nodeName = name;
}

std::string GltfNode::GetNodeName()
{
	return m_nodeName;
}

void GltfNode::SetScale(glm::vec3 scale)
{
	m_scale = scale;
	m_blendScale = scale;
}

void GltfNode::SetTranslation(glm::vec3 translation)
{
	m_translation = translation;
	m_blendTranslation = translation;
}

void GltfNode::SetRotation(glm::quat rotation)
{
	m_rotation = rotation;
	m_blendRotation = rotation;
}

void GltfNode::BlendScale(glm::vec3 scale, float blendFactor)
{
	float factor = std::clamp(blendFactor, 0.0f, 1.0f);
	m_blendScale = scale * factor + m_scale * (1.0f - factor);
}

void GltfNode::BlendTranslation(glm::vec3 translation, float blendFactor)
{
	float factor = std::clamp(blendFactor, 0.0f, 1.0f);
	m_blendTranslation = translation * factor + m_translation * (1.0f - factor);
}

void GltfNode::BlendRotation(glm::quat rotation, float blendFactor)
{
	float factor = std::clamp(blendFactor, 0.0f, 1.0f);
	m_blendRotation = slerp(m_rotation, rotation, factor);
}

void GltfNode::CalculateLocalTrsMatrix()
{
	glm::mat4 sMatrix = scale(glm::mat4(1.0f), m_blendScale);
	glm::mat4 rMatrix = mat4_cast(m_blendRotation);
	glm::mat4 tMatrix = translate(glm::mat4(1.0f), m_blendTranslation);

	m_localTrsMatrix = tMatrix * rMatrix * sMatrix;
}

void GltfNode::CalculateNodeMatrix(glm::mat4 parentNodeMatrix)
{
	m_nodeMatrix = parentNodeMatrix * m_localTrsMatrix;
}

glm::mat4 GltfNode::GetNodeMatrix()
{
	return m_nodeMatrix;
}

void GltfNode::PrintTree() {
	//Logger::Log(1, "%s: ---- tree ----\n", __FUNCTION__);
	//Logger::Log(1, "%s: parent : %i (%s)\n", __FUNCTION__, mNodeNum, mNodeName.c_str());
	for (const auto& childNode : m_childNodes) {
		GltfNode::PrintNodes(childNode, 1);
	}
	//Logger::Log(1, "%s: -- end tree --\n", __FUNCTION__);
}

void GltfNode::PrintNodes(std::shared_ptr<GltfNode> node, int indent)
{
	std::string indendString = "";
	for (int i = 0; i < indent; ++i)
	{
		indendString += " ";
	}
	indendString += "-";
	//Logger::Log(1, "%s: %s child : %i (%s)\n", __FUNCTION__,
//		indendString.c_str(), node->mNodeNum, node->mNodeName.c_str());

	for (const auto& childNode : node->m_childNodes)
	{
		PrintNodes(childNode, indent + 1);
	}
}
