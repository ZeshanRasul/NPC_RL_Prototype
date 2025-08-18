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
	return m_nextTextureHandle++;
}



ModelHandle AssetManager::LoadStaticModel(const std::string& gltfPath) {
	CpuStaticModel cpu{};
	std::vector<CpuMaterial> outMaterials{};

	if (!ImportStaticModelFromGltf(gltfPath, cpu, outMaterials)) {
		Logger::Log(0, "Failed to load static model from %s\n", gltfPath.c_str());
		return InvalidHandle;
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
	if (mat.baseColor.desc.height >= 0)
		mat.baseColorH = CreateTexture(mat.baseColor);
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

TextureHandle AssetManager::CreateTexture(CpuTexture tex)
{
	TextureHandle texHandle = MakeTextureHandle();
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
