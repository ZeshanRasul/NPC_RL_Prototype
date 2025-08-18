#pragma once

#include "glm/glm.hpp"
#include "EnTT/entt.hpp"

#include "OpenGL/UniformBuffer.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StaticModelRendererComponent.h"
#include "Shader.h"
#include "Engine/Render/RenderBackend.h"
#include "Engine/Render/GpuUploader.h"
#include "Logger.h"


struct PerObjectUBO
{
	std::vector<glm::mat4> matrices;
	bool shadowPass;
};

struct Pipelines {
	GpuPipelineHandle staticPbr = 0;
};

inline Shader* PickShader(const StaticModelRendererComponent& comp, bool shadowPass)
{
	return shadowPass ? (comp.shadowShader ? comp.shadowShader : comp.shader) : comp.shader;
}

inline void RenderStaticModels(entt::registry& reg, RenderBackend& rb,
	GpuUploader& up, const Pipelines& pipes)
{
	std::vector<DrawItem> draws;
	draws.reserve(1024);

	auto view = reg.view<StaticModelRendererComponent>();

	for (auto [e, comp] : view.each())
	{
		up.EnsureResident(comp.model);

		const GpuModel* gpuModel = up.Model(comp.model);

		if (!gpuModel || gpuModel->meshes.empty())
		{
			Logger::Log(1, "RenderSystem: Model %u not found or has no meshes\n", comp.model);
			continue;
		}

		for (size_t i = 0; i < gpuModel->meshes.size(); ++i)
		{
			const auto& mesh = gpuModel->meshes[i];
			

			for (const auto& submesh : mesh.submeshes)
			{
				DrawItem item{};
				item.pipeline = pipes.staticPbr;
				item.vao = submesh.vao;
				item.vertexBuffer = submesh.vertexBuffer;
				item.indexBuffer = submesh.indexBuffer;
				item.indexType = submesh.indexType;

				item.firstIndex = submesh.firstIndex;
				item.indexCount = submesh.indexCount;

				up.EnsureMatResident(submesh.material);
				item.materialId = up.MatId(submesh.material);
				item.materialHandle = submesh.material;
				//Logger::Log(1, "RenderStaticModels: submesh matHandle=%u\n", item.materialHandle);
				draws.push_back(item);
			}

		}
	}

	rb.BeginFrame();

	if (!draws.empty())
	{
		rb.Submit(draws.data(), static_cast<uint32_t>(draws.size()));
	}
	
	rb.EndFrame();

}