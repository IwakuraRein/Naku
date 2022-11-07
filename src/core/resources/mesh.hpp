#ifndef MESH_HPP
#define MESH_HPP

#include "naku.hpp"

namespace naku {
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	//glm::vec3 bitangent;
	glm::vec2 uv;
	glm::vec3 color;
	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	bool operator==(const Vertex& other) const {
		return position == other.position && color == other.color && normal == other.normal &&
			uv == other.uv;
	}

	// used for map
	//bool operator < (const Vertex& other) const;
	// used for unordered_map
	size_t hash() const;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::string filePath;

	static void loadObjFile(
		Mesh* mesh,
		const std::string& ObjFilePath,
		const glm::vec3* colorOverwrite = nullptr,
		bool reverseWindingOrder = false);
	static std::array<glm::vec3, 2> calcTangent(
		const Vertex& v0,
		const Vertex& v1,
		const Vertex& v2);
};

}

namespace std {
template <>
struct hash<naku::Vertex> {
	size_t operator()(naku::Vertex const& vertex) const {
		return vertex.hash();
	}
};

}

#endif