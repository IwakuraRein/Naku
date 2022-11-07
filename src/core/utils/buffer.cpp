//#define VMA_IMPLEMENTATION
#include "utils/buffer.hpp"

#include <cassert>
#include <iostream>
#include <cstring>

namespace naku {

/**
 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
VkDeviceSize Buffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
	if (minOffsetAlignment > 0) {
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

Buffer::Buffer(
	Device& device,
	VkDeviceSize instanceSize,
	uint32_t instanceCount,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memoryPropertyFlags,
	VkDeviceSize minOffsetAlignment)
	: _device{ device },
	_instanceSize{ instanceSize },
	_instanceCount{ instanceCount },
	_usageFlags{ usageFlags },
	_memoryPropertyFlags{ memoryPropertyFlags } {

	_alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
	_bufferSize = _alignmentSize * instanceCount;
	_allocation = createBuffer(_device, _bufferSize, usageFlags, _buffer, memoryPropertyFlags);
	VmaAllocationInfo allocInfo{};
	vmaGetAllocationInfo(_device.allocator(), _allocation, &allocInfo);
	_mapped = allocInfo.pMappedData;
}

Buffer::~Buffer() {
	vmaDestroyBuffer(_device.allocator(), _buffer, _allocation);
}

VmaAllocation Buffer::createBuffer(
	Device& device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkBuffer& buffer,
	VkMemoryPropertyFlags memoryPropertyFlags) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	else if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.preferredFlags = memoryPropertyFlags;
	VmaAllocation allocation;
	if (vmaCreateBuffer(device.allocator(), &bufferInfo, &allocCreateInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Error: Failed to create buffer!");
	}
	return allocation;
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
VkResult Buffer::map() {
	assert(_buffer && _allocation && "Called map on buffer before create");
	return vmaMapMemory(_device.allocator(), _allocation, &_mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void Buffer::unmap() {
	if (_mapped) {
		vmaUnmapMemory(_device.allocator(), _allocation);
		_mapped = nullptr;
	}
}

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void Buffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
	assert(_mapped && "Cannot copy to unmapped buffer");

	if (size == VK_WHOLE_SIZE) {
		memcpy(_mapped, data, _bufferSize);
	}
	else {
		char* memOffset = (char*)_mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
	return vmaFlushAllocation(_device.allocator(), _allocation, offset, size);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
	return vmaInvalidateAllocation(_device.allocator(), _allocation, offset, size);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
	return VkDescriptorBufferInfo{
		_buffer,
		offset,
		size,
	};
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void Buffer::writeToIndex(void* data, int index) {
	writeToBuffer(data, _instanceSize, index * _alignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult Buffer::flushIndex(int index) { return flush(_alignmentSize, index * _alignmentSize); }

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo Buffer::descriptorInfoForIndex(int index) {
	return descriptorInfo(_alignmentSize, index * _alignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult Buffer::invalidateIndex(int index) {
	return invalidate(_alignmentSize, index * _alignmentSize);
}

void Buffer::copyTo(const Buffer& dstBuffer, VkDeviceSize size, VkDeviceSize srcOffsett, VkDeviceSize dstOffest) {
	VkCommandBuffer commandBuffer = _device.beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffsett;  // Optional
	copyRegion.dstOffset = dstOffest;  // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, _buffer, dstBuffer.getBuffer(), 1, &copyRegion);

	_device.endSingleTimeCommands(commandBuffer);
}

}
