#pragma once
#include <cstdint>
#include <string>
#include "glm/glm.hpp"

using GpuBufferHandle = uint32_t;
using GpuPipelineHandle = uint32_t;
using GpuMaterialHandle = uint32_t;
using GpuMaterialId = uint32_t;
using GpuTextureId = uint32_t;
struct CpuTexture;

enum class RenderBackendType {
	OpenGL,
	Vulkan,
	DirectX12,
	Metal
};

enum class BufferUsage : uint32_t {
		Vertex,
		Index,
		Uniform,
		Storage,
		Staging
};

enum class IndexType : uint32_t {
		U16,
		U32
};

struct BufferCreateInfo {
	size_t size;
	BufferUsage usage = BufferUsage::Vertex;
	const void* initialData = nullptr;
};

enum class PixelFormatGpu { RGB8_UNORM, RGBA8_UNORM, R8_UNORM, RG8_UNORM};
enum class ColorSpaceGpu { Linear, SRGB };

enum class TexFilter : uint8_t { Nearest, Linear };
enum class MipFilter : uint8_t { None, Nearest, Linear };
enum class AddressMode : uint8_t { Repeat, ClampToEdge, MirrorRepeat };

struct SamplerDescGpu {
	TexFilter  minFilter = TexFilter::Linear;
	TexFilter  magFilter = TexFilter::Linear;
	MipFilter  mipFilter = MipFilter::Linear;
	AddressMode wrapS = AddressMode::Repeat;
	AddressMode wrapT = AddressMode::Repeat;
};

struct TextureCreateInfo {
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    PixelFormatGpu format = PixelFormatGpu::RGBA8_UNORM;
    ColorSpaceGpu colorSpace = ColorSpaceGpu::Linear;
	SamplerDescGpu sampler{};
    const void* initialData = nullptr;  
    size_t initialDataSize = 0;  
};

struct PipelineDesc {
	RenderBackendType backendType;
	GpuMaterialId materialId;
	std::string vertexShaderPath;
	std::string fragmentShaderPath;
	std::string computeShaderPath;
	uint32_t vertexShaderId;
	uint32_t fragmentShaderId;
	uint32_t computeShaderId;
	uint32_t vertexStride = 0;
	bool depthTest = true;
	bool depthWrite = true;
	bool cullFace = true;
	bool blend = false;
	uint32_t blendSrcFactor = 0; 
	uint32_t blendDstFactor = 0; 
};

struct MaterialGpuDesc {
	GpuTextureId baseColor = 0;
	float baseColorFactor[4];
	float metallic;
	float roughness;
	float emissiveFactor[3];
};

struct DrawItem {
	GpuPipelineHandle pipeline = 0;
#define ENGINE_BACKEND_OPENGL

#ifdef ENGINE_BACKEND_OPENGL
	GLuint vao = 0;
#endif 

	GpuBufferHandle vertexBuffer = 0;
	GpuBufferHandle indexBuffer = 0;
	IndexType indexType = IndexType::U32;
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t instanceCount = 1;
	uint32_t vertexCount = 0;
	GpuMaterialHandle materialHandle = 0;
	GpuMaterialId materialId = 0;
	GpuTextureId textureId = 0;
	glm::mat4 transform = glm::mat4(1.0f); // Optional, for static meshes
};

class RenderBackend {
public:
	virtual ~RenderBackend() = default;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual GpuBufferHandle CreateBuffer(const BufferCreateInfo& createInfo) = 0;
	virtual void UpdateBuffer(GpuBufferHandle h, size_t offset, const void* data, size_t size) = 0;		
	virtual void DestroyBuffer(GpuBufferHandle buffer) = 0;

	virtual GpuPipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;

	virtual GpuMaterialId CreateMaterial(MaterialGpuDesc& desc) = 0;
	virtual void UpdateMaterial(GpuMaterialId materialId, const MaterialGpuDesc& desc) = 0;
	virtual void DestroyMaterial(GpuMaterialId materialId) = 0;

	virtual GpuTextureId CreateTexture2D(const CpuTexture& cpu, GpuMaterialHandle matHandle) = 0;

	virtual void BeginFrame() = 0;
	virtual void Submit(const DrawItem* items, uint32_t itemCount) = 0;
	virtual void EndFrame() = 0;

	virtual void BindUniformBuffer(GpuBufferHandle h, uint32_t binding) = 0;
	//virtual void UploadCameraMatrices(GpuBufferHandle h, const std::vector<glm::mat4>& mats, int bindingPoint) = 0;
};