#pragma once

#include <unordered_map>
#include "glm/glm.hpp"

#include "RenderBackend.h"
#include "Engine/Asset/AssetManager.h"

struct GpuSubmeshBuffer {
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t firstVertex;
	MaterialHandle material;
	GpuMaterialId materialId = 0;
	uint32_t vao = 0;
	GpuBufferHandle vertexBuffer = 0;
	GpuBufferHandle indexBuffer = 0;
	IndexType indexType = IndexType::U32;
	int vertexStride = 0;
};

struct GpuMeshBuffer {
	std::vector<GpuSubmeshBuffer> submeshes;
};

struct GpuModel {
	std::vector<GpuMeshBuffer> meshes;
	std::vector<glm::mat4> meshLocalTransforms;
};

struct GpuMaterial {
	MaterialGpuDesc desc;
	GpuMaterialId id = 0;
};

class GpuUploader {
public:
	GpuUploader(RenderBackend* backend, AssetManager* assetManager)
		: m_backend(backend), m_assetManager(assetManager) {
	}

	void EnsureResident(ModelHandle ModelHandle);
	void EnsureMatResident(MaterialHandle matHandle);

	const GpuModel* Model(ModelHandle handle) const {
		auto it = m_modelBuffers.find(handle);
		return (it == m_modelBuffers.end()) ? nullptr : &it->second;
	}

	const GpuMaterial* Mat(MaterialHandle handle) const {
		auto it = m_materialBuffers.find(handle);
		return (it == m_materialBuffers.end()) ? nullptr : &it->second;
	}

	GpuMaterialId MatId(MaterialHandle handle) const {
		auto it = m_materialBuffers.find(handle);
		if (it != m_materialBuffers.end()) {
			return it->second.id;
		}
		return 0;
	}

private:
	RenderBackend* m_backend;
	const AssetManager* m_assetManager;
	std::unordered_map<ModelHandle, GpuModel> m_modelBuffers;
	std::unordered_map<MaterialHandle, GpuMaterial> m_materialBuffers;
};




