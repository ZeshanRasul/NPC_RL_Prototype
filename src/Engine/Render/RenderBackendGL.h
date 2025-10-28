#include <vector>
#include <unordered_map>
#include <variant>
#include <glad/glad.h>

#include "Engine/Render/RenderBackend.h"
#include "Engine/Render/GpuUploader.h"
#include "OpenGL/Shader.h"
#include "OpenGL/UniformBuffer.h"
#include "OpenGL/ShaderStorageBuffer.h"
#include "OpenGL/Texture.h"
#include "Logger.h"

#define ENGINE_BACKEND_OPENGL

struct GLMat {
	MaterialGpuDesc desc;
};

struct GLPipeline {
	Shader program;
	uint32_t vertexStride = 0;
};

struct GLVertexIndexBuffer {
	GLuint id = 0;
	GLenum target = GL_ARRAY_BUFFER;
	size_t size = 0;
};

struct GLUniformBuffer {
	UniformBuffer ubo;
	size_t size = 0;
	GLuint id = 0;
};

struct GLStorageBuffer {
	ShaderStorageBuffer ssbo;
	size_t size = 0;
};

using GLBuffer = std::variant<GLVertexIndexBuffer, GLUniformBuffer, GLStorageBuffer>;

struct GLSamplerParams {
	GLint minFilterGL;
	GLint magFilterGL;
	GLint wrapSGL;
	GLint wrapTGL;
};

struct GLTexFormat { int internalFormat; int externalFormat; int type; };

static GLTexFormat ToGLFmt(PixelFormatGpu pf, ColorSpaceGpu cs) {
	const bool srgb = (cs == ColorSpaceGpu::SRGB);
	switch (pf) {
	case PixelFormatGpu::RGB8_UNORM:
		return { srgb ? GL_SRGB8 : GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };
	case PixelFormatGpu::RGBA8_UNORM:
		return { srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
	case PixelFormatGpu::R8_UNORM:
		return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
	case PixelFormatGpu::RG8_UNORM:
		return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };
	default:
		return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
	}
}

static GLSamplerParams ToGL(const SamplerDescGpu& s) {
	return {
		GL_LINEAR,
		GL_LINEAR,
		GL_REPEAT,
		GL_REPEAT
	};
}


class RenderBackendGL : public RenderBackend {
public:
	void Initialize() override {};
	void Shutdown() override {
		for (auto& [_, buf] : m_buffers) {
			if (std::holds_alternative<GLVertexIndexBuffer>(buf)) {
				auto& b = std::get<GLVertexIndexBuffer>(buf);
				if (b.id) glDeleteBuffers(1, &b.id);
			}
			else if (std::holds_alternative<GLUniformBuffer>(buf)) {
				std::get<GLUniformBuffer>(buf).ubo.Cleanup();     // uses your UBO cleanup :contentReference[oaicite:0]{index=0}
			}
			else {
				std::get<GLStorageBuffer>(buf).ssbo.Cleanup();    // uses your SSBO cleanup :contentReference[oaicite:1]{index=1}
			}
		}

		for (auto& [handle, pipeline] : m_pipelines) {
			if (pipeline.program.GetProgram()) {
				glDeleteProgram(pipeline.program.GetProgram());
			}
		}

		m_buffers.clear();
		m_pipelines.clear();
		m_materials.clear();
	};

	GpuBufferHandle CreateBuffer(const BufferCreateInfo& createInfo) override {
		GpuBufferHandle h = m_nextBuf++;
		if (createInfo.usage == BufferUsage::Uniform) {
			GLUniformBuffer u{};
			u.size = createInfo.size;
			u.ubo.Init(createInfo.size);
			u.id = u.ubo.GetBuffer();
			if (createInfo.initialData)
				UpdateBuffer(h, 0, createInfo.initialData, createInfo.size);
			m_buffers.emplace(h, std::move(u));
		}
		else if (createInfo.usage == BufferUsage::Storage) {
			GLStorageBuffer s{};
			s.size = createInfo.size;
			s.ssbo.Init(createInfo.size);
			if (createInfo.initialData)
				UpdateBuffer(h, 0, createInfo.initialData, createInfo.size);
			m_buffers.emplace(h, std::move(s));
		}
		else {
			GLenum tgt = (createInfo.usage == BufferUsage::Index) ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
			GLuint id = 0; glGenBuffers(1, &id);
			glBindBuffer(tgt, id);
			glBufferData(tgt, (GLsizeiptr)createInfo.size, createInfo.initialData, GL_STATIC_DRAW);
			glBindBuffer(tgt, 0);
			m_buffers.emplace(h, GLVertexIndexBuffer{ id, tgt, createInfo.size });
		}
		return h;
	}

	void UpdateBuffer(GpuBufferHandle h, size_t offset, const void* data, size_t size) override {
		auto it = m_buffers.find(h); assert(it != m_buffers.end());
		auto& buf = it->second;
		m_pipelines[0].program.Use();

		if (std::holds_alternative<GLVertexIndexBuffer>(buf)) {
			auto& b = std::get<GLVertexIndexBuffer>(buf);
			glBindBuffer(b.target, b.id);
			glBufferSubData(b.target, (GLintptr)offset, (GLsizeiptr)size, data);
			glBindBuffer(b.target, 0);
		}
		else if (std::holds_alternative<GLUniformBuffer>(buf)) {
			auto& b = std::get<GLUniformBuffer>(buf);
			glBindBuffer(GL_UNIFORM_BUFFER, b.id);
			glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
		else {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	virtual void BindUniformBuffer(GpuBufferHandle h, uint32_t binding) override {
		auto it = m_buffers.find(h); assert(it != m_buffers.end());
		auto& buf = it->second;
		m_pipelines[0].program.Use();

		const auto& u = std::get<GLUniformBuffer>(buf);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, u.id);
	}

	void DestroyBuffer(GpuBufferHandle h) override {
		auto it = m_buffers.find(h); if (it == m_buffers.end()) return;
		if (std::holds_alternative<GLVertexIndexBuffer>(it->second)) {
			auto& b = std::get<GLVertexIndexBuffer>(it->second);
			if (b.id) glDeleteBuffers(1, &b.id);
		}
		else if (std::holds_alternative<GLUniformBuffer>(it->second)) {
			std::get<GLUniformBuffer>(it->second).ubo.Cleanup();       // :contentReference[oaicite:7]{index=7}
		}
		else {
			std::get<GLStorageBuffer>(it->second).ssbo.Cleanup();      // :contentReference[oaicite:8]{index=8}
		}
		m_buffers.erase(it);
	}

	GpuPipelineHandle CreatePipeline(const PipelineDesc& desc) override {
		GLPipeline pipeline{};

		if (desc.backendType != RenderBackendType::OpenGL) {
			Logger::Log(1, "Unsupported backend type for OpenGL pipeline creation\n");
			return 0;
		}

		if (desc.vertexShaderPath.empty() || desc.fragmentShaderPath.empty()) {
			Logger::Log(1, "Vertex and fragment shader paths must be provided for OpenGL pipeline\n");
			return 0;
		}

		pipeline.program.LoadShaders((const char*)desc.vertexShaderPath.c_str(),
			(const char*)desc.fragmentShaderPath.c_str());

		pipeline.vertexStride = desc.vertexStride;

		GpuPipelineHandle handle = m_nextPipe++;
		m_pipelines.emplace(handle, std::move(pipeline));
		pipeline.program.Use();
		return handle;
	}

	GpuMaterialId CreateMaterial(MaterialGpuDesc& desc) override {
		GpuMaterialId id = m_nextMat++;
		m_materials[id] = GpuMaterial{ desc, id };
		return id;
	}

	void UpdateMaterial(GpuMaterialId id, const MaterialGpuDesc& desc) override {
		auto it = m_materials.find(id);
		if (it != m_materials.end()) {
			it->second.desc = desc;
		}
		else {
			Logger::Log(1, "Material ID %u not found for update\n", id);
		}
	}

	void DestroyMaterial(GpuMaterialId materialId) override {
		auto it = m_materials.find(materialId);
		if (it != m_materials.end()) {
			m_materials.erase(it);
		}
		else {
			Logger::Log(1, "Material ID %u not found for destruction\n", materialId);
		}
	};

	void BeginFrame() override {
	}

	void Submit(const DrawItem* items, uint32_t itemCount) override;

	void EndFrame() override {}

	//void UploadCameraMatrices(GpuBufferHandle h, const std::vector<glm::mat4>& mats, int bindingPoint) override {
	//	/*auto& ub = std::get<GLUniformBuffer>(m_buffers.at(h)).ubo;
	//	ub.UploadUboData(mats, bindingPoint);*/
	//	m_cameraData = mats;
	//}

	GpuTextureId CreateTexture2D(const CpuTexture& cpu, GpuMaterialHandle matHandle) override {
		TextureCreateInfo ci{};
		if (cpu.pixels.empty()) {
			Logger::Log(1, "%s: CPU texture has no pixel data\n", __FUNCTION__);
			return 0;
		}

		if (cpu.desc.width == 0 || cpu.desc.height == 0 || cpu.desc.width > 9000) {
			Logger::Log(1, "%s: CPU texture has invalid dimensions %ux%u\n",
				__FUNCTION__, cpu.desc.width, cpu.desc.height);
			return 0;
		}


		ci.width = cpu.desc.width;
		ci.height = cpu.desc.height;
		ci.mipLevels = cpu.desc.mipLevels;
		ci.format = (PixelFormatGpu)cpu.desc.format;
		ci.colorSpace = (ColorSpaceGpu)cpu.desc.colorSpace;
		ci.initialData = cpu.pixels.data();
		ci.initialDataSize = cpu.pixels.size();

		const auto glf = ToGLFmt(ci.format, ci.colorSpace);

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		// Upload one level (assumes tightly packed data).
		// If you need strides, expose that in TextureCreateInfo.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8,
			ci.width, ci.height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE,
			ci.initialData);

		if (ci.mipLevels >= 1) {
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		SamplerDescGpu sdg;
		const auto gls = ToGL(sdg);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindTexture(GL_TEXTURE_2D, 0);

		Logger::Log(1, "Created OpenGL texture %u, size %ux%u, format %d\n",
			tex, ci.width, ci.height, (int)ci.format);

		if ((int)ci.format == 0)
		{
			return 0;
		}

		m_textures[matHandle] = tex;

		Logger::Log(1, "%s: Tex ID: %u, for Material Handle %u\n", __FUNCTION__, tex, matHandle);

		return tex;
	}

private:
	std::unordered_map<GpuPipelineHandle, GLPipeline> m_pipelines;
	std::unordered_map<GpuBufferHandle, GLBuffer> m_buffers;
	std::unordered_map <GpuMaterialId, GpuMaterial> m_materials;
	std::unordered_map <GpuMaterialId, GpuTextureId> m_textures;
	GpuBufferHandle m_nextBuf = 1;
	GpuMaterialId m_nextMat = 1;
	GpuPipelineHandle m_nextPipe = 1;
	std::vector<glm::mat4> m_cameraData; // For camera matrices, can be used in UBOs or SSBOs
};

static RenderBackend* CreateRenderBackend() {
	return new RenderBackendGL();
}