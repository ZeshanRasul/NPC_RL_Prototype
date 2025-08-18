#pragma once

#include <vector>
#include <string>
#include <cstdint>

using ModelHandle = uint32_t;
using MaterialHandle = uint32_t;
using TextureHandle = uint32_t;
constexpr uint32_t InvalidHandle = 0;

enum class TextureUsage : uint8_t {
	BaseColorSRGB,
	NormalLinear,
	MetalRoughLinear,
	EmissiveSRGB,
	Unknown
};

enum class TexFilterGpu : uint8_t { Nearest, Linear };
enum class MipFilterGpu : uint8_t { None, Nearest, Linear };
enum class AddressModeGpu : uint8_t { Repeat, ClampToEdge, MirrorRepeat };

struct SamplerDesc {
	TexFilterGpu  minFilter = TexFilterGpu::Linear;
	TexFilterGpu  magFilter = TexFilterGpu::Linear;
	MipFilterGpu  mipFilter = MipFilterGpu::Linear;
	AddressModeGpu wrapS = AddressModeGpu::Repeat;
	AddressModeGpu wrapT = AddressModeGpu::Repeat;
};

enum class ColorSpace : uint8_t { Linear, SRGB };

enum class PixelFormat : uint8_t {
	R8_UNORM, RG8_UNORM, RGB8_UNORM, RGBA8_UNORM,
	R16_UNORM, RG16_UNORM, RGB16_UNORM, RGBA16_UNORM,
	R16_FLOAT, RG16_FLOAT, RGB16_FLOAT, RGBA16_FLOAT,
	R32_FLOAT, RG32_FLOAT, RGB32_FLOAT, RGBA32_FLOAT,
};

struct CpuSubmesh {
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t materialIndex = 0;
	uint32_t firstVertex = 0;
	std::vector<uint8_t> vertexData;
	std::vector<uint8_t> indexData;
	uint32_t vertexCount = 0;
	uint32_t vertexStride = 0;
	bool     index32 = true;
	float aabbMin[3] = { 0.0f, 0.0f, 0.0f };
	float aabbMax[3] = { 0.0f, 0.0f, 0.0f };
	MaterialHandle material = InvalidHandle;
};

struct CpuStaticMesh {
	std::vector<CpuSubmesh> submeshes;
};

struct TextureDesc {
	uint32_t    width = 0;
	uint32_t    height = 0;
	uint32_t    mipLevels = 1;  
	PixelFormat format = PixelFormat::RGBA8_UNORM;
	ColorSpace  colorSpace = ColorSpace::Linear;
	SamplerDesc sampler{};
};

struct CpuTexture {
	TextureDesc desc;

	std::vector<uint8_t> pixels;
	uint32_t rowStride = 0;
	TextureUsage usage = TextureUsage::BaseColorSRGB;
};

struct MaterialTexcoordSets {
	int baseColor = 0;
	int normal = 0;
	int metallicRoughness = 0;
	int occlusion = 0;
	int emissive = 0;
};

struct CpuMaterial {
	//CpuTexture* baseColor;
	//CpuTexture* metallicRoughness;
	//CpuTexture* normal;
	//CpuTexture* emissive;

	TextureHandle baseColorH;
	TextureHandle metallicRoughnessH;
	TextureHandle normalH;
	TextureHandle emissiveH;

	int baseColorTexIdx = -1;

	MaterialTexcoordSets texcoordSets{};

	float baseColorFactor[4] = { 1.0f,1.0f,1.0f,1.0f };
	float metallic = 0.0f;
	float roughness = 1.0f;
	float emissiveFactor[3] = { 0.0f, 0.0f, 0.0f };
};

struct CpuStaticModel {
	std::vector<CpuStaticMesh> meshes;
	std::vector<MaterialHandle> materials;
	std::vector<CpuMaterial> materialsData;
};



