#include "Engine/Asset/AssetManager.h"
#include "GLTFImporter.h"
#include "Logger.h"

ModelHandle AssetManager::MakeModelHandle() {
	return m_nextModelHandle++;
}

MaterialHandle AssetManager::MakeMaterialHandle() {
	return m_nextMaterialHandle++;
}

TextureHandle AssetManager::MakeTextureHandle()
{
	return ++m_nextTextureHandle;
}



ModelHandle AssetManager::LoadStaticModel(const std::string& gltfPath) {
	CpuStaticModel cpu{};
	std::vector<CpuMaterial> outMaterials{};
	std::vector<CpuTexture> outTextures{};

	if (!ImportStaticModelFromGltf(gltfPath, cpu, outMaterials, outTextures)) {
		Logger::Log(0, "Failed to load static model from %s\n", gltfPath.c_str());
		return InvalidHandle;
	}

	std::vector<TextureHandle> textureHandles;
	textureHandles.reserve(outTextures.size());
	for (auto tex : outTextures) {
		textureHandles.push_back(CreateTexture(std::move(tex)));
	}

	m_cpuMaterials.reserve(outMaterials.size());

	for (auto& mat : outMaterials) {
		MaterialHandle matHandle = CreateMaterial(mat);
		cpu.materials.push_back(matHandle);
		m_cpuMaterials[matHandle] = std::make_unique<CpuMaterial>(std::move(mat));
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
	m_cpuMaterials[matHandle] = std::make_unique<CpuMaterial>(std::move(mat));
	return matHandle;
}

const CpuMaterial* AssetManager::GetCpuMaterial(MaterialHandle h) const {
	auto it = m_cpuMaterials.find(h);
	if (it != m_cpuMaterials.end()) {
		return it->second.get();
	}
	return nullptr;
}

static int bytesPerPixel(PixelFormat f) {
	switch (f) {
	case PixelFormat::RGB8_UNORM:  return 3;
	case PixelFormat::RGBA8_UNORM: return 4;
		// add others you use
	default: return 0;
	}
}

TextureHandle AssetManager::CreateTexture(CpuTexture tex)
{
	TextureHandle texHandle = MakeTextureHandle();
	while (GetCpuTexture(texHandle)) {
		texHandle = MakeTextureHandle();
	}

	m_cpuTextures[texHandle] = std::make_unique<CpuTexture>(std::move(tex));

	return texHandle;
}


const CpuTexture* AssetManager::GetCpuTexture(TextureHandle h) const
{
	auto it = m_cpuTextures.find(h);
	if (it != m_cpuTextures.end()) {
		return it->second.get();
	}
	return nullptr;
}
