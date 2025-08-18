#include "glad/glad.h"

#include "GpuUploader.h"
#include "Logger.h"

void GpuUploader::EnsureResident(ModelHandle modelHandle)
{
	if (!modelHandle || m_modelBuffers.find(modelHandle) != m_modelBuffers.end()) return;

	const CpuStaticModel* cpu = m_assetManager->GetCpuStaticModel(modelHandle);
	if (!cpu) {
		Logger::Log(0, "GpuUploader: ModelHandle %u not found in AssetManager\n", modelHandle);
		return;
	}

	GpuModel gpuModel;
	gpuModel.meshes.reserve(cpu->meshes.size());

	for (auto& cm : cpu->meshes) {
		
		GpuMeshBuffer gpuMesh{};

		for (const auto& sm : cm.submeshes) {
			BufferCreateInfo vci{
			sm.vertexData.size(),
			BufferUsage::Vertex,
			sm.vertexData.data()
			};

			BufferCreateInfo ici{
				sm.indexData.size(),
				BufferUsage::Index,
				sm.indexData.data()
			};

			auto vb = m_backend->CreateBuffer(vci);
			auto ib = m_backend->CreateBuffer(ici);

			GpuSubmeshBuffer gpuSubmesh{};
			gpuSubmesh.vertexBuffer = vb;
			gpuSubmesh.indexBuffer = ib;
			gpuSubmesh.indexType = sm.index32 ? IndexType::U32 : IndexType::U16;
			gpuSubmesh.vertexStride = sm.vertexStride;

			gpuSubmesh.firstIndex = sm.firstIndex;
			gpuSubmesh.indexCount = sm.indexCount;
			
			MaterialHandle mh = (sm.materialIndex < cpu->materials.size()) ? cpu->materials[sm.materialIndex] : 0;
			gpuSubmesh.material = mh;
			
			EnsureMatResident(mh);
			gpuSubmesh.materialId = MatId(mh);

			GLuint vao = 0;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, (GLuint)gpuSubmesh.vertexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)gpuSubmesh.indexBuffer);

			GLsizei stride = (GLsizei)gpuSubmesh.vertexStride;

			glEnableVertexAttribArray(0); // Position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1); // Normal
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(sizeof(float) * 0));
			glEnableVertexAttribArray(2); // TexCoord
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(sizeof(float) * 0));
			glEnableVertexAttribArray(3); // TexCoord2
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(sizeof(float) * 0));
			glEnableVertexAttribArray(4); // TexCoord3
			glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(sizeof(float) * 0));


			glBindVertexArray(0);

			gpuSubmesh.vao = vao;

			gpuMesh.submeshes.push_back(std::move(gpuSubmesh));
		}

#ifdef ENGINE_BACKEND_OPENGL
#endif

		gpuModel.meshes.push_back(std::move(gpuMesh));
	}


	m_modelBuffers[modelHandle] = std::move(gpuModel);
}

void GpuUploader::EnsureMatResident(MaterialHandle matHandle)
{
	if (matHandle == 0 || m_materialBuffers.count(matHandle)) return;
	
	const CpuMaterial* cm = m_assetManager->GetCpuMaterial(matHandle);
	if (!cm) return;


	MaterialGpuDesc gd{};
	std::memcpy(gd.baseColorFactor, cm->baseColorFactor, sizeof(gd.baseColorFactor));
	gd.metallic = cm->metallic;
	gd.roughness = cm->roughness;

	gd.baseColor = EnsureTextureResident(cm->baseColorH);

	GpuMaterialId matId = m_backend->CreateMaterial(gd);

	GpuMaterial gpuMaterial{};
	gpuMaterial.desc = gd;
	gpuMaterial.id = matId;
	m_materialBuffers.emplace(matHandle, gpuMaterial);
}
//
//const CpuTexture* GpuUploader::EnsureTexResident(TextureHandle texHandle)
//{
//	if (texHandle == 0 || m_materialBuffers.count(texHandle)) return nullptr;
//	const CpuTexture* ct = m_assetManager->GetCpuTexture(texHandle);
//	if (!ct) return nullptr;
//	return ct;
//}

