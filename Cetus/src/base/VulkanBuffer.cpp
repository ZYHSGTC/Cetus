#include "VulkanBuffer.h"

namespace Cetus
{	
	// map方法，用于将缓冲区的一部分或全部映射到主机内存(获得一个主机内存)
	VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	// unmap方法，用于取消缓冲区的映射，释放主机内存
	void Buffer::unmap()
	{
		if (mapped)							// 如果映射内存不为空
		{
			vkUnmapMemory(device, memory);	// 调用Vulkan API的vkUnmapMemory函数，传入设备句柄和内存句柄，取消映射
			mapped = nullptr;				// 将映射内存的指针置为空
		}
	}

	// bind方法，用于将缓冲区绑定到设备内存的指定偏移量
	VkResult Buffer::bind(VkDeviceSize offset)
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	// setupDescriptor方法，用于设置缓冲区的描述符信息，根据指定的大小和偏移量
	void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
	{
		descriptor.offset = offset;			// 将描述符信息的偏移量设为参数中的偏移量
		descriptor.buffer = buffer;			// 将描述符信息的缓冲区设为当前缓冲区
		descriptor.range = size;			// 将描述符信息的范围设为参数中的大小
	}

	// copyTo方法，用于将指定大小的数据从主机内存复制到缓冲区的映射内存
	void Buffer::copyTo(void* data, VkDeviceSize size)
	{
		assert(mapped);						// 断言映射内存不为空
		memcpy(mapped, data, size);			// 调用标准库的memcpy函数，传入映射内存的指针、数据的指针和大小，进行内存复制
	}

	// flush方法，用于将缓冲区的映射内存刷新到设备内存
	VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;		// 将结构体的内存设为当前内存
		mappedRange.offset = offset;		// 将结构体的偏移量设为参数中的偏移量
		mappedRange.size = size;			// 将结构体的大小设为参数中的大小
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	// invalidate方法，用于将缓冲区的设备内存失效，使得映射内存能够获取最新的数据
	VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	// destroy方法，用于销毁缓冲区和释放内存，清理资源
	void Buffer::destroy()
	{
		if (buffer)							// 如果缓冲区不为空
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
		if (memory)							// 如果内存不为空
		{
			vkFreeMemory(device, memory, nullptr);
		}
	}
};
