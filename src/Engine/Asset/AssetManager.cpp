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

	std::vector<TextureHandle> textureHandles(outTextures.size(), InvalidHandle);
	
	m_cpuTextures.reserve(m_cpuTextures.size() + outTextures.size());

	for (size_t i = 0; i < outTextures.size(); ++i) {
		textureHandles[i] = CreateTexture(outTextures[i]);
		cpu.textures.push_back(textureHandles[i]);
		m_cpuTextures[textureHandles[i]] = std::make_unique<CpuTexture>(std::move(outTextures[i]));
	}

	std::vector<MaterialHandle> gltfMatIdx_to_handle(outMaterials.size(), InvalidHandle);
	m_cpuMaterials.reserve(m_cpuMaterials.size() + outMaterials.size());

	for (size_t gi = 0; gi < outMaterials.size(); ++gi)
	{
		CpuMaterial& mat = outMaterials[gi];

		if (mat.baseColorTexIdx >= 0 && mat.baseColorTexIdx < static_cast<int>(textureHandles.size()))
			mat.baseColorH = textureHandles[mat.baseColorTexIdx];
		else
			mat.baseColorH = InvalidHandle;

		mat.baseColorTexIdx = -1;

		if (mat.emissiveFactor[0] > 0.0f || mat.emissiveFactor[1] > 0.0f || mat.emissiveFactor[2] > 0.0f) {
		}

		MaterialHandle mh = CreateMaterial(mat);
		gltfMatIdx_to_handle[mh] = mh;
		cpu.materials.push_back(mh);
		m_cpuMaterials[mh] = std::make_unique<CpuMaterial>(std::move(mat));
	}

	for (auto& mesh : cpu.meshes)
	{
		for (auto& sm : mesh.submeshes)
		{
			const int gi = sm.materialIndex; // tinygltf prim.material
			if (gi < 0)
			{
				sm.material = 0; // “no material” in glTF
			}
			else if (static_cast<size_t>(gi) < gltfMatIdx_to_handle.size())
			{
				sm.material = gltfMatIdx_to_handle[gi];
			}
			else
			{
				// Out-of-range guard: fall back to default to avoid UB
				sm.material = 0;
			}
		}
	}

	Logger::Log(1, "[Model] %s: %zu mats, %zu tex, %zu meshes\n",
		gltfPath.c_str(), outMaterials.size(), textureHandles.size(), cpu.meshes.size());

	for (size_t i = 0; i < gltfMatIdx_to_handle.size(); ++i) {
		auto mh = gltfMatIdx_to_handle[i];
		auto* cm = m_cpuMaterials[mh].get();
		Logger::Log(1, "  gi=%zu -> handle=%u baseColorH=%u\n", i, mh, cm ? cm->baseColorH : 0);
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
