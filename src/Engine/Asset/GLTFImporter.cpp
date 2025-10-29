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

template<typename T>
static void ReadAccessorData(const tinygltf::Model& model,
	const tinygltf::Accessor& acc,
	std::vector<T>& out,
	size_t compCount)
{
	const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
	const tinygltf::Buffer& buf = model.buffers[bv.buffer];

	size_t stride = bv.byteStride ? bv.byteStride : compCount * sizeof(T);
	const unsigned char* src = buf.data.data() + bv.byteOffset + acc.byteOffset;

	size_t start = out.size();
	out.resize(start + acc.count * compCount);

	for (size_t i = 0; i < acc.count; ++i) {
		const T* val = reinterpret_cast<const T*>(src + i * stride);
		for (size_t c = 0; c < compCount; ++c)
			out[start + i * compCount + c] = val[c];
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

			uint32_t vertCount = 0;
			// Store transform per mesh
			cpuMesh.submeshes.reserve(mesh.primitives.size());

			for (const auto& prim : mesh.primitives) {
				CpuStaticMesh cpuMesh;

		

				uint32_t vertCount = 0;
				// Store transform per mesh
				cpuMesh.submeshes.reserve(mesh.primitives.size());
				for (const auto& prim : mesh.primitives) {
					CpuSubmesh sm{};

					// gather attributes into temporary vectors
					std::vector<float> pos, norm, tan, uv0, uv1, col;

					if (prim.attributes.count("POSITION")) {
						const auto& acc = model.accessors.at(prim.attributes.at("POSITION"));
						ReadAccessorData<float>(model, acc, pos, 3);
					}
					else {
						// glTF spec requires POSITION, but guard anyway
						return;
					}
					if (prim.attributes.count("NORMAL")) {
						const auto& acc = model.accessors.at(prim.attributes.at("NORMAL"));
						ReadAccessorData<float>(model, acc, norm, 3);
					}

					if (prim.attributes.count("TEXCOORD_0")) {
						const auto& acc = model.accessors.at(prim.attributes.at("TEXCOORD_0"));
						ReadAccessorData<float>(model, acc, uv0, 2);
					}


					// stride = sum of enabled attributes
					sm.vertexStride = 3 * sizeof(float);
					if (!norm.empty()) sm.vertexStride += 3 * sizeof(float);
					if (!uv0.empty())  sm.vertexStride += 2 * sizeof(float);

					// pack vertices interleaved
					size_t vertCount = pos.size() / 3;
					sm.vertexCount = (uint32_t)vertCount;
					sm.vertexData.resize(sm.vertexCount * sm.vertexStride);

					for (size_t i = 0; i < vertCount; ++i) {
						unsigned char* dst = sm.vertexData.data() + i * sm.vertexStride;
						float* f = reinterpret_cast<float*>(dst);

						// position
						f[0] = pos[i * 3 + 0]; f[1] = pos[i * 3 + 1]; f[2] = pos[i * 3 + 2];
						size_t offset = 3;

						if (!norm.empty()) {
							f[offset + 0] = norm[i * 3 + 0];
							f[offset + 1] = norm[i * 3 + 1];
							f[offset + 2] = norm[i * 3 + 2];
							offset += 3;
						}
						if (!uv0.empty()) {
							f[offset + 0] = uv0[i * 2 + 0];
							f[offset + 1] = uv0[i * 2 + 1];
							offset += 2;
						}

					}

					// indices
					size_t idxBegin = 0;
					if (prim.indices >= 0) {
						const auto& acc = model.accessors[prim.indices];
						const auto& bv = model.bufferViews[acc.bufferView];
						const auto& buf = model.buffers[bv.buffer];

						size_t stride = bv.byteStride ? bv.byteStride :
							(acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ? 4 :
								acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2 : 1);

						const unsigned char* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
						sm.indexCount = (uint32_t)acc.count;
						sm.index32 = (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);

						size_t elemSize = sm.index32 ? sizeof(uint32_t) : sizeof(uint16_t);
						sm.indexData.resize(sm.indexCount * elemSize);

						for (size_t i = 0; i < acc.count; ++i) {
							const unsigned char* p = base + i * stride;
							uint32_t v = 0;
							switch (acc.componentType) {
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  v = *reinterpret_cast<const uint8_t*>(p); break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: v = *reinterpret_cast<const uint16_t*>(p); break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   v = *reinterpret_cast<const uint32_t*>(p); break;
							default: break;
							}
							if (sm.index32)
								reinterpret_cast<uint32_t*>(sm.indexData.data())[i] = v;
							else
								reinterpret_cast<uint16_t*>(sm.indexData.data())[i] = (uint16_t)v;
						}
					}
					else {
						// non-indexed: sequential indices
						sm.indexCount = sm.vertexCount;
						sm.index32 = (sm.vertexCount > 65535);
						size_t elemSize = sm.index32 ? sizeof(uint32_t) : sizeof(uint16_t);
						sm.indexData.resize(sm.indexCount * elemSize);
						for (uint32_t i = 0; i < sm.vertexCount; ++i) {
							if (sm.index32)
								reinterpret_cast<uint32_t*>(sm.indexData.data())[i] = i;
							else
								reinterpret_cast<uint16_t*>(sm.indexData.data())[i] = (uint16_t)i;
						}
					}

					// material
					sm.materialIndex = prim.material >= 0 ? prim.material : 0;

					// AABB
					sm.aabbMin[0] = sm.aabbMin[1] = sm.aabbMin[2] = FLT_MAX;
					sm.aabbMax[0] = sm.aabbMax[1] = sm.aabbMax[2] = -FLT_MAX;
					for (size_t i = 0; i < vertCount; i++) {
						float x = pos[i * 3 + 0], y = pos[i * 3 + 1], z = pos[i * 3 + 2];
						sm.aabbMin[0] = std::min(sm.aabbMin[0], x); sm.aabbMax[0] = std::max(sm.aabbMax[0], x);
						sm.aabbMin[1] = std::min(sm.aabbMin[1], y); sm.aabbMax[1] = std::max(sm.aabbMax[1], y);
						sm.aabbMin[2] = std::min(sm.aabbMin[2], z); sm.aabbMax[2] = std::max(sm.aabbMax[2], z);
					}


					Logger::Log(1, "%s: loaded %zu meshes with %u vertices and %u indices\n",
						__FUNCTION__,
						outModel.meshes.size(),
						(unsigned)sm.vertexData.size(),
						(unsigned)sm.indexData.size());
					const tinygltf::Value& val = mesh.extras.Get("isBox");
					const tinygltf::Value& val2 = mesh.extras.Get("isCollider");
					if (val.IsInt() && val.Get<int>() == 1 && val2.IsInt() && val2.Get<int>() == 1) {
						Logger::Log(1, "% Mesh is a box collider, setting up AABB\n", __FUNCTION__);
						continue;
					}

					const tinygltf::Value& planeVal = mesh.extras.Get("isPlane");
					const tinygltf::Value& planeVal2 = mesh.extras.Get("isCollider");
					if (planeVal.IsInt() && planeVal.Get<int>() == 1 && planeVal2.IsInt() && planeVal2.Get<int>() == 1) {
						Logger::Log(1, "% Mesh is a plane collider, setting up AABB\n", __FUNCTION__);
						continue;
					}
					cpuMesh.submeshes.push_back(sm);
				}
				outModel.meshes.emplace_back(std::move(cpuMesh));

			}
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

		if (m.emissiveFactor.size() == 3) {
			cm.emissiveFactor[0] = (float)m.emissiveFactor[0];
			cm.emissiveFactor[1] = (float)m.emissiveFactor[1];
			cm.emissiveFactor[2] = (float)m.emissiveFactor[2];
		}
		else {
			cm.emissiveFactor[0] = cm.emissiveFactor[1] = cm.emissiveFactor[2] = 0.0f;
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
			cm.baseColorTexIdx = (int)outTextures.size();


			int texArrayIndex = (int)outTextures.size();
			cm.baseColorTexIdx = texArrayIndex;

			cm.baseColorH = (TextureHandle)(texArrayIndex);
			Logger::Log(1, "%s: baseColorTexIdx=%d, baseColorH=%u, material=%s\n",
				__FUNCTION__, cm.baseColorTexIdx, cm.baseColorH, m.name.c_str());
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



