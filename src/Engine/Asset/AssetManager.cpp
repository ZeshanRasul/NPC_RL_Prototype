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
	std::vector<CpuTexture> outTextures{};

	if (!ImportStaticModelFromGltf(gltfPath, cpu, outMaterials, outTextures)) {
		Logger::Log(0, "Failed to load static model from %s\n", gltfPath.c_str());
		return InvalidHandle;
	}

	//std::vector<TextureHandle> textureHandles;
	//textureHandles.reserve(outTextures.size());
	//for (CpuTexture tex : outTextures) {
	//	textureHandles.push_back(CreateTexture(tex));
	//}

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
	CreateTexture(mat.baseColor);
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

	// Validate

	// Expand to RGBA if needed (e.g. JPG source)
	ExpandRGBToRGBA(newtex.pixels, newtex.desc.width, newtex.desc.width);
		

	// Force consistency: everything is RGBA8 UNORM
	newtex.desc.format = PixelFormat::RGBA8_UNORM;
;

	// Optionally generate a single mip chain here (CPU side)
	// or just store base level; renderer can generate GPU mips.

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
