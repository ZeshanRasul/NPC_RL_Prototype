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

	ok = gltfLoader.LoadASCIIFromFile(&model, &loaderErrors, &loaderWarnings, gltfPath);

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

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices32;
	std::vector<uint16_t> indices16;
	outCpuModel.materials.clear();
	outMaterials.clear();

	if (model.meshes.empty()) return false;

	for (const auto& mesh : model.meshes)
	{
		uint32_t baseVertex = 0;
		uint32_t baseIndex = 0;

		CpuStaticMesh cpuMesh;

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

		const tinygltf::Mesh& gltfMesh = mesh;
		for (const auto& primitive : gltfMesh.primitives)
		{
			if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
				Logger::Log(1, "%s: unsupported primitive mode %d\n", __FUNCTION__, primitive.mode);
				continue;
			}

			int posAccessorIndex = primitive.attributes.at("POSITION");
			int normAccessorIndex = primitive.attributes.at("NORMAL");
			int texCoordAccessorIndex = primitive.attributes.at("TEXCOORD_0");

			const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
			const tinygltf::Accessor& normAccessor = model.accessors[normAccessorIndex];
			const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordAccessorIndex];

			const uint8_t* posData = AccessorPointer(model, posAccessor);
			const uint8_t* normData = AccessorPointer(model, normAccessor);
			const uint8_t* texCoordData = AccessorPointer(model, texCoordAccessor);

			if (!posData || !normData || !texCoordData) {
				Logger::Log(1, "%s: missing vertex data for mesh '%s'\n", __FUNCTION__, gltfMesh.name.c_str());
				continue;
			}

			size_t vertexCount = posAccessor.count;
			vertices.resize(vertices.size() + vertexCount);

			for (size_t i = 0; i < vertexCount; ++i)
			{
				Vertex& vtx = vertices.back();
				vtx.px = reinterpret_cast<const float*>(posData)[i * 3 + 0];
				vtx.py = reinterpret_cast<const float*>(posData)[i * 3 + 1];
				vtx.pz = reinterpret_cast<const float*>(posData)[i * 3 + 2];
				vtx.nx = reinterpret_cast<const float*>(normData)[i * 3 + 0];
				vtx.ny = reinterpret_cast<const float*>(normData)[i * 3 + 1];
				vtx.nz = reinterpret_cast<const float*>(normData)[i * 3 + 2];
				vtx.u = reinterpret_cast<const float*>(texCoordData)[i * 2 + 0];
				vtx.v = reinterpret_cast<const float*>(texCoordData)[i * 2 + 1];
			}

			uint32_t firstIndex = (uint32_t)(outCpuModel.meshes.data()->index32 ? indices32.size() : indices16.size());
			uint32_t indexCount = 0;

			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const uint8_t* indexData = AccessorPointer(model, indexAccessor);

				if (!indexData) {
					Logger::Log(1, "%s: missing index data for mesh '%s'\n", __FUNCTION__, gltfMesh.name.c_str());
					continue;
				}

				indexCount = indexAccessor.count;

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					indices32.resize(indices32.size() + indexCount);
					for (size_t i = 0; i < indexCount; ++i)
					{
						indices32.back() = reinterpret_cast<const uint32_t*>(indexData)[i];
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					indices16.resize(indices16.size() + indexCount);
					for (size_t i = 0; i < indexCount; ++i)
					{
						indices16.back() = reinterpret_cast<const uint16_t*>(indexData)[i];
					}
				}
			}
			else
			{
				indexCount = static_cast<uint32_t>(vertexCount);
				for (uint32_t i = 0; i < indexCount; ++i) {
					if (outCpuModel.meshes.data()->index32) {
						indices32.push_back(firstIndex + i);
					}
					else {
						indices16.push_back(static_cast<uint16_t>(firstIndex + i));
					}
				}
			}


			CpuSubmesh sm{};
			sm.firstIndex = firstIndex;
			sm.indexCount = indexCount;
			sm.materialIndex = (primitive.material >= 0) ? uint32_t(primitive.material) : 0;
			cpuMesh.submeshes.push_back(sm);

			baseVertex = static_cast<uint32_t>(vertexCount);
			baseIndex += indexCount;
		}

		cpuMesh.vertexCount = (uint32_t)vertices.size();
		cpuMesh.vertexStride = sizeof(Vertex);
		cpuMesh.vertexData.resize(cpuMesh.vertexStride * vertices.size());
		std::memcpy(cpuMesh.vertexData.data(), vertices.data(),
			cpuMesh.vertexData.size());

		cpuMesh.index32 = !indices16.empty();
		cpuMesh.indexCount = cpuMesh.index32 ? (uint32_t)indices32.size() : (uint16_t)indices16.size();
		size_t indexBytes = cpuMesh.index32 ? indices32.size() * sizeof(uint32_t) : indices16.size() * sizeof(uint16_t);

		cpuMesh.indexData.resize(indexBytes);
		if (cpuMesh.index32) {
			std::memcpy(cpuMesh.indexData.data(), indices32.data(), indexBytes);
		}
		else {
			std::memcpy(cpuMesh.indexData.data(), indices16.data(), indexBytes);
		}

		if (!cpuMesh.submeshes.empty())
		{
			// Calculate AABB
			glm::vec3 aabbMin(FLT_MAX);
			glm::vec3 aabbMax(-FLT_MAX);
			for (const auto& v : vertices)
			{
				aabbMin.x = std::min(aabbMin.x, v.px);
				aabbMin.y = std::min(aabbMin.y, v.py);
				aabbMin.z = std::min(aabbMin.z, v.pz);
				aabbMax.x = std::max(aabbMax.x, v.px);
				aabbMax.y = std::max(aabbMax.y, v.py);
				aabbMax.z = std::max(aabbMax.z, v.pz);
			}
			std::memcpy(cpuMesh.aabbMin, &aabbMin[0], sizeof(float) * 3);
			std::memcpy(cpuMesh.aabbMax, &aabbMax[0], sizeof(float) * 3);
		}

		outCpuModel.meshes.push_back(cpuMesh);
	}

	return true;
}