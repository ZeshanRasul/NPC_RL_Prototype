#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include "Engine/Asset/AssetTypes.h"

class AssetManager {
public:
	ModelHandle LoadStaticModel(const std::string& gltfPath);
	const CpuStaticModel* GetCpuStaticModel(ModelHandle h) const;

	MaterialHandle CreateMaterial(const CpuMaterial& mat);
	const CpuMaterial* GetCpuMaterial(MaterialHandle h) const;

	TextureHandle CreateTexture(CpuTexture tex);
	const Texture* GetCpuTexture(TextureHandle h) const;

private:
	ModelHandle MakeModelHandle();
	MaterialHandle MakeMaterialHandle();

	std::unordered_map<ModelHandle, std::unique_ptr<CpuStaticModel>> m_cpuStaticModels;
	ModelHandle m_nextModelHandle{ 1 };

	std::unordered_map<MaterialHandle, std::unique_ptr<CpuMaterial>> m_cpuMaterials;
	MaterialHandle m_nextMaterialHandle{ 1 };
};
