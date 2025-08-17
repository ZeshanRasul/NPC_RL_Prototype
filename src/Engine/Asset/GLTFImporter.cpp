#include <tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>
#include <cstring>

#include "GLTFImporter.h"
#include "Logger.h"

static inline const uint8_t* AccessorPointer(const tinygltf::Model& model, const tinygltf::Accessor& acc)
{
	if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
		return nullptr; // Invalid buffer view
	}
	const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
	if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
		return nullptr; // Invalid buffer
	}
	const tinygltf::Buffer& buf = model.buffers[bv.buffer];
	return buf.data.data() + bv.byteOffset + acc.byteOffset;
}

bool ImportStaticModelFromGltf(const std::string& gltfPath, CpuStaticModel& outCpuModel, std::vector<CpuMaterial>& outMaterials)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF gltfLoader;
	std::string loaderErrors;
	std::string loaderWarnings;
	bool ok = false;

	ok = gltfLoader.LoadBinaryFromFile(&model, &loaderErrors, &loaderWarnings, gltfPath);

	if (!loaderWarnings.empty())
	{
		Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
			loaderWarnings.c_str());
	}

	if (!loaderErrors.empty())
	{
		Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
			loaderErrors.c_str());
	}

	if (!ok)
	{
		Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
			gltfPath.c_str());
		return false;
	}

	struct Vertex { float px, py, pz, nx, ny, nz, u, v; };

	
	outCpuModel.materials.clear();
	outCpuModel.meshes.clear();
	outMaterials.clear();

	if (model.meshes.empty()) return false;

	for (size_t i = 0; i < model.materials.size(); ++i)
	{
		CpuMaterial cpuMaterial{};

		if (model.materials[i].pbrMetallicRoughness.baseColorFactor.size() == 4)
		{
			for (int k = 0; k < 4; ++k)
			{
				cpuMaterial.baseColorFactor[k] = (float)model.materials[i].pbrMetallicRoughness.baseColorFactor[k];
			}
		}

		if (model.materials[i].pbrMetallicRoughness.metallicFactor >= 0.0f)
		{
			cpuMaterial.metallic = model.materials[i].pbrMetallicRoughness.metallicFactor;
		}

		if (model.materials[i].pbrMetallicRoughness.roughnessFactor >= 0.0f)
		{
			cpuMaterial.roughness = model.materials[i].pbrMetallicRoughness.roughnessFactor;
		}

		outMaterials.push_back(cpuMaterial);
	}

	for (const auto& mesh : model.meshes)
	{
		CpuStaticMesh cpuMesh;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices32;
		std::vector<uint16_t> indices16;

	


		const tinygltf::Mesh& gltfMesh = mesh;
		uint32_t baseVertex = 0;
		uint32_t baseIndex = 0;

		for (const auto& prim : gltfMesh.primitives) {
			// POSITION
			auto itPos = prim.attributes.find("POSITION");
			if (itPos == prim.attributes.end()) continue;
			const auto& aPos = model.accessors[itPos->second];
			const auto* pPos = (const float*)AccessorPointer(model, aPos);

			// NORMAL (optional)
			const float* pNrm = nullptr;
			auto itN = prim.attributes.find("NORMAL");
			if (itN != prim.attributes.end())
				pNrm = (const float*)AccessorPointer(model, model.accessors[itN->second]);

			// TEXCOORD_0 (optional)
			const float* pUV = nullptr;
			auto itUV = prim.attributes.find("TEXCOORD_0");
			if (itUV != prim.attributes.end())
				pUV = (const float*)AccessorPointer(model, model.accessors[itUV->second]);

			// Append vertices
			for (size_t v = 0; v < aPos.count; ++v) {
				Vertex vv{};
				vv.px = pPos[3 * v + 0]; vv.py = pPos[3 * v + 1]; vv.pz = pPos[3 * v + 2];
				if (pNrm) { vv.nx = pNrm[3 * v + 0]; vv.ny = pNrm[3 * v + 1]; vv.nz = pNrm[3 * v + 2]; }
				else { vv.nx = 0; vv.ny = 1; vv.nz = 0; }
				if (pUV) { vv.u = pUV[2 * v + 0]; vv.v = pUV[2 * v + 1]; }
				else { vv.u = 0; vv.v = 0; }
				vertices.push_back(vv);
			}

			// Indices
			uint32_t indexCount = 0;

			if (prim.indices >= 0) {
				const auto& aIdx = model.accessors[prim.indices];
				const uint8_t* pIdx = AccessorPointer(model, aIdx);

				if (aIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* p = (const uint16_t*)pIdx;
					for (size_t i = 0; i < aIdx.count; ++i) indices16.push_back((uint16_t)(baseVertex + p[i]));
					indexCount = (uint32_t)aIdx.count;
					cpuMesh.index32 = false;
				}
				else {
					cpuMesh.index32 = true;
					if (aIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
						const uint32_t* p = (const uint32_t*)pIdx;
						for (size_t i = 0; i < aIdx.count; ++i) indices32.push_back(baseVertex + p[i]);
					}
					else if (aIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
						const uint8_t* p = (const uint8_t*)pIdx;
						for (size_t i = 0; i < aIdx.count; ++i) indices32.push_back(baseVertex + p[i]);
					}
					else {
						return false;
					}
					indexCount = (uint32_t)aIdx.count;
				}
			}
			else {
				cpuMesh.index32 = true;
				for (uint32_t i = 0; i < (uint32_t)aPos.count; ++i) indices32.push_back(baseVertex + i);
				indexCount = (uint32_t)aPos.count;
			}
		
			uint32_t firstIndex = (uint32_t)(cpuMesh.index32 ? indices32.size() : indices16.size());

			CpuSubmesh sm{};
			sm.indexCount = cpuMesh.index32 ? (uint32_t)(indices32.size() - firstIndex) : (uint32_t)(indices16.size() - firstIndex);
			sm.firstIndex = firstIndex;
			sm.materialIndex = (prim.material >= 0) ? uint32_t(prim.material) : 0;
			cpuMesh.submeshes.push_back(sm);

			baseVertex += static_cast<uint32_t>(aPos.count);
			baseIndex += indexCount;
		}

		cpuMesh.vertexCount = (uint32_t)vertices.size();
		cpuMesh.vertexStride = sizeof(Vertex);
		cpuMesh.vertexData.resize(cpuMesh.vertexStride * vertices.size());
		std::memcpy(cpuMesh.vertexData.data(), vertices.data(),
			cpuMesh.vertexData.size());

		cpuMesh.index32 = indices16.empty();
		cpuMesh.indexCount = cpuMesh.index32 ? (uint32_t)indices32.size() : (uint32_t)indices16.size();
		size_t indexBytes = cpuMesh.index32 ? indices32.size() * sizeof(uint32_t) : indices16.size() * sizeof(uint16_t);

		cpuMesh.indexData.resize(indexBytes);
		if (cpuMesh.index32) {
			std::memcpy(cpuMesh.indexData.data(), indices32.data(), indexBytes);
		}
		else {
			std::memcpy(cpuMesh.indexData.data(), indices16.data(), indexBytes);
		}

		//if (!cpuMesh.submeshes.empty())
		//{
		//	// Calculate AABB
		//	glm::vec3 aabbMin(FLT_MAX);
		//	glm::vec3 aabbMax(-FLT_MAX);
		//	for (const auto& v : vertices)
		//	{
		//		aabbMin.x = std::min(aabbMin.x, v.px);
		//		aabbMin.y = std::min(aabbMin.y, v.py);
		//		aabbMin.z = std::min(aabbMin.z, v.pz);
		//		aabbMax.x = std::max(aabbMax.x, v.px);
		//		aabbMax.y = std::max(aabbMax.y, v.py);
		//		aabbMax.z = std::max(aabbMax.z, v.pz);
		//	}
		//	std::memcpy(cpuMesh.aabbMin, &aabbMin[0], sizeof(float) * 3);
		//	std::memcpy(cpuMesh.aabbMax, &aabbMax[0], sizeof(float) * 3);
		//}

		outCpuModel.meshes.emplace_back(std::move(cpuMesh));
		Logger::Log(1, "%s: loaded %zu meshes with %zu vertices and %zu indices\n", __FUNCTION__,
			outCpuModel.meshes.size(), vertices.size(), indices32.size() + indices16.size());
	}

	return true;
}