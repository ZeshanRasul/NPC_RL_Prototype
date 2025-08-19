#pragma once

#include <string>
#include <vector>

#include "AssetTypes.h"
#include "Logger.h"

bool ImportStaticModelFromGltf(const std::string& gltfPath, CpuStaticModel& outCpuModel,
	std::vector<CpuMaterial>& outMaterials, std::vector<CpuTexture>& outTextures);

CpuStaticModel ImportModel(const std::string& gltfPath, std::vector<CpuMaterial>& outMaterials, std::vector<CpuTexture>& outTextures);
