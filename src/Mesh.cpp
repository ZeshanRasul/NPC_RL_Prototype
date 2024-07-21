//#include "Mesh.h"
//
//Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, float shininess)
//{
//	this->vertices = vertices;
//	this->indices = indices;
//	this->textures = textures;
//	this->shininess = shininess;
//
//	setupMesh();
//}
//
//void Mesh::Draw(ShaderOld& shader)
//{
//	unsigned int diffuseNum = 1;
//	unsigned int specularNum = 1;
//
//	for (unsigned int i = 0; i < textures.size(); i++)
//	{
//		glActiveTexture(GL_TEXTURE0 + i);
//
//		std::string number;
//		std::string name = textures[i].type;
//
//		if (name == "texture_diffuse")
//			number = std::to_string(diffuseNum++);
//		else if (name == "texture_specular")
//			number = std::to_string(specularNum++);
//
//		shader.setInt(("material." + name + number).c_str(), i);
//		glBindTexture(GL_TEXTURE_2D, textures[i].id);
//	}
//
//	shader.setFloat("material.shininess", shininess);
//
//	glBindVertexArray(VAO);
//	glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
//	glBindVertexArray(0);
//}
//
//void Mesh::setupMesh()
//{
//	glGenVertexArrays(1, &VAO);
//	glGenBuffers(1, &VBO);
//	glGenBuffers(1, &EBO);
//
//	glBindVertexArray(VAO);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
//
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
//
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
//
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
//
//	glEnableVertexAttribArray(2);
//	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
//
//	glBindVertexArray(0);
//}