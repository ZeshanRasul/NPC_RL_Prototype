#pragma once

#include "glm/glm.hpp"
#include "EnTT/entt.hpp"

#include "OpenGL/UniformBuffer.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StaticModelRendererComponent.h"
#include "Shader.h"
#include "Logger.h"


struct PerObjectUBO
{
	std::vector<glm::mat4> matrices;
	bool shadowPass;
};

inline Shader* PickShader(const StaticModelRendererComponent& comp, bool shadowPass)
{
	return shadowPass ? (comp.shadowShader ? comp.shadowShader : comp.shader) : comp.shader;
}

inline void RenderStaticMeshes(
	entt::registry& r,
	UniformBuffer& perObjectUBO,
	const glm::mat4& viewMatrix,
	const glm::mat4& projMatrix,
	const glm::mat4& lightSpaceMatrix,
	const glm::vec3& cameraPos,
	bool shadowPass)
{
	auto view = r.view<TransformComponent, StaticModelRendererComponent>();

	// Use view.each instead of range-based for
	view.each([&](auto e, TransformComponent& t, StaticModelRendererComponent& sm)
		{
			if (!sm.visible) return;
			if (shadowPass && !sm.castsShadows) return;
			if (!sm.model || !sm.shader) return;

			Shader* shader = PickShader(sm, shadowPass);
			if (!shader) return;
			shader->Use();

			PerObjectUBO ubo;
			ubo.matrices.push_back(viewMatrix);
			ubo.matrices.push_back(projMatrix);
			ubo.matrices.push_back(t.World);
			size_t uboBufferSize = 4 * sizeof(glm::mat4);
			perObjectUBO.Init(uboBufferSize);
			Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
				uboBufferSize);
			perObjectUBO.UploadUboData(ubo.matrices, 0);
		});
}