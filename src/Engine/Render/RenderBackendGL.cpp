#include <vector>
#include <unordered_map>
#include <glad/glad.h>

#include "Engine/Render/RenderBackend.h"
#include "Logger.h"

#define ENGINE_BACKEND_OPENGL

struct GLBuffer {
	GLuint id = 0;
	GLenum target = GL_ARRAY_BUFFER;
	size_t size = 0;
};

struct GLMat {
	MaterialGpuDesc desc;
};

struct GLPipe {
	GLuint prog = 0;
	uint32_t vertexStride = 0;
};

class RenderBackendGL : public RenderBackend {
public:
	bool Initialize() override {};
	void Shutdown() override {
		for (auto& [handle, buffer] : m_buffers) {
			if (buffer.id) {
				glDeleteBuffers(1, &buffer.id);
			}
		}

		for (auto& [handle, pipe] : m_pipelines) {
			if (pipe.prog) {
				glDeleteProgram(pipe.prog);
			}
		}
	};

	GpuBufferHandle CreateBuffer(const BufferCreateInfo& createInfo) override {
		GLuint id = 0;
		GLenum target = (createInfo.usage == BufferUsage::Vertex) ? GL_ARRAY_BUFFER :
			(createInfo.usage == BufferUsage::Index) ? GL_ELEMENT_ARRAY_BUFFER :
			GL_ARRAY_BUFFER;

		glGenBuffers(1, &id);
		glBindBuffer(target, id);
		glBufferData(target, (GLsizeiptr)createInfo.size, createInfo.data, GL_STATIC_DRAW);
		glBindBuffer(target, 0);

		GpuBufferHandle handle = ++m_nextBuf;
		m_buffers[handle] = { id, target, createInfo.size };
		return handle;
	}

	void DestroyBuffer(GpuBufferHandle buffer) override {
		auto it = m_buffers.find(buffer);
		if (it != m_buffers.end()) {
			glDeleteBuffers(1, &it->second.id);
			m_buffers.erase(it);
		}
	}

	GpuPipelineHandle CreatePipeline(const PipelineDesc& desc) override {
		GLuint prog = glCreateProgram();
		if (desc.vertexShaderId) {
			glAttachShader(prog, desc.vertexShaderId);
		}
		if (desc.fragmentShaderId) {
			glAttachShader(prog, desc.fragmentShaderId);
		}
		if (desc.computeShaderId) {
			glAttachShader(prog, desc.computeShaderId);
		}
		glLinkProgram(prog);
		GLint linkStatus;
		glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE) {
			GLint logLength;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
			std::vector<char> log(logLength);
			glGetProgramInfoLog(prog, logLength, nullptr, log.data());
			Logger::Log(1, "Error linking program: %s\n", log.data());
			glDeleteProgram(prog);
			return 0;
		}
		GpuPipelineHandle handle = ++m_nextPipe;
		m_pipelines[handle] = GLPipe{ prog, desc.vertexStride };
		return handle;
	}

	GpuMaterialId CreateMaterial(const MaterialGpuDesc& desc) {
		GpuMaterialId id = ++m_nextMat;
		m_materials[id] = GLMat{ desc };
		return id;
	}

	void UpdateMaterial(GpuMaterialId id, const MaterialGpuDesc& desc) {
		auto it = m_materials.find(id);
		if (it != m_materials.end()) {
			it->second.desc = desc;
		} else {
			Logger::Log(1, "Material ID %u not found for update\n", id);
		}
	}

	void DestroyMaterial(GpuMaterialId materialId) {
		auto it = m_materials.find(materialId);
		if (it != m_materials.end()) {
			m_materials.erase(it);
		} else {
			Logger::Log(1, "Material ID %u not found for destruction\n", materialId);
		}
	}

	void BeginFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Submit(const DrawItem* items, uint32_t itemCount) override {
		for (uint32_t i = 0; i < itemCount; ++i) {
			const auto& di = items[i];
			const auto& glPipe = m_pipelines[di.pipeline];
			const auto& vb = m_buffers[di.vertexBuffer];
			const auto& ib = m_buffers[di.indexBuffer];

			glUseProgram(glPipe.prog);
			if (di.vao) {
				glBindVertexArray(di.vao);
				glDrawElements(GL_TRIANGLES, di.indexCount, 
					di.indexType == IndexType::U32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
					(void*)(di.firstIndex * (di.indexType == IndexType::U32 ? sizeof(uint32_t) : sizeof(uint16_t))));
				glBindVertexArray(0);
			} else {
				GLsizei stride = (GLsizei)glPipe.vertexStride;
				glBindBuffer(GL_ARRAY_BUFFER, vb.id);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.id);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,stride, (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

				GLenum iType = (di.indexType == IndexType::U32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
				glDrawElements(GL_TRIANGLES, (GLsizei)di.indexCount, iType, (void*)(uintptr_t)(di.firstIndex * (di.indexType == IndexType::U32 ? sizeof(uint32_t) : sizeof(uint16_t))));

			}
		}
	}

	void EndFrame() override {}

private:
	std::unordered_map<GpuBufferHandle, GLBuffer> m_buffers;
	std::unordered_map<GpuMaterialId, GLMat> m_materials;
	std::unordered_map<GpuPipelineHandle, GLPipe> m_pipelines;
	GpuBufferHandle m_nextBuf = 1;
	GpuMaterialId m_nextMat = 1;
	GpuPipelineHandle m_nextPipe = 1;
};

RenderBackend* CreateRenderBackend() {
	return new RenderBackendGL();
}