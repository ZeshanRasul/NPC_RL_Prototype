#pragma once

#include <vector>
#include <string>
#include <cstdint>

using ModelHandle = uint32_t;
using MaterialHandle = uint32_t;
constexpr uint32_t InvalidHandle = 0;

struct CpuSubmesh {
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t materialIndex = 0;
	uint32_t firstVertex = 0;
	std::vector<uint8_t> vertexData;
	std::vector<uint8_t> indexData;
	uint32_t vertexCount = 0;
	uint32_t vertexStride = 0;
	bool     index32 = false;
	float aabbMin[3] = { 0.0f, 0.0f, 0.0f };
	float aabbMax[3] = { 0.0f, 0.0f, 0.0f };
};

struct CpuStaticMesh {
	std::vector<CpuSubmesh> submeshes;
};

struct CpuMaterial {
	//uint32_t baseColorTexture = 0;
	float baseColorFactor[4] = { 1.0f,1.0f,1.0f,1.0f };
	float metallic = 0.0f;
	float roughness = 1.0f;
};

struct CpuStaticModel {
	std::vector<CpuStaticMesh> meshes;
	std::vector<MaterialHandle> materials;
	std::vector<CpuMaterial> materialsData;
};


