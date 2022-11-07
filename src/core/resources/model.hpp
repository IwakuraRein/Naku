#ifndef MODEL_HPP
#define MODEL_HPP

#include "naku.hpp"
#include "utils/device.hpp"
#include "utils/buffer.hpp"
#include "resources/resource.hpp"
#include "resources/mesh.hpp"

namespace naku {

class Model : public Resource {
public:
	Model(Device& device, const std::string& name, const Mesh& Mesh);
	Model(Device& device, const std::string& name, const std::string& ObjFilePath);
	~Model();
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;
	Model() = default;

	void cmdBind(VkCommandBuffer commandBuffer);
	void cmdDraw(VkCommandBuffer commandBuffer);

	uint32_t vertexCount() const { return _vertexCount; }
	uint32_t indexCount() const { return _indexCount; }

	bool hasIndexBuffer() const { return _hasIndexBuffer; }
	std::string filePath() const { return _filePath; }

	VkDevice device() const { return _device.device(); };


private:
	std::string _filePath;
	std::unique_ptr<Buffer> _vertexBuffer;
	uint32_t _vertexCount;

	bool _hasIndexBuffer{ false };
	std::unique_ptr<Buffer> _indexBuffer;
	uint32_t _indexCount;

	void createVertexBuffer(const std::vector<Vertex>& vertices);
	void createIndexBuffer(const std::vector<uint32_t>& indices);
};

}

#endif // !VKL_MODEL_HPP
