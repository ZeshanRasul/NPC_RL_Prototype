#include "Engine/Asset/AssetManager.h"
#include "GLTFImporter.h"
#include "Logger.h"

ModelHandle AssetManager::MakeModelHandle() {
	return ++m_nextModelHandle;
}

MaterialHandle AssetManager::MakeMaterialHandle() {
	return ++m_nextMaterialHandle;
}

TextureHandle AssetManager::MakeTextureHandle()
{
	return ++m_nextTextureHandle;
}



ModelHandle AssetManager::LoadStaticModel(const std::string& gltfPath) {
	CpuStaticModel cpu{};
	std::vector<CpuMaterial> outMaterials{};
	std::vector<CpuTexture> outTextures{};

	cpu = ImportModel(gltfPath, outMaterials, outTextures);

	std::vector<TextureHandle> textureHandles;
	textureHandles.reserve(outTextures.size());
	
	m_cpuTextures.reserve(outTextures.size());

	for (auto& tex : outTextures) {
		TextureHandle th = CreateTexture(tex);
		textureHandles.push_back(th);
		cpu.textures.push_back(th);
		m_cpuTextures[th] = std::make_unique<CpuTexture>(std::move(tex));
	}

	m_cpuMaterials.reserve(outMaterials.size());

	for (auto& mat : outMaterials) {
		MaterialHandle matHandle = CreateMaterial(mat);
		if (mat.baseColorTexIdx >= 0 && mat.baseColorTexIdx < (int(textureHandles.size()))) {
			mat.baseColorH = textureHandles[mat.baseColorTexIdx];
			mat.baseColorTexIdx = -1;
			Logger::Log(1, "Material baseColorH %u for material with handle %u\n", 
				mat.baseColorH, matHandle);
		} else {
			mat.baseColorH = InvalidHandle; 
		}
		cpu.materials.push_back(matHandle);
		m_cpuMaterials[matHandle] = std::make_unique<CpuMaterial>(std::move(mat));
	}		
	
	std::vector<MaterialHandle> gltfMatIdx_to_handle;
	gltfMatIdx_to_handle.reserve(cpu.materials.size());
	for (auto h : cpu.materials) gltfMatIdx_to_handle.push_back(h);

	for (auto& mesh : cpu.meshes) {
		for (auto& sm : mesh.submeshes) {
			if (sm.materialIndex >= 0 && sm.materialIndex < (int)gltfMatIdx_to_handle.size()) {
				sm.material = gltfMatIdx_to_handle[sm.materialIndex];
				Logger::Log(1, "Submesh remap: glTF mat %d -> handle %u\n",
					sm.materialIndex, sm.material);
			}
			else {
				sm.material = InvalidHandle;
				Logger::Log(1, "Submesh remap: glTF mat %d -> InvalidHandle\n", sm.materialIndex);
			}


			sm.materialIndex = -1;
		}
	}

	ModelHandle mh = MakeModelHandle();
	m_cpuStaticModels[mh] = std::make_unique<CpuStaticModel>(std::move(cpu));

	return mh;
}

const CpuStaticModel* AssetManager::GetCpuStaticModel(ModelHandle h) const {
	auto it = m_cpuStaticModels.find(h);
	if (it != m_cpuStaticModels.end()) {
		return it->second.get();
	}
	return nullptr;
}

MaterialHandle AssetManager::CreateMaterial(CpuMaterial mat) {
	MaterialHandle matHandle = MakeMaterialHandle();
	//mat.baseColorH = CreateTexture(*mat.baseColor);
	//m_cpuTextures.emplace(std::make_uniqueh);
	//m_cpuMaterials[matHandle] = std::make_unique<CpuMaterial>(std::move(mat));
	return matHandle;
}

const CpuMaterial* AssetManager::GetCpuMaterial(MaterialHandle h) const {
	auto it = m_cpuMaterials.find(h);
	if (it != m_cpuMaterials.end()) {
		return it->second.get();
	}
	return m_cpuMaterials.at(0).get();
	//TODO : handle invalid material handle gracefully
}

static int bytesPerPixel(PixelFormat f) {
	switch (f) {
	case PixelFormat::RGB8_UNORM:  return 3;
	case PixelFormat::RGBA8_UNORM: return 4;
	default: return 0;
	}
}
static void ExpandRGBToRGBA(std::vector<uint8_t>& data, int w, int h)
{
	const size_t srcBytes = static_cast<size_t>(w) * h * 3;
	if (data.size() != srcBytes) return; // already RGBA or invalid

	std::vector<uint8_t> out;
	out.resize(static_cast<size_t>(w) * h * 4);

	const uint8_t* src = data.data();
	uint8_t* dst = out.data();

	for (size_t i = 0, j = 0; i < srcBytes; i += 3, j += 4) {
		dst[j + 0] = src[i + 0]; // R
		dst[j + 1] = src[i + 1]; // G
		dst[j + 2] = src[i + 2]; // B
		dst[j + 3] = 255;        // A = fully opaque
	}

	data.swap(out);
}
TextureHandle AssetManager::CreateTexture(CpuTexture tex)
{
	// Own the texture immediately
	auto handle = MakeTextureHandle();
	m_cpuTextures[handle] = std::make_unique<CpuTexture>(std::move(tex));
	CpuTexture& newtex = *m_cpuTextures[handle];

	 
	if (newtex.desc.format == PixelFormat::RGB8_UNORM) {
		ExpandRGBToRGBA(newtex.pixels, newtex.desc.width, newtex.desc.height);
		if (newtex.pixels.size() == size_t(newtex.desc.width) * newtex.desc.height * 4) {
			newtex.desc.format = PixelFormat::RGBA8_UNORM;
		}
	};

	return handle;
}


const CpuTexture* AssetManager::GetCpuTexture(TextureHandle h) const
{
	auto it = m_cpuTextures.find(h);
	if (it != m_cpuTextures.end()) {
		return it->second.get();
	}
	return nullptr;
}
