#include "GltfAnimationClip.h"

GltfAnimationClip::GltfAnimationClip(std::string name) : m_clipName(name)
{
}

void GltfAnimationClip::AddChannel(std::shared_ptr<tinygltf::Model> model,
                                   tinygltf::Animation anim, tinygltf::AnimationChannel channel)
{
	auto chan = std::make_shared<GltfAnimationChannel>();
	chan->LoadChannelData(model, anim, channel);
	m_animationChannels.push_back(chan);
}

void GltfAnimationClip::SetAnimationFrame(std::vector<std::shared_ptr<GltfNode>> nodes, std::vector<bool> additiveMask,
                                          float time)
{
	for (auto& channel : m_animationChannels)
	{
		int targetNode = channel->GetTargetNode();
		/* do not change if masked out */
		if (additiveMask.at(targetNode))
		{
			switch (channel->GetTargetPath())
			{
			case ETargetPath::ROTATION:
				nodes.at(targetNode)->SetRotation(channel->GetRotation(time));
				break;
			case ETargetPath::TRANSLATION:
				nodes.at(targetNode)->SetTranslation(channel->GetTranslation(time));
				break;
			case ETargetPath::SCALE:
				nodes.at(targetNode)->SetScale(channel->GetScaling(time));
				break;
			}
		}
	}
	/* update all nodes in a single run */
	for (auto& node : nodes)
	{
		if (node)
		{
			node->CalculateLocalTrsMatrix();
		}
	}
}

void GltfAnimationClip::BlendAnimationFrame(std::vector<std::shared_ptr<GltfNode>> nodes,
                                            std::vector<bool> additiveMask,
                                            float time, float blendFactor)
{
	for (auto& channel : m_animationChannels)
	{
		int targetNode = channel->GetTargetNode();
		if (additiveMask.at(targetNode))
		{
			switch (channel->GetTargetPath())
			{
			case ETargetPath::ROTATION:
				nodes.at(targetNode)->BlendRotation(channel->GetRotation(time), blendFactor);
				break;
			case ETargetPath::TRANSLATION:
				nodes.at(targetNode)->BlendTranslation(channel->GetTranslation(time), blendFactor);
				break;
			case ETargetPath::SCALE:
				nodes.at(targetNode)->BlendScale(channel->GetScaling(time), blendFactor);
				break;
			}
		}
	}
	/* update all nodes in a single run */
	for (auto& node : nodes)
	{
		if (node)
		{
			node->CalculateLocalTrsMatrix();
		}
	}
}

float GltfAnimationClip::GetClipEndTime()
{
	return m_animationChannels.at(0)->GetMaxTime();
}

std::string GltfAnimationClip::GetClipName()
{
	return m_clipName;
}
