#include "GltfAnimationChannel.h"

void GltfAnimationChannel::LoadChannelData(std::shared_ptr<tinygltf::Model> model, tinygltf::Animation anim,
                                           tinygltf::AnimationChannel channel)
{
	m_targetNode = channel.target_node;

	const tinygltf::Accessor& inputAccessor = model->accessors.at(anim.samplers.at(channel.sampler).input);
	const tinygltf::BufferView& inputBufferView = model->bufferViews.at(inputAccessor.bufferView);
	const tinygltf::Buffer& inputBuffer = model->buffers.at(inputBufferView.buffer);

	std::vector<float> timings;
	timings.resize(inputAccessor.count);

	std::memcpy(timings.data(), &inputBuffer.data.at(0) + inputBufferView.byteOffset, inputBufferView.byteLength);
	SetTimings(timings);

	const tinygltf::AnimationSampler sampler = anim.samplers.at(channel.sampler);
	if (sampler.interpolation.compare("STEP") == 0)
	{
		m_interType = EInterpolationType::STEP;
	}
	else if (sampler.interpolation.compare("LINEAR") == 0)
	{
		m_interType = EInterpolationType::LINEAR;
	}
	else
	{
		m_interType = EInterpolationType::CUBICSPLINE;
	}

	const tinygltf::Accessor& outputAccessor = model->accessors.at(anim.samplers.at(channel.sampler).output);
	const tinygltf::BufferView& outputBufferView = model->bufferViews.at(outputAccessor.bufferView);
	const tinygltf::Buffer& outputBuffer = model->buffers.at(outputBufferView.buffer);

	if (channel.target_path.compare("rotation") == 0)
	{
		m_targetPath = ETargetPath::ROTATION;
		std::vector<glm::quat> rotations;
		rotations.resize(outputAccessor.count);

		std::memcpy(rotations.data(), &outputBuffer.data.at(0) + outputBufferView.byteOffset,
		            outputBufferView.byteLength);
		SetRotations(rotations);
	}
	else if (channel.target_path.compare("translation") == 0)
	{
		m_targetPath = ETargetPath::TRANSLATION;
		std::vector<glm::vec3> translations;
		translations.resize(outputAccessor.count);

		std::memcpy(translations.data(), &outputBuffer.data.at(0) + outputBufferView.byteOffset,
		            outputBufferView.byteLength);
		SetTranslations(translations);
	}
	else
	{
		m_targetPath = ETargetPath::SCALE;
		std::vector<glm::vec3> scale;
		scale.resize(outputAccessor.count);

		std::memcpy(scale.data(), &outputBuffer.data.at(0) + outputBufferView.byteOffset, outputBufferView.byteLength);
		SetScalings(scale);
	}
}

void GltfAnimationChannel::SetTimings(std::vector<float> timings)
{
	m_timings = timings;
}

void GltfAnimationChannel::SetScalings(std::vector<glm::vec3> scalings)
{
	m_scaling = scalings;
}

void GltfAnimationChannel::SetTranslations(std::vector<glm::vec3> translations)
{
	m_translations = translations;
}

void GltfAnimationChannel::SetRotations(std::vector<glm::quat> rotations)
{
	m_rotations = rotations;
}

int GltfAnimationChannel::GetTargetNode()
{
	return m_targetNode;
}

ETargetPath GltfAnimationChannel::GetTargetPath()
{
	return m_targetPath;
}

glm::vec3 GltfAnimationChannel::GetScaling(float time)
{
	if (m_scaling.size() == 0)
	{
		return glm::vec3(1.0f);
	}

	if (time < m_timings.at(0))
	{
		return m_scaling.at(0);
	}
	if (time > m_timings.at(m_timings.size() - 1))
	{
		return m_scaling.at(m_scaling.size() - 1);
	}

	int prevTimeIndex = 0;
	int nextTimeIndex = 0;
	for (int i = 0; i < m_timings.size(); ++i)
	{
		if (m_timings.at(i) > time)
		{
			nextTimeIndex = i;
			break;
		}
		prevTimeIndex = i;
	}

	if (prevTimeIndex == nextTimeIndex)
	{
		return m_scaling.at(prevTimeIndex);
	}

	auto finalScale = glm::vec3(1.0f);
	switch (m_interType)
	{
	case EInterpolationType::STEP:
		finalScale = m_scaling.at(prevTimeIndex);
		break;
	case EInterpolationType::LINEAR:
		{
			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));

			glm::vec3 prevScale = m_scaling.at(prevTimeIndex);
			glm::vec3 nextScale = m_scaling.at(nextTimeIndex);

			finalScale = prevScale + interpolatedTime * (nextScale - prevScale);
		}
		break;
	case EInterpolationType::CUBICSPLINE:
		{
			/* scale tangents */
			float deltaTime = m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex);
			glm::vec3 prevTangent = deltaTime * m_scaling.at(prevTimeIndex * 3 + 2);
			glm::vec3 nextTangent = deltaTime * m_scaling.at(nextTimeIndex * 3);

			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));
			float interpolatedTimeSq = interpolatedTime * interpolatedTime;
			float interpolatedTimeCub = interpolatedTimeSq * interpolatedTime;

			glm::vec3 prevPoint = m_scaling.at(prevTimeIndex * 3 + 1);
			glm::vec3 nextPoint = m_scaling.at(nextTimeIndex * 3 + 1);

			finalScale =
				(2 * interpolatedTimeCub - 3 * interpolatedTimeSq + 1) * prevPoint +
				(interpolatedTimeCub - 2 * interpolatedTimeSq + interpolatedTime) * prevTangent +
				(-2 * interpolatedTimeCub + 3 * interpolatedTimeSq) * nextPoint +
				(interpolatedTimeCub - interpolatedTimeSq) * nextTangent;
		}
		break;
	}

	return finalScale;
}

glm::vec3 GltfAnimationChannel::GetTranslation(float time)
{
	if (m_translations.size() == 0)
	{
		return glm::vec3(0.0f);
	}

	if (time < m_timings.at(0))
	{
		return m_translations.at(0);
	}
	if (time > m_timings.at(m_timings.size() - 1))
	{
		return m_translations.at(m_translations.size() - 1);
	}

	int prevTimeIndex = 0;
	int nextTimeIndex = 0;
	for (int i = 0; i < m_timings.size(); ++i)
	{
		if (m_timings.at(i) > time)
		{
			nextTimeIndex = i;
			break;
		}
		prevTimeIndex = i;
	}

	if (prevTimeIndex == nextTimeIndex)
	{
		return m_translations.at(prevTimeIndex);
	}

	auto finalTranslate = glm::vec3(0.0f);
	switch (m_interType)
	{
	case EInterpolationType::STEP:
		finalTranslate = m_translations.at(prevTimeIndex);
		break;

	case EInterpolationType::LINEAR:
		{
			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));

			glm::vec3 prevTranslate = m_translations.at(prevTimeIndex);
			glm::vec3 nextTranslate = m_translations.at(nextTimeIndex);

			finalTranslate = prevTranslate + interpolatedTime *
				(nextTranslate - prevTranslate);
		}
		break;
	case EInterpolationType::CUBICSPLINE:
		{
			/* scale tangents */
			float deltaTime = m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex);
			glm::vec3 prevTangent = deltaTime * m_translations.at(prevTimeIndex * 3 + 2);
			glm::vec3 nextTangent = deltaTime * m_translations.at(nextTimeIndex * 3);

			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));
			float interpolatedTimeSq = interpolatedTime * interpolatedTime;
			float interpolatedTimeCub = interpolatedTimeSq * interpolatedTime;

			glm::vec3 prevPoint = m_translations.at(prevTimeIndex * 3 + 1);
			glm::vec3 nextPoint = m_translations.at(nextTimeIndex * 3 + 1);

			finalTranslate =
				(2 * interpolatedTimeCub - 3 * interpolatedTimeSq + 1) * prevPoint +
				(interpolatedTimeCub - 2 * interpolatedTimeSq + interpolatedTime) * prevTangent +
				(-2 * interpolatedTimeCub + 3 * interpolatedTimeSq) * nextPoint +
				(interpolatedTimeCub - interpolatedTimeSq) * nextTangent;
		}
		break;
	}

	return finalTranslate;
}

glm::quat GltfAnimationChannel::GetRotation(float time)
{
	if (m_rotations.size() == 0)
	{
		return glm::identity<glm::quat>();
	}

	if (time < m_timings.at(0))
	{
		return m_rotations.at(0);
	}
	if (time > m_timings.at(m_timings.size() - 1))
	{
		return m_rotations.at(m_rotations.size() - 1);
	}

	int prevTimeIndex = 0;
	int nextTimeIndex = 0;
	for (int i = 0; i < m_timings.size(); ++i)
	{
		if (m_timings.at(i) > time)
		{
			nextTimeIndex = i;
			break;
		}
		prevTimeIndex = i;
	}

	if (prevTimeIndex == nextTimeIndex)
	{
		return m_rotations.at(prevTimeIndex);
	}

	auto finalRotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	switch (m_interType)
	{
	case EInterpolationType::STEP:
		finalRotate = m_rotations.at(prevTimeIndex);
		break;
	case EInterpolationType::LINEAR:
		{
			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));

			glm::quat prevRotate = m_rotations.at(prevTimeIndex);
			glm::quat nextRotate = m_rotations.at(nextTimeIndex);

			finalRotate = slerp(prevRotate, nextRotate, interpolatedTime);
		}
		break;
	case EInterpolationType::CUBICSPLINE:
		{
			/* scale tangents */
			float deltaTime = m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex);
			glm::quat prevTangent = deltaTime * m_rotations.at(prevTimeIndex * 3 + 2);
			glm::quat nextTangent = deltaTime * m_rotations.at(nextTimeIndex * 3);

			float interpolatedTime = (time - m_timings.at(prevTimeIndex)) /
				(m_timings.at(nextTimeIndex) - m_timings.at(prevTimeIndex));
			float interpolatedTimeSq = interpolatedTime * interpolatedTime;
			float interpolatedTimeCub = interpolatedTimeSq * interpolatedTime;

			glm::quat prevPoint = m_rotations.at(prevTimeIndex * 3 + 1);
			glm::quat nextPoint = m_rotations.at(nextTimeIndex * 3 + 1);

			finalRotate =
				(2 * interpolatedTimeCub - 3 * interpolatedTimeSq + 1) * prevPoint +
				(interpolatedTimeCub - 2 * interpolatedTimeSq + interpolatedTime) * prevTangent +
				(-2 * interpolatedTimeCub + 3 * interpolatedTimeSq) * nextPoint +
				(interpolatedTimeCub - interpolatedTimeSq) * nextTangent;
		}
		break;
	}

	return finalRotate;
}

float GltfAnimationChannel::GetMaxTime()
{
	return m_timings.at(m_timings.size() - 1);
}
