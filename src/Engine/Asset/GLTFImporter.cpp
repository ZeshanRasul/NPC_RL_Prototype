#include <tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>
#include <cfloat>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/gtc/type_ptr.hpp"
#include "GLTFImporter.h"

static SamplerDesc MapGltfSampler(const tinygltf::Sampler* s) {
	SamplerDesc out{};
	if (!s) return out;

	// Wrap
	auto toWrap = [](int w)->AddressModeGpu {
		switch (w) {
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:   return AddressModeGpu::ClampToEdge;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return AddressModeGpu::MirrorRepeat;
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
		default:                                    return AddressModeGpu::Repeat;
		}
		};
	out.wrapS = toWrap(s->wrapS);
	out.wrapT = toWrap(s->wrapT);

	auto setMinMip = [&](int minFilter) {
		switch (minFilter) {
		case TINYGLTF_TEXTURE_FILTER_NEAREST:                out.minFilter = TexFilterGpu::Nearest; out.mipFilter = MipFilterGpu::None;    break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:                 out.minFilter = TexFilterGpu::Linear;  out.mipFilter = MipFilterGpu::None;    break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: out.minFilter = TexFilterGpu::Nearest; out.mipFilter = MipFilterGpu::Nearest; break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:  out.minFilter = TexFilterGpu::Linear;  out.mipFilter = MipFilterGpu::Nearest; break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:  out.minFilter = TexFilterGpu::Nearest; out.mipFilter = MipFilterGpu::Linear;  break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:   out.minFilter = TexFilterGpu::Linear;  out.mipFilter = MipFilterGpu::Linear;  break;
		default: /* keep defaults */ break;
		}
		};
	setMinMip(s->minFilter);

	switch (s->magFilter) {
	case TINYGLTF_TEXTURE_FILTER_NEAREST: out.magFilter = TexFilterGpu::Nearest; break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:  out.magFilter = TexFilterGpu::Linear;  break;
	default: break;
	}

	return out;
}

static inline const uint8_t* BufferViewPtr(const tinygltf::Model& model,
	const tinygltf::BufferView& bv) {
	if (bv.buffer < 0 || bv.buffer >= (int)model.buffers.size()) return nullptr;
	const tinygltf::Buffer& buf = model.buffers[bv.buffer];
	if (bv.byteOffset >= buf.data.size()) return nullptr;
	return buf.data.data() + bv.byteOffset;
}

static inline const uint8_t* AccessorPtr(const tinygltf::Model& model,
	const tinygltf::Accessor& acc) {
	if (acc.bufferView < 0 || acc.bufferView >= (int)model.bufferViews.size()) return nullptr;
	const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
	const uint8_t* base = BufferViewPtr(model, bv);
	if (!base) return nullptr;
	size_t off = acc.byteOffset;
	if (off > bv.byteLength) return nullptr;
	return base + off;
}

static inline size_t ComponentSize(int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
	case TINYGLTF_COMPONENT_TYPE_BYTE:   return 1;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	case TINYGLTF_COMPONENT_TYPE_SHORT:  return 2;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
	case TINYGLTF_COMPONENT_TYPE_INT:
	case TINYGLTF_COMPONENT_TYPE_FLOAT:  return 4;
	default: return 0;
	}
}

static inline size_t NumComponents(int type /* TINYGLTF_TYPE_VEC3, etc. */) {
	switch (type) {
	case TINYGLTF_TYPE_SCALAR: return 1;
	case TINYGLTF_TYPE_VEC2:   return 2;
	case TINYGLTF_TYPE_VEC3:   return 3;
	case TINYGLTF_TYPE_VEC4:   return 4;
	case TINYGLTF_TYPE_MAT2:   return 4;
	case TINYGLTF_TYPE_MAT3:   return 9;
	case TINYGLTF_TYPE_MAT4:   return 16;
	default: return 0;
	}
}

// Read a float attribute stream (POSITION, NORMAL). Handles interleaved data.
static void ReadFloatAttrib3(const tinygltf::Model& model,
	const tinygltf::Accessor& acc,
	std::vector<glm::vec3>& out) {
	out.resize(acc.count);
	const uint8_t* src = AccessorPtr(model, acc);
	if (!src) { std::fill(out.begin(), out.end(), glm::vec3(0)); return; }

	const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
	const size_t compSize = ComponentSize(acc.componentType);
	const size_t ncomp = NumComponents(acc.type); // expect 3
	const size_t packed = compSize * ncomp;
	const size_t stride = bv.byteStride ? bv.byteStride : packed;

	if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || ncomp < 3) {
		std::fill(out.begin(), out.end(), glm::vec3(0));
		return;
	}
	for (size_t i = 0; i < acc.count; ++i) {
		const float* f = reinterpret_cast<const float*>(src + i * stride);
		out[i] = glm::vec3(f[0], f[1], f[2]);
	}
}

// Read a float vec2 attribute stream (TEXCOORD_n). Handles interleaved data.
static void ReadFloatAttrib2(const tinygltf::Model& model,
	const tinygltf::Accessor& acc,
	std::vector<glm::vec2>& out) {
	out.resize(acc.count);
	const uint8_t* src = AccessorPtr(model, acc);
	if (!src) { std::fill(out.begin(), out.end(), glm::vec2(0)); return; }

	const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
	const size_t compSize = ComponentSize(acc.componentType);
	const size_t ncomp = NumComponents(acc.type); // expect 2
	const size_t packed = compSize * ncomp;
	const size_t stride = bv.byteStride ? bv.byteStride : packed;

	if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || ncomp < 2) {
		std::fill(out.begin(), out.end(), glm::vec2(0));
		return;
	}
	for (size_t i = 0; i < acc.count; ++i) {
		const float* f = reinterpret_cast<const float*>(src + i * stride);
		out[i] = glm::vec2(f[0], f[1]);
	}
}

static PixelFormat ChoosePixelFormat(int components, int bits, int pixelType) {
	const bool isFloat = (pixelType == TINYGLTF_COMPONENT_TYPE_FLOAT);

	if (!isFloat) {
		if (bits == 8) {
			switch (components) {
			case 1: return PixelFormat::R8_UNORM;
			case 2: return PixelFormat::RG8_UNORM;
			case 3: return PixelFormat::RGB8_UNORM;
			case 4: return PixelFormat::RGBA8_UNORM;
			}
		}
		else if (bits == 16) {
			switch (components) {
			case 1: return PixelFormat::R16_UNORM;
			case 2: return PixelFormat::RG16_UNORM;
			case 3: return PixelFormat::RGB16_UNORM;
			case 4: return PixelFormat::RGBA16_UNORM;
			}
		}
	}
	else {
		if (bits == 16) {
			switch (components) {
			case 1: return PixelFormat::R16_FLOAT;
			case 2: return PixelFormat::RG16_FLOAT;
			case 3: return PixelFormat::RGB16_FLOAT;
			case 4: return PixelFormat::RGBA16_FLOAT;
			}
		}
		else if (bits == 32) {
			switch (components) {
			case 1: return PixelFormat::R32_FLOAT;
			case 2: return PixelFormat::RG32_FLOAT;
			case 3: return PixelFormat::RGB32_FLOAT;
			case 4: return PixelFormat::RGBA32_FLOAT;
			}
		}
	}

	return PixelFormat::RGB8_UNORM;
}

static ColorSpace ColorSpaceForSlot(const std::string& usageTag) {
	if (usageTag == "baseColor" || usageTag == "emissive")
		return ColorSpace::SRGB;
	return ColorSpace::Linear;
}

static CpuTexture& BuildCpuTextureFromGltfImage(tinygltf::Image& img,
	SamplerDesc& samp,
	const std::string& usageTag, CpuTexture& out)
{
	out.desc.width = static_cast<uint32_t>(img.width);
	out.desc.height = static_cast<uint32_t>(img.height);
	out.desc.sampler = samp;
	out.desc.colorSpace = ColorSpaceForSlot(usageTag);
	out.desc.format = PixelFormat::RGB8_UNORM;
	out.usage = TextureUsage::BaseColorSRGB;
	out.desc.mipLevels = 1;

	Logger::Log(1, "%s: image %s, size %ux%u, format %d, components %d, bits %d\n",
		__FUNCTION__, img.uri.c_str(), img.width, img.height,
		out.desc.format, img.component, img.bits);
	Logger::Log(1, "%s: image %s, width: %u height: %u, format %d, components %d, bits %d\n",
		__FUNCTION__, img.uri.c_str(), out.desc.width, out.desc.height,
		out.desc.format, img.component, img.bits);
	out.pixels.assign(img.image.begin(), img.image.end());
	return out;
}

glm::mat4 GetNodeTransform(const tinygltf::Node& node) {
	glm::mat4 matrix(1.0f);

	if (node.matrix.size() == 16) {
		matrix = glm::make_mat4(node.matrix.data());
	}
	else {
		if (node.translation.size() == 3) {
			matrix = glm::translate(matrix, glm::vec3(
				node.translation[0],
				node.translation[1],
				node.translation[2]));
		}
		if (node.rotation.size() == 4) {
			glm::quat q(node.rotation[3], node.rotation[0],
				node.rotation[1], node.rotation[2]);
			matrix *= glm::mat4_cast(q);
		}
		if (node.scale.size() == 3) {
			matrix = glm::scale(matrix, glm::vec3(
				node.scale[0],
				node.scale[1],
				node.scale[2]));
		}
	}
	return matrix;
}

void ProcessNode(const tinygltf::Model& model,
	int nodeIndex,
	const glm::mat4& parentTransform,
	CpuStaticModel& outModel,
	std::vector<CpuMaterial>& outMaterials,
	std::vector<CpuTexture>& outTextures)
{
	const tinygltf::Node& node = model.nodes[nodeIndex];
	glm::mat4 local = GetNodeTransform(node);
	glm::mat4 world = parentTransform * local;
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 norm;
		glm::vec2 uv;
		glm::vec2 uv1;
		glm::vec2 uv2;
	};

	std::vector<Vertex>         vertices;
	std::vector<uint32_t>    idx;
	size_t indexCount = 0;
	// indices
	uint32_t firstIndex = 0;
	uint64_t sum = 0;




	if (node.mesh >= 0) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		for (const auto& prim : mesh.primitives) {
			CpuStaticMesh cpuMesh;
			// Store transform per mesh
			cpuMesh.submeshes.reserve(mesh.primitives.size());
			for (const auto& prim : mesh.primitives) {
				// position (required)
				auto itPos = prim.attributes.find("POSITION");
				if (itPos == prim.attributes.end())
					continue;
				const tinygltf::Accessor& aPos = model.accessors[itPos->second];

				// prepare streams
				std::vector<glm::vec3> P, N;
				std::vector<glm::vec2> UV0, UV1, UV2;
				ReadFloatAttrib3(model, aPos, P);

				if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end())
					ReadFloatAttrib3(model, model.accessors[it->second], N);
				else
					N.assign(P.size(), glm::vec3(0, 1, 0));

				if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end())
					ReadFloatAttrib2(model, model.accessors[it->second], UV0);
				else
					UV0.assign(P.size(), glm::vec2(0, 0));

				if (auto it = prim.attributes.find("TEXCOORD_1"); it != prim.attributes.end())
					ReadFloatAttrib2(model, model.accessors[it->second], UV1);
				else
					UV1.assign(P.size(), glm::vec2(0, 0));

				if (auto it = prim.attributes.find("TEXCOORD_2"); it != prim.attributes.end())
					ReadFloatAttrib2(model, model.accessors[it->second], UV2);
				else
					UV2.assign(P.size(), glm::vec2(0, 0));

				const uint32_t baseV = (uint32_t)vertices.size();
				vertices.reserve(vertices.size() + P.size());

				for (size_t i = 0; i < P.size(); ++i) {
					Vertex v{};
					v.pos = P[i];
					v.norm = i < N.size() ? N[i] : glm::vec3(0, 1, 0);
					v.uv = i < UV0.size() ? UV0[i] : glm::vec2(0);
					v.uv1 = i < UV1.size() ? UV1[i] : glm::vec2(0);
					v.uv2 = i < UV2.size() ? UV2[i] : glm::vec2(0);
					vertices.push_back(v);
				}

				uint32_t added = 0;
				std::vector<uint32_t> localIdx;
				if (prim.indices >= 0) {
					const tinygltf::Accessor& indexAccessor = model.accessors[prim.indices];
					const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];



					// Pointer to the actual index data
					const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

					// Loop through and extract indices based on the component type
					for (size_t i = 0; i < indexAccessor.count; ++i) {
						switch (indexAccessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							idx.push_back(static_cast<unsigned int>(reinterpret_cast<const uint8_t*>(dataPtr)[i]));
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							idx.push_back(static_cast<unsigned int>(reinterpret_cast<const uint16_t*>(dataPtr)[i]));
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							idx.push_back(reinterpret_cast<const uint32_t*>(dataPtr)[i]);
							break;
						}
						default:
							Logger::Log(1, " << indexAccessor.componentType, %zu", indexAccessor.componentType);
							break;
						}
					}
					added = (uint32_t)idx.size() - firstIndex;
				}
				else {
					localIdx.resize(P.size());
					//std::iota(localIdx.begin(), localIdx.end(), 0u);
				}

				CpuSubmesh sm{};
				sm.firstIndex = firstIndex;
				firstIndex += added;
				sm.indexCount = idx.size() - sm.firstIndex; // or (uint32_t)idx.size() - firstIndex
				sm.materialIndex = (prim.material >= 0) ? (uint32_t)prim.material : 0;
				sm.firstVertex = baseV; // optional
				sm.transform = world;
				uint32_t maxIdx = 0;
				for (auto v : idx) maxIdx = std::max(maxIdx, v);

				if (maxIdx >= vertices.size()) {
					Logger::Log(1, "[DEBUG] mesh %zu: maxIdx=%u >= vertCount=%zu  <-- baseV/VAO issue\n",
						mesh, maxIdx, vertices.size());
				}

				for (auto& i : localIdx) i += baseV;

				sm.firstVertex = baseV;
				//sm.firstIndex = (uint32_t)idx.size();
				//sm.indexCount = (uint32_t)localIdx.size();

		/*		for (uint32_t i = firstIndex; i < added; i++)
				{
					sm.indexData.push_back(idx[i]);
				}*/

				Logger::Log(1, "ImportStaticModelFromGltf: mesh %zu name='%s' prims=%zu verts=%zu idx=%zu\n",
					mesh,
					mesh.name.c_str(),
					mesh.primitives.size(),
					vertices.size(),
					idx.size());

				sm.vertexCount = (uint32_t)vertices.size() - baseV;
				sm.vertexStride = sizeof(Vertex);
				sm.vertexData.resize(sm.vertexStride * vertices.size());
				if (!vertices.empty())
					std::memcpy(sm.vertexData.data(), vertices.data(), sm.vertexData.size());

				//sm.indexCount = (uint32_t)indexCount;
				maxIdx = 0;
				for (uint32_t v : idx) maxIdx = std::max(maxIdx, v);
				const bool fitsU16 = (maxIdx <= 0xFFFF);
				sm.index32 = !fitsU16;


				// TODO indexCount += sm.indexData.size();



				for (const auto& sm : cpuMesh.submeshes) sum += sm.indexData.size();
				if (sum != idx.size()) {
					Logger::Log(1, "[DEBUG] mesh %zu: sum(submesh.indexCount)=%llu != idx.size()=%zu  <-- firstIndex/indexCount bookkeeping\n",
						mesh, (unsigned long long)sum, idx.size());
				}

				if (fitsU16) {
					std::vector<uint16_t> idx16(idx.begin(), idx.end());
					sm.indexData.resize(idx16.size() * sizeof(uint16_t));
					std::memcpy(sm.indexData.data(), idx16.data(), sm.indexData.size());
				}
				else {
					sm.indexData.resize(idx.size() * sizeof(uint32_t));
					std::memcpy(sm.indexData.data(), idx.data(), sm.indexData.size());
				}

				cpuMesh.submeshes.push_back(sm);

			}
			outModel.meshes.emplace_back(std::move(cpuMesh));
			Logger::Log(1, "%s: loaded %zu meshes with %u vertices and %u indices\n",
				__FUNCTION__,
				outModel.meshes.size(),
				(unsigned)vertices.size(),
				(unsigned)idx.size());
	}
}

// Recurse into children
for (int childIndex : node.children) {
	ProcessNode(model, childIndex, world, outModel, outMaterials, outTextures);
}
}

CpuStaticModel ImportModel(const std::string& gltfPath, std::vector<CpuMaterial>& outMaterials, std::vector<CpuTexture>& outTextures) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);
	if (!ok) ok = loader.LoadBinaryFromFile(&model, &err, &warn, gltfPath);

	if (!warn.empty())
		Logger::Log(1, "%s warnings:\n%s\n", __FUNCTION__, warn.c_str());
	if (!ok) {
		Logger::Log(1, "%s errors:\n%s\n", __FUNCTION__, err.c_str());
		Logger::Log(1, "%s: failed to load %s\n", __FUNCTION__, gltfPath.c_str());
	}
	CpuStaticModel result;

	int sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	const tinygltf::Scene& scene = model.scenes[sceneIndex];
	for (const auto& m : model.materials) {
		CpuMaterial cm{};
		CpuTexture baseColorTex{};
		CpuTexture metRoughTex{};
		CpuTexture normalTex{};
		if (m.pbrMetallicRoughness.baseColorFactor.size() == 4) {
			for (int k = 0; k < 4; ++k)
				cm.baseColorFactor[k] = (float)m.pbrMetallicRoughness.baseColorFactor[k];
		}
		else {
			cm.baseColorFactor[0] = cm.baseColorFactor[1] =
				cm.baseColorFactor[2] = cm.baseColorFactor[3] = 1.0f;
		}
		if (m.pbrMetallicRoughness.metallicFactor >= 0.0f) cm.metallic = (float)m.pbrMetallicRoughness.metallicFactor;
		if (m.pbrMetallicRoughness.roughnessFactor >= 0.0f) cm.roughness = (float)m.pbrMetallicRoughness.roughnessFactor;

		if (m.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			uint8_t texIndex;
			texIndex = m.pbrMetallicRoughness.baseColorTexture.index;

			const tinygltf::Texture& gltfTex = model.textures[texIndex];
			tinygltf::Image gltfImg = model.images[gltfTex.source];

			SamplerDesc sampler{};
			if (gltfTex.sampler >= 0 && gltfTex.sampler < (int)model.samplers.size()) {
				sampler = MapGltfSampler(&model.samplers[gltfTex.sampler]);
			}
			else {
				sampler = MapGltfSampler(nullptr); // or a default SamplerDesc
			}

			baseColorTex = BuildCpuTextureFromGltfImage(gltfImg, sampler, "baseColor", baseColorTex);

			//cm.baseColor = &baseColorTex;
			Logger::Log(1, "%s: baseColor texture %u for material %s\n",
				__FUNCTION__, texIndex, m.name.c_str());
			Logger::Log(1, "%s: Mat baseColor texture %u for material %s\n",
				__FUNCTION__, baseColorTex.desc.width, m.name.c_str());

			outTextures.push_back(std::move(baseColorTex));
			cm.baseColorTexIdx = (int)outTextures.size() - 1;

			Logger::Log(1, "%s: baseColorTexIdx %d for material %s\n",
				__FUNCTION__, cm.baseColorTexIdx, m.name.c_str());
		}

		Logger::Log(1, "%s: material %s, baseColorFactor: %.2f, %.2f, %.2f, %.2f\n",
			__FUNCTION__, m.name.c_str(),
			cm.baseColorFactor[0], cm.baseColorFactor[1],
			cm.baseColorFactor[2], cm.baseColorFactor[3]);
		Logger::Log(1, "%s: material %s, metallic: %.2f, roughness: %.2f\n", __FUNCTION__,
			m.name.c_str(), cm.metallic, cm.roughness);

		Logger::Log(1, "%s: material %s, baseColorH: %u\n", __FUNCTION__,
			m.name.c_str(), cm.baseColorH);
		outMaterials.push_back(cm);
	}
	glm::mat4 identity(1.0f);
	for (int nodeIndex : scene.nodes) {
		ProcessNode(model, nodeIndex, identity, result, outMaterials, outTextures);
	}

	return result;
}





struct VertexLayoutKey {
	bool hasNormal = false;
	bool hasTangent = false;
	uint8_t texCoordCount = 0;
	bool hasColor0 = false;
};




bool ImportStaticModelFromGltf(const std::string& gltfPath,
	CpuStaticModel& outCpuModel,
	std::vector<CpuMaterial>& outMaterials,
	std::vector<CpuTexture>& outTextures)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);
	if (!ok) ok = loader.LoadBinaryFromFile(&model, &err, &warn, gltfPath);

	if (!warn.empty())
		Logger::Log(1, "%s warnings:\n%s\n", __FUNCTION__, warn.c_str());
	if (!ok) {
		Logger::Log(1, "%s errors:\n%s\n", __FUNCTION__, err.c_str());
		Logger::Log(1, "%s: failed to load %s\n", __FUNCTION__, gltfPath.c_str());
		return false;
	}

	outCpuModel.meshes.clear();
	outCpuModel.materials.clear();
	outMaterials.clear();
	outTextures.clear();

	outCpuModel.meshes.reserve(model.meshes.size());
	outCpuModel.materials.reserve(model.materials.size());
	outMaterials.reserve(model.materials.size());
	outTextures.reserve(model.images.size());

	for (const auto& m : model.materials) {
		CpuMaterial cm{};
		CpuTexture baseColorTex{};
		CpuTexture metRoughTex{};
		CpuTexture normalTex{};
		if (m.pbrMetallicRoughness.baseColorFactor.size() == 4) {
			for (int k = 0; k < 4; ++k)
				cm.baseColorFactor[k] = (float)m.pbrMetallicRoughness.baseColorFactor[k];
		}
		else {
			cm.baseColorFactor[0] = cm.baseColorFactor[1] =
				cm.baseColorFactor[2] = cm.baseColorFactor[3] = 1.0f;
		}
		if (m.pbrMetallicRoughness.metallicFactor >= 0.0f) cm.metallic = (float)m.pbrMetallicRoughness.metallicFactor;
		if (m.pbrMetallicRoughness.roughnessFactor >= 0.0f) cm.roughness = (float)m.pbrMetallicRoughness.roughnessFactor;

		if (m.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			uint8_t texIndex;
			texIndex = m.pbrMetallicRoughness.baseColorTexture.index;

			const tinygltf::Texture& gltfTex = model.textures[texIndex];
			tinygltf::Image& gltfImg = model.images[gltfTex.source];

			SamplerDesc sampler{};
			if (gltfTex.sampler >= 0 && gltfTex.sampler < (int)model.samplers.size()) {
				sampler = MapGltfSampler(&model.samplers[gltfTex.sampler]);
			}
			else {
				sampler = MapGltfSampler(nullptr); // or a default SamplerDesc
			}

			baseColorTex = BuildCpuTextureFromGltfImage(gltfImg, sampler, "baseColor", baseColorTex);

			//cm.baseColor = &baseColorTex;
			Logger::Log(1, "%s: baseColor texture %u for material %s\n",
				__FUNCTION__, texIndex, m.name.c_str());
			Logger::Log(1, "%s: Mat baseColor texture %u for material %s\n",
				__FUNCTION__, baseColorTex.desc.width, m.name.c_str());

			outTextures.push_back(std::move(baseColorTex));
			cm.baseColorTexIdx = (int)outTextures.size() - 1;

			Logger::Log(1, "%s: baseColorTexIdx %d for material %s\n",
				__FUNCTION__, cm.baseColorTexIdx, m.name.c_str());
		}

		//if (m.pbrMetallicRoughness.baseColorTexture.index >= 0)
		//{
		//	uint8_t texIndex;
		//	texIndex = m.pbrMetallicRoughness.baseColorTexture.index;

		//	const tinygltf::Texture& gltfTex = model.textures[texIndex];
		//	tinygltf::Image& gltfImg = model.images[gltfTex.source];

		//	SamplerDesc sampler{};
		//	if (gltfTex.sampler >= 0 && gltfTex.sampler < (int)model.samplers.size()) {
		//		sampler = MapGltfSampler(&model.samplers[gltfTex.sampler]);
		//	}
		//	else {
		//		sampler = MapGltfSampler(nullptr); // or a default SamplerDesc
		//	}

		//	BuildCpuTextureFromGltfImage(gltfImg, sampler, "baseColor", baseColorTex);

		//	//cm.baseColor = &baseColorTex;
		//	Logger::Log(1, "%s: baseColor texture %u for material %s\n",
		//		__FUNCTION__, texIndex, m.name.c_str());
		//	Logger::Log(1, "%s: Mat baseColor texture %u for material %s\n",
		//		__FUNCTION__, baseColorTex.desc.width, m.name.c_str());

		//	outTextures.push_back(std::move(baseColorTex));
		//	cm.baseColorTexIdx = (int)outTextures.size() - 1;

		//	Logger::Log(1, "%s: baseColorTexIdx %d for material %s\n",
		//		__FUNCTION__, cm.baseColorTexIdx, m.name.c_str());
		//}

		//if (m.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		//{
		//	uint8_t texIndex;
		//	texIndex = m.pbrMetallicRoughness.metallicRoughnessTexture.index;

		//	const tinygltf::Texture& gltfTex = model.textures[texIndex];
		//	tinygltf::Image& gltfImg = model.images[gltfTex.source];

		//	SamplerDesc sampler{};
		//	if (gltfTex.sampler >= 0 && gltfTex.sampler < (int)model.samplers.size()) {
		//		sampler = MapGltfSampler(&model.samplers[gltfTex.sampler]);
		//	}
		//	else {
		//		sampler = MapGltfSampler(nullptr); // or a default SamplerDesc
		//	}

		//	BuildCpuTextureFromGltfImage(gltfImg, sampler, "", baseColorTex);

		//	//cm.baseColor = &baseColorTex;
		//	Logger::Log(1, "%s: metrough texture %u for material %s\n",
		//		__FUNCTION__, texIndex, m.name.c_str());
		//	Logger::Log(1, "%s: Mat metrough texture %u for material %s\n",
		//		__FUNCTION__, baseColorTex.desc.width, m.name.c_str());

		//	outTextures.push_back(std::move(baseColorTex));
		//	cm.baseColorTexIdx = (int)outTextures.size() - 1;

		//	Logger::Log(1, "%s: metrough %d for material %s\n",
		//		__FUNCTION__, cm.baseColorTexIdx, m.name.c_str());
		//}

		Logger::Log(1, "%s: material %s, baseColorFactor: %.2f, %.2f, %.2f, %.2f\n",
			__FUNCTION__, m.name.c_str(),
			cm.baseColorFactor[0], cm.baseColorFactor[1],
			cm.baseColorFactor[2], cm.baseColorFactor[3]);
		Logger::Log(1, "%s: material %s, metallic: %.2f, roughness: %.2f\n", __FUNCTION__,
			m.name.c_str(), cm.metallic, cm.roughness);

		Logger::Log(1, "%s: material %s, baseColorH: %u\n", __FUNCTION__,
			m.name.c_str(), cm.baseColorH);
		outMaterials.push_back(cm);
	}



	// Vertex with uv0/uv1/uv2
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 norm;
		glm::vec2 uv;
		glm::vec2 uv1;
		glm::vec2 uv2;
	};

	if (model.meshes.empty()) {
		Logger::Log(1, "%s: no meshes in file\n", __FUNCTION__);
		return false;
	}
	outCpuModel.meshes.reserve(model.meshes.size());

	for (size_t im = 0; im < model.meshes.size(); ++im) {
		const tinygltf::Mesh& gmesh = model.meshes[im];

		const tinygltf::Value& val = gmesh.extras.Get("isBox");
		const tinygltf::Value& val2 = gmesh.extras.Get("isCollider");
		if (val.IsInt() && val.Get<int>() == 1 && val2.IsInt() && val2.Get<int>() == 1) {
			Logger::Log(1, "Mesh is a box collider, skipping\n");
			continue;
		}

		const tinygltf::Value& planeVal = gmesh.extras.Get("isPlane");
		const tinygltf::Value& planeVal2 = gmesh.extras.Get("isCollider");
		if (planeVal.IsInt() && planeVal.Get<int>() == 1 && planeVal2.IsInt() && planeVal2.Get<int>() == 1) {
			Logger::Log(1, "Mesh is a plane collider, skipping\n");
			continue;
		}

		VertexLayoutKey layoutKey{};

		CpuStaticMesh cpu{};
		std::vector<Vertex>         vertices;
		std::vector<uint32_t>    idx;
		size_t indexCount = 0;
		// indices
		uint32_t firstIndex = 0;
		uint64_t sum = 0;

		cpu.submeshes.reserve(gmesh.primitives.size());
		for (const auto& prim : gmesh.primitives) {
			// position (required)
			auto itPos = prim.attributes.find("POSITION");
			if (itPos == prim.attributes.end())
				continue;
			const tinygltf::Accessor& aPos = model.accessors[itPos->second];

			// prepare streams
			std::vector<glm::vec3> P, N;
			std::vector<glm::vec2> UV0, UV1, UV2;
			ReadFloatAttrib3(model, aPos, P);

			if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end())
				ReadFloatAttrib3(model, model.accessors[it->second], N);
			else
				N.assign(P.size(), glm::vec3(0, 1, 0));

			if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end())
				ReadFloatAttrib2(model, model.accessors[it->second], UV0);
			else
				UV0.assign(P.size(), glm::vec2(0, 0));

			if (auto it = prim.attributes.find("TEXCOORD_1"); it != prim.attributes.end())
				ReadFloatAttrib2(model, model.accessors[it->second], UV1);
			else
				UV1.assign(P.size(), glm::vec2(0, 0));

			if (auto it = prim.attributes.find("TEXCOORD_2"); it != prim.attributes.end())
				ReadFloatAttrib2(model, model.accessors[it->second], UV2);
			else
				UV2.assign(P.size(), glm::vec2(0, 0));

			const uint32_t baseV = (uint32_t)vertices.size();
			vertices.reserve(vertices.size() + P.size());

			for (size_t i = 0; i < P.size(); ++i) {
				Vertex v{};
				v.pos = P[i];
				v.norm = i < N.size() ? N[i] : glm::vec3(0, 1, 0);
				v.uv = i < UV0.size() ? UV0[i] : glm::vec2(0);
				v.uv1 = i < UV1.size() ? UV1[i] : glm::vec2(0);
				v.uv2 = i < UV2.size() ? UV2[i] : glm::vec2(0);
				vertices.push_back(v);
			}

			uint32_t added = 0;
			std::vector<uint32_t> localIdx;
			if (prim.indices >= 0) {
				const tinygltf::Accessor& indexAccessor = model.accessors[prim.indices];
				const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];



				// Pointer to the actual index data
				const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

				// Loop through and extract indices based on the component type
				for (size_t i = 0; i < indexAccessor.count; ++i) {
					switch (indexAccessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						idx.push_back(static_cast<unsigned int>(reinterpret_cast<const uint8_t*>(dataPtr)[i]));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						idx.push_back(static_cast<unsigned int>(reinterpret_cast<const uint16_t*>(dataPtr)[i]));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						idx.push_back(reinterpret_cast<const uint32_t*>(dataPtr)[i]);
						break;
					}
					default:
						Logger::Log(1, " << indexAccessor.componentType, %zu", indexAccessor.componentType);
						break;
					}
				}
				added = (uint32_t)idx.size() - firstIndex;
			}
			else {
				localIdx.resize(P.size());
				//std::iota(localIdx.begin(), localIdx.end(), 0u);
			}

			CpuSubmesh sm{};
			sm.firstIndex = firstIndex;
			firstIndex += added;
			sm.indexCount = idx.size() - sm.firstIndex; // or (uint32_t)idx.size() - firstIndex
			sm.materialIndex = (prim.material >= 0) ? (uint32_t)prim.material : 0;
			sm.firstVertex = baseV; // optional

			uint32_t maxIdx = 0;
			for (auto v : idx) maxIdx = std::max(maxIdx, v);

			if (maxIdx >= vertices.size()) {
				Logger::Log(1, "[DEBUG] mesh %zu: maxIdx=%u >= vertCount=%zu  <-- baseV/VAO issue\n",
					im, maxIdx, vertices.size());
			}

			for (auto& i : localIdx) i += baseV;

			sm.firstVertex = baseV;
			//sm.firstIndex = (uint32_t)idx.size();
			//sm.indexCount = (uint32_t)localIdx.size();

	/*		for (uint32_t i = firstIndex; i < added; i++)
			{
				sm.indexData.push_back(idx[i]);
			}*/

			Logger::Log(1, "ImportStaticModelFromGltf: mesh %zu name='%s' prims=%zu verts=%zu idx=%zu\n",
				im,
				gmesh.name.c_str(),
				gmesh.primitives.size(),
				vertices.size(),
				idx.size());

			sm.vertexCount = (uint32_t)vertices.size() - baseV;
			sm.vertexStride = sizeof(Vertex);
			sm.vertexData.resize(sm.vertexStride * vertices.size());
			if (!vertices.empty())
				std::memcpy(sm.vertexData.data(), vertices.data(), sm.vertexData.size());

			//sm.indexCount = (uint32_t)indexCount;
			maxIdx = 0;
			for (uint32_t v : idx) maxIdx = std::max(maxIdx, v);
			const bool fitsU16 = (maxIdx <= 0xFFFF);
			sm.index32 = !fitsU16;


			// TODO indexCount += sm.indexData.size();



			for (const auto& sm : cpu.submeshes) sum += sm.indexData.size();
			if (sum != idx.size()) {
				Logger::Log(1, "[DEBUG] mesh %zu: sum(submesh.indexCount)=%llu != idx.size()=%zu  <-- firstIndex/indexCount bookkeeping\n",
					im, (unsigned long long)sum, idx.size());
			}

			if (fitsU16) {
				std::vector<uint16_t> idx16(idx.begin(), idx.end());
				sm.indexData.resize(idx16.size() * sizeof(uint16_t));
				std::memcpy(sm.indexData.data(), idx16.data(), sm.indexData.size());
			}
			else {
				sm.indexData.resize(idx.size() * sizeof(uint32_t));
				std::memcpy(sm.indexData.data(), idx.data(), sm.indexData.size());
			}

			cpu.submeshes.push_back(sm);

		}
		outCpuModel.meshes.emplace_back(std::move(cpu));
		Logger::Log(1, "%s: loaded %zu meshes with %u vertices and %u indices\n",
			__FUNCTION__,
			outCpuModel.meshes.size(),
			(unsigned)vertices.size(),
			(unsigned)idx.size());
	}

	Logger::Log(1, "%s: loaded %zu materials\n", __FUNCTION__, outMaterials.size());
	return true;
}
