#include "RenderBackendGL.h"

struct empty {};

void RenderBackendGL::Submit(const DrawItem* items, uint32_t itemCount) {
	for (uint32_t i = 0; i < itemCount; ++i) {
		const auto& di = items[i];
		const auto& glPipe = m_pipelines[di.pipeline];
		const auto& vb = m_buffers[di.vertexBuffer];
		const auto& ib = m_buffers[di.indexBuffer];

		const auto& vbuf = std::get<GLVertexIndexBuffer>(vb);
		const auto& ibuf = std::get<GLVertexIndexBuffer>(ib);

		const auto& mat = m_materials[di.materialId];
		const auto& baseColorTex = m_textures[di.materialId];
	//	glDisable(GL_CULL_FACE);
		//Logger::Log(1, "%s base color tex: %u\n", __FUNCTION__, baseColorTex);
		glUseProgram(glPipe.program.GetProgram());

		//GLint bcloc = glGetUniformLocation(glPipe.program.GetProgram(), "uBaseColorFactor");
		//if (bcloc >= 0) glUniform4fv(bcloc, 1, mat.desc.baseColorFactor);
		//
		//GLint mrloc = glGetUniformLocation(glPipe.program.GetProgram(), "uMetallicRoughness");
		//if (mrloc >= 0) glUniform2f(mrloc, mat.desc.metallic, mat.desc.roughness);
		//
		//GLint samplerLoc = glGetUniformLocation(glPipe.program.GetProgram(), "uBaseColorTexture");

		if (baseColorTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, baseColorTex);
			glPipe.program.SetVec3("uBaseColorFactor", mat.desc.baseColorFactor[0], mat.desc.baseColorFactor[1], mat.desc.baseColorFactor[2]);
			glPipe.program.SetVec2("uMetallicRoughness", mat.desc.metallic, mat.desc.roughness);
			glPipe.program.SetBool("useTex", true);
			glPipe.program.SetInt("uBaseColorTexture", 0);
			glPipe.program.SetMat4("Transform", di.transform);
		}
		else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glPipe.program.SetVec3("uBaseColorFactor", mat.desc.baseColorFactor[0], mat.desc.baseColorFactor[1], mat.desc.baseColorFactor[2]);
			glPipe.program.SetVec2("uMetallicRoughness", mat.desc.metallic, mat.desc.roughness);
			glPipe.program.SetBool("useTex", false);
			glPipe.program.SetInt("uBaseColorTexture", 0);
			glPipe.program.SetMat4("Transform", di.transform);
		}

		if (di.vao) {
			glBindVertexArray(di.vao);
			//glDisable(GL_CULL_FACE);

			glDrawElements(GL_TRIANGLES, (GLsizei)di.indexCount,
				di.indexType == IndexType::U32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
				(void*)(uintptr_t)(0 * (di.indexType == IndexType::U32 ? sizeof(uint32_t) : sizeof(uint16_t))));
			glBindVertexArray(0);
		}
		else {
			GLsizei stride = (GLsizei)glPipe.vertexStride;
			glBindBuffer(GL_ARRAY_BUFFER, vbuf.id);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf.id);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)3);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)6);
			//glEnableVertexAttribArray(3);
			//glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)8);
			//glEnableVertexAttribArray(4);
			//glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)10);
			glDisable(GL_CULL_FACE);

			GLenum iType = (di.indexType == IndexType::U32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
			glDrawElements(GL_TRIANGLES, (GLsizei)di.indexCount, iType, (void*)(uintptr_t)(di.firstIndex * (di.indexType == IndexType::U32 ? sizeof(uint32_t) : sizeof(uint16_t))));

		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}