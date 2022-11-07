#include "resources/mesh.hpp"

#include <tiny_obj_loader.h>

#include <string>
#include <iostream>
#include <unordered_map>

namespace naku {

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tangent);
	//attributeDescriptions[3].binding = 0;
	//attributeDescriptions[3].location = 3;
	//attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	//attributeDescriptions[3].offset = offsetof(Vertex, bitangent);
	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, uv);
	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(Vertex, color);
	return attributeDescriptions;
}

size_t Vertex::hash() const {
	size_t seed = 0;
	hashCombine(
		seed,
		position.x,
		position.y,
		position.z,
		normal.x,
		normal.y,
		normal.z,
		tangent.x,
		tangent.y,
		tangent.z,
		uv.x,
		uv.y,
		color.x,
		color.y,
		color.z);
	return seed;
}

std::array<glm::vec3, 2> Mesh::calcTangent(
	const Vertex& v0,
	const Vertex& v1,
	const Vertex& v2) {
	const auto E1 = v1.position - v0.position;
	const auto E2 = v2.position - v0.position;
	glm::vec2 dUV1 = v1.uv - v0.uv;
	glm::vec2 dUV2 = v2.uv - v0.uv;

	float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
	glm::vec3 T{
		f * (dUV2.y * E1.x - dUV1.y * E2.x),
		f * (dUV2.y * E1.y - dUV1.y * E2.y),
		f * (dUV2.y * E1.z - dUV1.y * E2.z),
	};
	T = glm::normalize(T);
	glm::vec3 B{
		f * (-dUV2.x * E1.x + dUV1.x * E2.x),
		f * (-dUV2.x * E1.y + dUV1.x * E2.y),
		f * (-dUV2.x * E1.z + dUV1.x * E2.z),
	};
	B = glm::normalize(B);
	return { T, B };
}

void Mesh::loadObjFile(Mesh* mesh, const std::string& ObjFilePath, const glm::vec3* colorOverwrite, bool reverseWindingOrder) {
	mesh->filePath = ObjFilePath;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, ObjFilePath.c_str())) {
		//throw std::runtime_error(warn + err);
		std::cerr << "Error: Failed to load obj file: " << warn + err << std::endl;
	}

	//std::map<Vertex, uint32_t> uniqueVertices{};
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	std::unordered_map<uint32_t, std::vector<uint32_t>> vert2face;
	std::unordered_map<uint32_t, std::array<glm::vec3, 2>> tangents;
	for (const auto& shape : shapes) {
		uint32_t faceIdx{ 0 };
		for (uint32_t i = 0; i < shape.mesh.indices.size(); i++) {
			const auto& vertInfo = shape.mesh.indices[i];
			Vertex vertex{};

			if (vertInfo.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3 * vertInfo.vertex_index + 0],
					attrib.vertices[3 * vertInfo.vertex_index + 1],
					attrib.vertices[3 * vertInfo.vertex_index + 2],
				};
				if (colorOverwrite) {
					vertex.color = *colorOverwrite;
				}
				else {
					vertex.color = {
						attrib.colors[3 * vertInfo.vertex_index + 0],
						attrib.colors[3 * vertInfo.vertex_index + 1],
						attrib.colors[3 * vertInfo.vertex_index + 2],
					};
				}
			}

			if (vertInfo.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3 * vertInfo.normal_index + 0],
					attrib.normals[3 * vertInfo.normal_index + 1],
					attrib.normals[3 * vertInfo.normal_index + 2],
				};
			}

			if (vertInfo.texcoord_index >= 0) {
				vertex.uv = {
					attrib.texcoords[2 * vertInfo.texcoord_index + 0],
					attrib.texcoords[2 * vertInfo.texcoord_index + 1],
				};
			}

			auto pair = uniqueVertices.find(vertex);
			uint32_t vertIdx;
			if (pair == uniqueVertices.end()) {
				vertIdx = mesh->vertices.size();
				uniqueVertices[vertex] = vertIdx;
				mesh->vertices.push_back(vertex);
			}
			else vertIdx = pair->second;

			if ((mesh->indices.size()) % 3 == 0) { // begining of a face
				faceIdx = mesh->indices.size();
			}
			mesh->indices.push_back(vertIdx);
			if (!MapHas(vert2face, vertIdx))
				vert2face.emplace(vertIdx, std::move(std::vector<uint32_t>{}));
			vert2face[vertIdx].push_back(faceIdx);

			if (reverseWindingOrder) {
				if ((mesh->indices.size()) % 3 == 0) { // end of a face
					auto tmp = mesh->indices[faceIdx];
					mesh->indices[faceIdx] = mesh->indices[faceIdx + 2];
					mesh->indices[faceIdx + 2] = tmp;
				}
			}
		}

		// calculate tangent
		for (uint32_t i = 0; i < mesh->indices.size(); i+=3) {
			const Vertex& v0 = mesh->vertices[mesh->indices[i]];
			const Vertex& v1 = mesh->vertices[mesh->indices[i+1]];
			const Vertex& v2 = mesh->vertices[mesh->indices[i+2]];
			tangents.emplace(i, calcTangent(v0, v1, v2));
		}

		for (auto& pair : vert2face) {
			Vertex& vertex = mesh->vertices[pair.first];
			glm::vec3 tangent{ 0.f };
			//glm::vec3 bitangent{ 0.f };
			for (auto i : pair.second) {
				auto t = tangents[i][0];
				auto b = tangents[i][1];
				if (glm::dot(glm::cross(vertex.normal, t), b) < 0.0f)
					t = t * -1.0f;
				tangent += t;
				//bitangent += b;
			}
			tangent = glm::normalize(tangent - vertex.normal * glm::dot(vertex.normal, tangent));
			vertex.tangent = tangent;
			//vertex.bitangent = bitangent;
		}
	}
}
/*
bool Vertex::operator < (const Vertex& other) const {
	if (*this == other) return false;
	if (position.x < other.position.x) return true;
	else if (position.x > other.position.x) return false;
	if (position.y < other.position.y)
		return true;
	else if (position.y > other.position.y) return false;
	else {
		if (position.z < other.position.z)
			return true;
		else if (position.z > other.position.z) return false;
		else {
			if (normal.x < other.normal.x)
				return true;
			else if (normal.x > other.normal.x) return false;
			else {
				if (normal.y < other.normal.y)
					return true;
				else if (normal.y > other.normal.y) return false;
				else {
					if (normal.z < other.normal.z)
						return true;
					else if (normal.z > other.normal.z) return false;
					else {
						if (uv.x < other.uv.x)
							return true;
						else if (uv.x > other.uv.x) return false;
						else {
							if (uv.y < other.uv.y)
								return true;
							else if (uv.y > other.uv.y) return false;
							else {
								if (color.x < other.color.x)
									return true;
								else if (color.x > other.color.x) return false;
								else {
									if (color.y < other.color.y)
										return true;
									else if (color.y > other.color.y) return false;
									else {
										if (color.z < other.color.z)
											return true;
										else if (color.z >= other.color.z)
											return false;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
*/
}