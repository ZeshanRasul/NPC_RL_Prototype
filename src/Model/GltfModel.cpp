#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GltfModel.h"
#include "Logger.h"

bool GltfModel::LoadModelNoAnim(std::string modelFilename)
{
	m_model = std::make_shared<tinygltf::Model>();

	tinygltf::TinyGLTF gltfLoader;
	std::string loaderErrors;
	std::string loaderWarnings;
	bool result = false;

	result = gltfLoader.LoadASCIIFromFile(m_model.get(), &loaderErrors, &loaderWarnings,
	                                      modelFilename);

	if (!loaderWarnings.empty())
	{
		Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
		            loaderWarnings.c_str());
	}

	if (!loaderErrors.empty())
	{
		Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
		            loaderErrors.c_str());
	}

	if (!result)
	{
		Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
		            modelFilename.c_str());
		return false;
	}

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	CreateVertexBuffers();
	CreateIndexBuffer();

	glBindVertexArray(0);

	return true;
}

bool GltfModel::LoadModel(std::string modelFilename, bool isEnemy)
{
	m_model = std::make_shared<tinygltf::Model>();

	m_filename = modelFilename;

	tinygltf::TinyGLTF gltfLoader;
	std::string loaderErrors;
	std::string loaderWarnings;
	bool result = false;

	result = gltfLoader.LoadASCIIFromFile(m_model.get(), &loaderErrors, &loaderWarnings,
	                                      modelFilename);

	if (!loaderWarnings.empty())
	{
		Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
		            loaderWarnings.c_str());
	}

	if (!loaderErrors.empty())
	{
		Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
		            loaderErrors.c_str());
	}

	if (!result)
	{
		Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
		            modelFilename.c_str());
		return false;
	}

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	CreateVertexBuffers(isEnemy);
	CreateIndexBuffer();

	glBindVertexArray(0);

	GetJointData();
	GetWeightData();
	GetInvBindMatrices();

	m_nodeCount = (int)m_model->nodes.size();
	int rootNode = m_model->scenes.at(0).nodes.at(0);
	Logger::Log(1, "%s: model has %i nodes, root node is %i\n", __FUNCTION__, m_nodeCount, rootNode);

	m_nodeList.resize(m_nodeCount);

	m_rootNode = GltfNode::CreateRoot(rootNode);

	m_nodeList.at(rootNode) = m_rootNode;

	GetNodeData(m_rootNode, glm::mat4(1.0f));
	GetNodes(m_rootNode);

	m_rootNode->PrintTree();

	GetAnimations();

	m_additiveAnimationMask.resize(m_nodeCount);
	m_invertedAdditiveAnimationMask.resize(m_nodeCount);

	std::fill(m_additiveAnimationMask.begin(), m_additiveAnimationMask.end(), true);
	m_invertedAdditiveAnimationMask = m_additiveAnimationMask;
	m_invertedAdditiveAnimationMask.flip();

	return true;
}

Texture GltfModel::LoadTexture(std::string textureFilename, bool flip)
{
	if (!m_tex.LoadTexture(textureFilename, false))
	{
		Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
	}
	Logger::Log(1, "%s: glTF model texture '%s' successfully loaded\n", __FUNCTION__,
	            textureFilename.c_str());

	return m_tex;
}

void GltfModel::CreateVertexBuffers(bool isEnemy)
{
	const tinygltf::Primitive& primitives = m_model->meshes.at(0).primitives.at(0);
	m_vertexVbo.resize(primitives.attributes.size() + 1);
	m_attribAccessors.resize(primitives.attributes.size());

	std::vector<glm::vec3> tangents(m_vertices.size(), glm::vec3(0.0f));

	std::vector<glm::vec3> vertPositions(m_vertices.size(), glm::vec3(0.0f));
	std::vector<glm::vec3> normals(m_vertices.size(), glm::vec3(0.0f));
	std::vector<glm::vec2> texCoords(m_vertices.size(), glm::vec2(0.0f));

	for (const auto& attrib : primitives.attributes)
	{
		const std::string attribType = attrib.first;
		const int accessorNum = attrib.second;

		const tinygltf::Accessor& accessor = m_model->accessors.at(accessorNum);
		const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

		if ((attribType.compare("POSITION") != 0) && (attribType.compare("NORMAL") != 0)
			&& (attribType.compare("TEXCOORD_0") != 0) && (attribType.compare("JOINTS_0") != 0)
			&& (attribType.compare("WEIGHTS_0") != 0) && (attribType.compare("COLOR_0") != 0)
			&& (attribType.compare("COLOR_1") != 0 && (attribType.compare("TEXCOORD_1") != 0)))
		{
			Logger::Log(1, "%s: skipping attribute type %s\n", __FUNCTION__, attribType.c_str());
			continue;
		}

		Logger::Log(1, "%s: data for %s uses accessor %i\n", __FUNCTION__, attribType.c_str(),
		            accessorNum);

		if (attribType.compare("POSITION") == 0) {
			int numPositionEntries = accessor.count;
			//Logger::Log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
			//	numPositionEntries);
		}

		if (attribType.compare("POSITION") == 0) {
			int numPositionEntries = accessor.count;
			//Logger::Log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
			//	numPositionEntries);

			// Extract vertices
			auto positions = reinterpret_cast<const float*>(
				buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

			m_vertices.resize(numPositionEntries);
			vertPositions.resize(numPositionEntries);
			tangents.resize(numPositionEntries);
			normals.resize(numPositionEntries);
			texCoords.resize(numPositionEntries);
			for (int i = 0; i < numPositionEntries; ++i)
			{
				m_vertices[i] = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
				vertPositions[i] = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
			}
		}
		else if (attribType.compare("NORMAL") == 0)
		{
			int numNormalEntries = (int)accessor.count;
			auto normalsData = reinterpret_cast<const float*>(
				buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
			normals.resize(numNormalEntries);
			for (int i = 0; i < numNormalEntries; ++i)
			{
				normals[i] = glm::vec3(normalsData[i * 3 + 0], normalsData[i * 3 + 1], normalsData[i * 3 + 2]);
			}
		}
		else if (attribType.compare("TEXCOORD_0") == 0)
		{
			int numTexCoordEntries = (int)accessor.count;
			auto texCoordsData = reinterpret_cast<const float*>(
				buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
			texCoords.resize(numTexCoordEntries);
			for (int i = 0; i < numTexCoordEntries; ++i)
			{
				texCoords[i] = glm::vec2(texCoordsData[i * 2 + 0], texCoordsData[i * 2 + 1]);
			}

			const tinygltf::Accessor& indexAccessor = m_model->accessors[primitives.indices];
			const tinygltf::BufferView& indBufferView = m_model->bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indbuffer = m_model->buffers[bufferView.buffer];

			const unsigned char* dataPtr = indbuffer.data.data() + indBufferView.byteOffset + indexAccessor.byteOffset;

			std::vector<unsigned int> indices(indexAccessor.count);

			if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				auto buf = reinterpret_cast<const unsigned short*>(dataPtr);
				for (size_t i = 0; i < indexAccessor.count; ++i)
				{
					indices[i] = static_cast<unsigned int>(buf[i]);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				auto buf = reinterpret_cast<const unsigned int*>(dataPtr);
				for (size_t i = 0; i < indexAccessor.count; ++i)
				{
					indices[i] = buf[i];
				}
			}

			// Calculate tangents
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				// Get vertex indices
				int idx0 = indices[i];
				int idx1 = indices[i + 1];
				int idx2 = indices[i + 2];

				// Get vertex positions and texture coordinates
				glm::vec3 v0 = vertPositions[idx0];
				glm::vec3 v1 = vertPositions[idx1];
				glm::vec3 v2 = vertPositions[idx2];

				glm::vec2 uv0 = texCoords[idx0];
				glm::vec2 uv1 = texCoords[idx1];
				glm::vec2 uv2 = texCoords[idx2];

				// Calculate edges and delta UVs
				glm::vec3 deltaPos1 = v1 - v0;
				glm::vec3 deltaPos2 = v2 - v0;

				glm::vec2 deltaUV1 = uv1 - uv0;
				glm::vec2 deltaUV2 = uv2 - uv0;

				// Tangent calculation
				float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;

				// Accumulate tangents
				tangents[idx0] += tangent;
				tangents[idx1] += tangent;
				tangents[idx2] += tangent;
			}

			// Normalize tangents
			for (size_t i = 0; i < tangents.size(); ++i)
			{
				tangents[i] = normalize(tangents[i]);
			}
		}


		if (isEnemy)
		{
			m_attribAccessors.at(m_enemyAttributes.at(attribType)) = accessorNum;
		}
		else
		{
			m_attribAccessors.at(m_attributes.at(attribType)) = accessorNum;
		}

		int dataSize = 1;
		switch (accessor.type)
		{
		case TINYGLTF_TYPE_SCALAR:
			dataSize = 1;
			break;
		case TINYGLTF_TYPE_VEC2:
			dataSize = 2;
			break;
		case TINYGLTF_TYPE_VEC3:
			dataSize = 3;
			break;
		case TINYGLTF_TYPE_VEC4:
			dataSize = 4;
			break;
		default:
			Logger::Log(1, "%s error: accessor %i uses data size %i\n", __FUNCTION__,
			            accessorNum, accessor.type);
			break;
		}

		GLuint dataType = GL_FLOAT;
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			dataType = GL_FLOAT;
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			dataType = GL_UNSIGNED_SHORT;
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			dataType = GL_UNSIGNED_BYTE;
			break;
		default:
			//Logger::Log(1, "%s error: accessor %i uses unknown data type %i\n", __FUNCTION__,
		//		accessorNum, accessor.componentType);
			break;
		}

		if (isEnemy)
		{
			glGenBuffers(1, &m_vertexVbo.at(m_enemyAttributes.at(attribType)));
			glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(m_enemyAttributes.at(attribType)));

			glVertexAttribPointer(m_enemyAttributes.at(attribType), dataSize, dataType, GL_FALSE,
			                      0, static_cast<void*>(nullptr));
			glEnableVertexAttribArray(m_enemyAttributes.at(attribType));

			if (attribType == "WEIGHTS_0")
			{
				GLuint tangentBuffer;
				glGenBuffers(1, &tangentBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
				glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), tangents.data(), GL_STATIC_DRAW);

				// Define tangent attribute
				const GLuint tangentLocation = m_enemyAttributes["TANGENT"];
				glVertexAttribPointer(tangentLocation, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(nullptr));
				glEnableVertexAttribArray(tangentLocation);

				// Add to m_vertexVbo
				m_vertexVbo.push_back(tangentBuffer);
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else
		{
			glGenBuffers(1, &m_vertexVbo.at(m_attributes.at(attribType)));
			glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(m_attributes.at(attribType)));

			glVertexAttribPointer(m_attributes.at(attribType), dataSize, dataType, GL_FALSE,
			                      0, static_cast<void*>(nullptr));
			glEnableVertexAttribArray(m_attributes.at(attribType));

			if (attribType == "WEIGHTS_0")
			{
				GLuint tangentBuffer;
				glGenBuffers(1, &tangentBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
				glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), tangents.data(), GL_STATIC_DRAW);

				const GLuint tangentLocation = m_attributes["TANGENT"];

				glVertexAttribPointer(tangentLocation, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(nullptr));
				glEnableVertexAttribArray(tangentLocation);

				m_vertexVbo.push_back(tangentBuffer);
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
}

void GltfModel::CreateIndexBuffer()
{
	glGenBuffers(1, &m_indexVbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVbo);
}

void GltfModel::uploadVertexBuffers()
{
	for (int i = 0; i < 9; ++i)
	{
		const tinygltf::Accessor& accessor = m_model->accessors.at(i);
		const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(i));
		glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength,
		             &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void GltfModel::uploadVertexBuffersMap() {
	for (int i = 0; i < 1; ++i) {
		const tinygltf::Accessor& accessor = m_model->accessors.at(i);
		const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(i));
		glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength,
			&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void GltfModel::uploadEnemyVertexBuffers() {
	for (int i = 0; i < 7; ++i) {
		const tinygltf::Accessor& accessor = m_model->accessors.at(i);
		const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(i));
		glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength,
		             &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


void GltfModel::uploadVertexBuffersNoAnimations()
{
	for (int i = 0; i < 3; ++i)
	{
		const tinygltf::Accessor& accessor = m_model->accessors.at(i);
		const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo.at(i));
		glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength,
		             &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void GltfModel::uploadIndexBuffer()
{
	/* buffer for vertex indices */
	const tinygltf::Primitive& primitives = m_model->meshes.at(0).primitives.at(0);
	const tinygltf::Accessor& indexAccessor = m_model->accessors.at(primitives.indices);
	const tinygltf::BufferView& indexBufferView = m_model->bufferViews.at(indexAccessor.bufferView);
	const tinygltf::Buffer& indexBuffer = m_model->buffers.at(indexBufferView.buffer);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferView.byteLength,
	             &indexBuffer.data.at(0) + indexBufferView.byteOffset, GL_STATIC_DRAW);
}

int GltfModel::getJointMatrixSize()
{
	return (int)m_jointMatrices.size();
}

std::vector<glm::mat4> GltfModel::getJointMatrices()
{
	return m_jointMatrices;
}

int GltfModel::getJointDualQuatsSize()
{
	return (int)m_jointDualQuats.size();
}

std::vector<glm::mat2x4> GltfModel::getJointDualQuats()
{
	return m_jointDualQuats;
}

int GltfModel::GetTriangleCount()
{
	const tinygltf::Primitive& primitives = m_model->meshes.at(0).primitives.at(0);
	const tinygltf::Accessor& indexAccessor = m_model->accessors.at(primitives.indices);

	unsigned int triangles = 0;
	switch (primitives.mode)
	{
	case TINYGLTF_MODE_TRIANGLES:
		triangles = (unsigned int)indexAccessor.count / 3;
		break;
	default:
		//Logger::Log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, primitives.mode);
		break;
	}
	return triangles;
}

void GltfModel::GetAnimations()
{
	for (const auto& anim : m_model->animations)
	{
		Logger::Log(1, "%s: loading animation '%s' with %i channels\n", __FUNCTION__, anim.name.c_str(),
		            anim.channels.size());
		auto clip = std::make_shared<GltfAnimationClip>(anim.name);
		for (const auto& channel : anim.channels)
		{
			clip->AddChannel(m_model, anim, channel);
		}
		m_animClips.push_back(clip);
	}
}

void GltfModel::PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
	double currentTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	if (playBackwards)
	{
		BlendAnimationFrame(animNum, m_animClips.at(animNum)->GetClipEndTime() -
		                    std::fmod(currentTime / 1000.0 * speedDivider,
		                              m_animClips.at(animNum)->GetClipEndTime()), blendFactor);
	}
	else
	{
		BlendAnimationFrame(animNum, std::fmod(currentTime / 1000.0 * speedDivider,
		                                       m_animClips.at(animNum)->GetClipEndTime()), blendFactor);
	}
}

void GltfModel::PlayAnimation(int sourceAnimNumber, int destAnimNumber,
                              float speedDivider, float blendFactor, bool playBackwards)
{
	double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	if (playBackwards)
	{
		CrossBlendAnimationFrame(sourceAnimNumber, destAnimNumber,
		                         m_animClips.at(sourceAnimNumber)->GetClipEndTime() -
		                         std::fmod(currentTime / 1000.0 * speedDivider,
		                                   m_animClips.at(sourceAnimNumber)->GetClipEndTime()), blendFactor);
	}
	else
	{
		CrossBlendAnimationFrame(sourceAnimNumber, destAnimNumber,
		                         std::fmod(currentTime / 1000.0 * speedDivider,
		                                   m_animClips.at(sourceAnimNumber)->GetClipEndTime()), blendFactor);
	}
}

void GltfModel::BlendAnimationFrame(int animNum, float time, float blendFactor)
{
	m_animClips.at(animNum)->BlendAnimationFrame(m_nodeList, m_additiveAnimationMask, time, blendFactor);
	UpdateNodeMatrices(m_rootNode, glm::mat4(1.0f));
}

void GltfModel::CrossBlendAnimationFrame(int sourceAnimNumber, int destAnimNumber, float time,
                                         float blendFactor)
{
	float sourceAnimDuration = m_animClips.at(sourceAnimNumber)->GetClipEndTime();
	float destAnimDuration = m_animClips.at(destAnimNumber)->GetClipEndTime();
	float scaledTime = time * (destAnimDuration / sourceAnimDuration);

	m_animClips.at(sourceAnimNumber)->SetAnimationFrame(m_nodeList, m_additiveAnimationMask, time);
	m_animClips.at(destAnimNumber)->BlendAnimationFrame(m_nodeList, m_additiveAnimationMask, scaledTime, blendFactor);

	m_animClips.at(destAnimNumber)->SetAnimationFrame(m_nodeList, m_invertedAdditiveAnimationMask, scaledTime);
	m_animClips.at(sourceAnimNumber)->BlendAnimationFrame(m_nodeList, m_invertedAdditiveAnimationMask, time, blendFactor);

	UpdateNodeMatrices(m_rootNode, glm::mat4(1.0f));
}

float GltfModel::GetAnimationEndTime(int animNum)
{
	return m_animClips.at(animNum)->GetClipEndTime();
}

std::string GltfModel::GetClipName(int animNum)
{
	return m_animClips.at(animNum)->GetClipName();
}

void GltfModel::draw(Texture tex)
{
	const tinygltf::Primitive& primitives = m_model->meshes.at(0).primitives.at(0);
	const tinygltf::Accessor& indexAccessor = m_model->accessors.at(primitives.indices);

	GLuint drawMode = GL_TRIANGLES;
	switch (primitives.mode)
	{
	case TINYGLTF_MODE_TRIANGLES:
		drawMode = GL_TRIANGLES;
		break;
	default:
		//Logger::Log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, primitives.mode);
		break;
	}

	tex.Bind();
	glBindVertexArray(m_vao);
	glDrawElements(drawMode, indexAccessor.count, indexAccessor.componentType, nullptr);
	glBindVertexArray(0);
	tex.Unbind();
}

void GltfModel::drawNoTex() {
	//const tinygltf::Primitive& primitives = mModel->meshes.at(0).primitives.at(0);
	//const tinygltf::Accessor& indexAccessor = mModel->accessors.at(primitives.indices);
	//
	//GLuint drawMode = GL_TRIANGLES;
	//switch (primitives.mode) {
	//case TINYGLTF_MODE_TRIANGLES:
	//	drawMode = GL_TRIANGLES;
	//	break;
	//default:
	//	Logger::Log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, primitives.mode);
	//	break;
	//}

	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 132804);
	glBindVertexArray(0);
}

void GltfModel::GetJointData() {
	std::string jointsAccessorAttrib = "JOINTS_0";
	int jointsAccessor = m_model->meshes.at(0).primitives.at(0).attributes.at(jointsAccessorAttrib);
	//Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, jointsAccessor,
		//jointsAccessorAttrib.c_str());

	const tinygltf::Accessor& accessor = m_model->accessors.at(jointsAccessor);
	const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
	const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

	int jointVecSize = accessor.count;
	//Logger::Log(1, "%s: %i short vec4 in JOINTS_0\n", __FUNCTION__, jointVecSize);
	m_jointVec.resize(jointVecSize);

	std::memcpy(m_jointVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
	            bufferView.byteLength);

	m_nodeToJoint.resize(m_model->nodes.size());

	const tinygltf::Skin& skin = m_model->skins.at(0);
	for (int i = 0; i < skin.joints.size(); ++i)
	{
		int destinationNode = skin.joints.at(i);
		m_nodeToJoint.at(destinationNode) = i;
		//Logger::Log(2, "%s: joint %i affects node %i\n", __FUNCTION__, i, destinationNode);
	}
}

void GltfModel::GetWeightData()
{
	std::string weightsAccessorAttrib = "WEIGHTS_0";
	int weightAccessor = m_model->meshes.at(0).primitives.at(0).attributes.at(weightsAccessorAttrib);
	//Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, weightAccessor,
	//	weightsAccessorAttrib.c_str());

	const tinygltf::Accessor& accessor = m_model->accessors.at(weightAccessor);
	const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
	const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

	int weightVecSize = accessor.count;
	//Logger::Log(1, "%s: %i vec4 in WEIGHTS_0\n", __FUNCTION__, weightVecSize);
	m_weightVec.resize(weightVecSize);

	std::memcpy(m_weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
	            bufferView.byteLength);
}

void GltfModel::GetInvBindMatrices()
{
	const tinygltf::Skin& skin = m_model->skins.at(0);
	int invBindMatAccessor = skin.inverseBindMatrices;

	const tinygltf::Accessor& accessor = m_model->accessors.at(invBindMatAccessor);
	const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
	const tinygltf::Buffer& buffer = m_model->buffers.at(bufferView.buffer);

	m_inverseBindMatrices.resize(skin.joints.size());
	m_jointMatrices.resize(skin.joints.size());
	m_jointDualQuats.resize(skin.joints.size());

	std::memcpy(m_inverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset,
	            bufferView.byteLength);
}

void GltfModel::GetNodes(std::shared_ptr<GltfNode> treeNode)
{
	int nodeNum = treeNode->GetNodeNum();
	std::vector<int> childNodes = m_model->nodes.at(nodeNum).children;

	/* remove the child node with skin/mesh metadata, confuses skeleton */
	auto removeIt = std::remove_if(childNodes.begin(), childNodes.end(),
	                               [&](int num) { return m_model->nodes.at(num).skin != -1; }
	);
	childNodes.erase(removeIt, childNodes.end());

	treeNode->AddChilds(childNodes);
	glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();

	for (auto& childNode : treeNode->GetChilds())
	{
		m_nodeList.at(childNode->GetNodeNum()) = childNode;
		GetNodeData(childNode, treeNodeMatrix);
		GetNodes(childNode);
	}
}

void GltfModel::GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
	int nodeNum = treeNode->GetNodeNum();
	const tinygltf::Node& node = m_model->nodes.at(nodeNum);
	treeNode->SetNodeName(node.name);

	if (node.translation.size())
	{
		treeNode->SetTranslation(glm::make_vec3(node.translation.data()));
	}
	if (node.rotation.size())
	{
		treeNode->SetRotation(glm::make_quat(node.rotation.data()));
	}
	if (node.scale.size())
	{
		treeNode->SetScale(glm::make_vec3(node.scale.data()));
	}

	treeNode->CalculateLocalTrsMatrix();
	treeNode->CalculateNodeMatrix(parentNodeMatrix);

	UpdateJointMatricesAndQuats(treeNode);
}

std::string GltfModel::GetNodeName(int nodeNum)
{
	if (nodeNum >= 0 && nodeNum < (m_nodeList.size()) && m_nodeList.at(nodeNum))
	{
		return m_nodeList.at(nodeNum)->GetNodeName();
	}
	return "(Invalid)";
}

void GltfModel::ResetNodeData()
{
	GetNodeData(m_rootNode, glm::mat4(1.0f));
	ResetNodeData(m_rootNode, glm::mat4(1.0f));
}

void GltfModel::ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
	glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();
	for (auto& childNode : treeNode->GetChilds())
	{
		GetNodeData(childNode, treeNodeMatrix);
		ResetNodeData(childNode, treeNodeMatrix);
	}
}

void GltfModel::UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
	treeNode->CalculateNodeMatrix(parentNodeMatrix);
	UpdateJointMatricesAndQuats(treeNode);

	glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();

	for (auto& childNode : treeNode->GetChilds())
	{
		UpdateNodeMatrices(childNode, treeNodeMatrix);
	}
}

void GltfModel::UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode)
{
	int nodeNum = treeNode->GetNodeNum();
	m_jointMatrices.at(m_nodeToJoint.at(nodeNum)) =
		treeNode->GetNodeMatrix() * m_inverseBindMatrices.at(m_nodeToJoint.at(nodeNum));

	/* extract components from node matrix */
	glm::quat orientation;
	glm::vec3 scale;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::dualquat dq;

	/* create dual quaternion */
	if (decompose(m_jointMatrices.at(m_nodeToJoint.at(nodeNum)), scale, orientation,
	              translation, skew, perspective))
	{
		dq[0] = orientation;
		dq[1] = glm::quat(0.0, translation.x, translation.y, translation.z) * orientation * 0.5f;
		m_jointDualQuats.at(m_nodeToJoint.at(nodeNum)) = mat2x4_cast(dq);
		glm::mat2x4 newDq = m_jointDualQuats.at(m_nodeToJoint.at(nodeNum));
		newDq;
	}
	else {
		//Logger::Log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__,
	//		nodeNum);
	}
}

void GltfModel::UpdateAdditiveMask(std::shared_ptr<GltfNode> treeNode, int splitNodeNum)
{
	if (treeNode->GetNodeNum() == splitNodeNum)
	{
		return;
	}

	m_additiveAnimationMask.at(treeNode->GetNodeNum()) = false;
	for (auto& childNode : treeNode->GetChilds())
	{
		UpdateAdditiveMask(childNode, splitNodeNum);
	}
}

void GltfModel::cleanup()
{
	glDeleteBuffers(m_vertexVbo.size(), m_vertexVbo.data());
	glDeleteBuffers(1, &m_vao);
	glDeleteBuffers(1, &m_indexVbo);
	m_tex.Cleanup();
	m_model.reset();
	m_nodeList.clear();
}
