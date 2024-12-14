#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tiny_gltf.h>

#include "GltfNode.h"
#include "GltfAnimationChannel.h"

class GltfAnimationClip
{
public:
	GltfAnimationClip(std::string name);
	void AddChannel(std::shared_ptr<tinygltf::Model> model, tinygltf::Animation anim,
	                tinygltf::AnimationChannel channel);
	void SetAnimationFrame(std::vector<std::shared_ptr<GltfNode>> nodes, std::vector<bool> additiveMask, float time);
	void BlendAnimationFrame(std::vector<std::shared_ptr<GltfNode>> nodes, std::vector<bool> additiveMask, float time,
	                         float blendFactor);
	float GetClipEndTime();
	std::string GetClipName();

private:
	std::vector<std::shared_ptr<GltfAnimationChannel>> m_animationChannels{};

	std::string m_clipName;
};
