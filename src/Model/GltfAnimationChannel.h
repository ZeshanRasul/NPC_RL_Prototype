#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

enum class ETargetPath
{
	ROTATION,
	TRANSLATION,
	SCALE
};

enum class EInterpolationType
{
	STEP,
	LINEAR,
	CUBICSPLINE
};

class GltfAnimationChannel
{
public:
	void LoadChannelData(std::shared_ptr<tinygltf::Model> model, tinygltf::Animation anim,
	                     tinygltf::AnimationChannel channel);

	int GetTargetNode();
	ETargetPath GetTargetPath();

	glm::vec3 GetScaling(float time);
	glm::vec3 GetTranslation(float time);
	glm::quat GetRotation(float time);
	float GetMaxTime();

private:
	int m_targetNode = -1;
	ETargetPath m_targetPath = ETargetPath::ROTATION;
	EInterpolationType m_interType = EInterpolationType::LINEAR;

	std::vector<float> m_timings{};
	std::vector<glm::vec3> m_scaling{};
	std::vector<glm::vec3> m_translations{};
	std::vector<glm::quat> m_rotations{};

	void SetTimings(std::vector<float> timings);
	void SetScalings(std::vector<glm::vec3> scalings);
	void SetTranslations(std::vector<glm::vec3> translations);
	void SetRotations(std::vector<glm::quat> rotations);
};
