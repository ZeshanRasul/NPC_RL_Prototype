#include "glad/glad.h"

#include "GpuUploader.h"
#include "Logger.h"

void GpuUploader::EnsureResident(ModelHandle handle)
{
	if (!handle || m_modelBuffers.find(handle) != m_modelBuffers.end()) return;

	const CpuStaticModel* cpu = m_assetManager->GetCpuStaticModel(handle);
	if(!cpu) {
		Logger::Log(0, "GpuUploader: ModelHandle %u not found in AssetManager\n", handle);
		return;
	}

	GpuModel gpuModel;
	gpuModel.meshes.reserve(cpu->meshes.size());

	for (auto& cm : cpu->meshes) {
		BufferCreateInfo vci{
			cm.vertexData.size(),
			BufferUsage::Vertex,
			cm.vertexData.data()
		};

		BufferCreateInfo ici{
			cm.indexData.size(),
			BufferUsage::Index,
			cm.indexData.data()
		};

		auto vb = m_backend->CreateBuffer(vci);
		auto ib = m_backend->CreateBuffer(ici);

		GpuMeshBuffer gpuMesh{};
		gpuMesh.vertexBuffer = vb;
		gpuMesh.indexBuffer = ib;
		gpuMesh.indexType = cm.index32 ? IndexType::U32 : IndexType::U16;
		gpuMesh.vertexStride = cm.vertexStride;
		gpuMesh.indexCount = cm.indexCount;
		
#ifdef ENGINE_BACKEND_OPENGL
		GLuint vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, (GLuint)gpuMesh.vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)gpuMesh.indexBuffer);

		GLsizei stride = (GLsizei)gpuMesh.vertexStride;

		glEnableVertexAttribArray(0); // Position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1); // Normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
		glEnableVertexAttribArray(2); // TexCoord
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
		glBindVertexArray(0);
		
		gpuMesh.vao = vao;
#endif
		gpuModel.meshes.push_back(std::move(gpuMesh));
	}

	m_modelBuffers[handle] = std::move(gpuModel);
}

