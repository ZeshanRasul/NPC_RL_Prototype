#pragma once

#include <string>
#include <vector>

#include "AssetTypes.h"

bool ImportStaticModelFromGltf(const std::string& gltfPath, CpuStaticModel& outCpuModel,
	std::vector<CpuMaterial>& outMaterials);