#pragma once

#include <unordered_map>
#include "glm/glm.hpp"

#include "RenderBackend.h"
#include "Engine/Asset/AssetManager.h"

struct GpuMeshBuffer {
	GpuBufferHandle vertexBuffer = 0;
	GpuBufferHandle indexBuffer = 0;
	IndexType indexType = IndexType::U32;
	int indexCount = 0;
	int vertexStride = 0;
	uint32_t vao = 0;
};

struct GpuModel {
	std::vector<GpuMeshBuffer> meshes;
	std::vector<glm::mat4> meshLocalTransforms;
};

class GpuUploader {
public:
	GpuUploader(RenderBackend* backend, AssetManager* assetManager)
		: m_backend(backend), m_assetManager(assetManager) {
	}

	void EnsureResident(ModelHandle handle);

	const GpuModel* Model(ModelHandle handle) const {
		auto it = m_modelBuffers.find(handle);
		return (it == m_modelBuffers.end()) ? nullptr : &it->second;
	}

private:
	RenderBackend* m_backend;
	const AssetManager* m_assetManager;
	std::unordered_map<ModelHandle, GpuModel> m_modelBuffers;
};




