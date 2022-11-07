#include "resources/model.hpp"

namespace naku {

Model::Model(Device& device, const std::string& name, const Mesh& Mesh)
	: Resource{ device, name }, _filePath{Mesh.filePath} {
	createVertexBuffer(Mesh.vertices);
	createIndexBuffer(Mesh.indices);
}

Model::Model(Device& device, const std::string& name, const std::string& ObjFilePath)
	: Resource{ device, name }, _filePath{ ObjFilePath } {
	Mesh mesh{};

	Mesh::loadObjFile(&mesh, ObjFilePath, nullptr, true);

	if (mesh.vertices.size() >= 3) {
		createVertexBuffer(mesh.vertices);
		createIndexBuffer(mesh.indices);
	}

}

Model::~Model() { }

void Model::createVertexBuffer(const std::vector<Vertex>& vertices) {
	_vertexCount = static_cast<uint32_t>(vertices.size());
	assert(_vertexCount >= 3 && "vertices must be no less than 3.");
	uint32_t vertexSize = sizeof(vertices[0]);
	VkDeviceSize bufferSize = sizeof(vertices[0]) * _vertexCount;

	Buffer stagingBuffer{
		_device,
		vertexSize,
		_vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	//stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	_vertexBuffer = std::make_unique<Buffer>(
		_device,
		vertexSize,
		_vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	stagingBuffer.copyTo(*_vertexBuffer, bufferSize);
}

void Model::createIndexBuffer(const std::vector<uint32_t>& indices) {
	_indexCount = static_cast<uint32_t>(indices.size());
	_hasIndexBuffer = _indexCount > 0;
	uint32_t indexSize = sizeof(indices[0]);
	VkDeviceSize bufferSize = sizeof(indices[0]) * _indexCount;

	if (_hasIndexBuffer) {
		Buffer stagingBuffer{
			  _device,
			  indexSize,
			  _indexCount,
			  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		//stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		_indexBuffer = std::make_unique<Buffer>(
			_device,
			indexSize,
			_indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		stagingBuffer.copyTo(*_indexBuffer, bufferSize);
	}
}

void Model::cmdBind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[] = { _vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	if (_hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, _indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::cmdDraw(VkCommandBuffer commandBuffer) {
	if (_hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, _indexCount, 1, 0, 0, 0);
	}
	else {
		vkCmdDraw(commandBuffer, _vertexCount, 1, 0, 0);
	}
}

}