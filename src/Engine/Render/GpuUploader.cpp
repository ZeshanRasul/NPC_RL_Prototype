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
	
	materialHandles = cpu->materials;

	for (auto mat : materialHandles)
	{
	//	EnsureMatResident(mat);
	}

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
			gpuSubmesh.vertexCount = sm.vertexCount;
			gpuSubmesh.firstIndex = sm.firstIndex;
			gpuSubmesh.indexCount = sm.indexCount;
			gpuSubmesh.transform = sm.transform;
			GpuMaterialHandle mh = MatHandle(sm.material);
			gpuSubmesh.material = sm.material;
			EnsureMatResident(sm.material);
			gpuSubmesh.texture = cpu->textures[sm.material];

			GLuint vao = 0;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, (GLuint)gpuSubmesh.vertexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)gpuSubmesh.indexBuffer);

			GLsizei stride = (GLsizei)gpuSubmesh.vertexStride;

			glEnableVertexAttribArray(0); // Position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
			glEnableVertexAttribArray(1); // Normal			stride
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
			glEnableVertexAttribArray(2); // TexCoord		stride
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
			//glEnableVertexAttribArray(3); // TexCoord2		stride
			//glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 8));
			//glEnableVertexAttribArray(4); // TexCoord3		stride
			//glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 10));


			glBindVertexArray(0);

			gpuSubmesh.vao = vao;

			gpuMesh.submeshes.push_back(gpuSubmesh);
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


	MaterialGpuDesc gd;
//	std::memcpy(gd.baseColorFactor, cm->baseColorFactor, sizeof(gd.baseColorFactor));

	gd.baseColorFactor[0] = cm->baseColorFactor[0];
	gd.baseColorFactor[1] = cm->baseColorFactor[1];
	gd.baseColorFactor[2] = cm->baseColorFactor[2];
	gd.baseColorFactor[3] = cm->baseColorFactor[3];

	gd.metallic = cm->metallic;
	gd.roughness = cm->roughness;

	//if (cm->baseColorH == InvalidHandle) return;

	gd.baseColor = EnsureTextureResident(cm->baseColorH, matHandle);
	Logger::Log(1, "%s Base Color is %u\n", __FUNCTION__, gd.baseColor);
	GpuMaterialHandle mathand = m_backend->CreateMaterial(gd);

	GpuMaterial gpuMaterial{};
	gpuMaterial.desc.baseColor = gd.baseColor;
	gpuMaterial.desc.baseColorFactor[0] = gd.baseColorFactor[0];
	gpuMaterial.desc.baseColorFactor[1] = gd.baseColorFactor[1];
	gpuMaterial.desc.baseColorFactor[2] = gd.baseColorFactor[2];
	gpuMaterial.desc.baseColorFactor[3] = gd.baseColorFactor[3];
	gpuMaterial.handle = mathand;
	Logger::Log(1, "%s Mat Handle is is %u\n", __FUNCTION__, gpuMaterial.handle);
	Logger::Log(1, "%s Base Color is %u\n", __FUNCTION__, gpuMaterial.desc.baseColor);
	Logger::Log(1, "%s Base Color Factor is %f, %f, %f, %f \n", __FUNCTION__, gpuMaterial.desc.baseColorFactor[0], gpuMaterial.desc.baseColorFactor[1], gpuMaterial.desc.baseColorFactor[2], gpuMaterial.desc.baseColorFactor[3]);

	m_materialBuffers.emplace(mathand, gpuMaterial);
	m_TextureCache.emplace(mathand, gd.baseColor);
}
//
//const CpuTexture* GpuUploader::EnsureTexResident(TextureHandle texHandle)
//{
//	if (texHandle == 0 || m_materialBuffers.count(texHandle)) return nullptr;
//	const CpuTexture* ct = m_assetManager->GetCpuTexture(texHandle);
//	if (!ct) return nullptr;
//	return ct;
//}

