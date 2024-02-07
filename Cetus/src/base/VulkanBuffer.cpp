#include "VulkanBuffer.h"

namespace Cetus
{	
	// map���������ڽ���������һ���ֻ�ȫ��ӳ�䵽�����ڴ�(���һ�������ڴ�)
	VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	// unmap����������ȡ����������ӳ�䣬�ͷ������ڴ�
	void Buffer::unmap()
	{
		if (mapped)							// ���ӳ���ڴ治Ϊ��
		{
			vkUnmapMemory(device, memory);	// ����Vulkan API��vkUnmapMemory�����������豸������ڴ�����ȡ��ӳ��
			mapped = nullptr;				// ��ӳ���ڴ��ָ����Ϊ��
		}
	}

	// bind���������ڽ��������󶨵��豸�ڴ��ָ��ƫ����
	VkResult Buffer::bind(VkDeviceSize offset)
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	// setupDescriptor�������������û���������������Ϣ������ָ���Ĵ�С��ƫ����
	void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
	{
		descriptor.offset = offset;			// ����������Ϣ��ƫ������Ϊ�����е�ƫ����
		descriptor.buffer = buffer;			// ����������Ϣ�Ļ�������Ϊ��ǰ������
		descriptor.range = size;			// ����������Ϣ�ķ�Χ��Ϊ�����еĴ�С
	}

	// copyTo���������ڽ�ָ����С�����ݴ������ڴ渴�Ƶ���������ӳ���ڴ�
	void Buffer::copyTo(void* data, VkDeviceSize size)
	{
		assert(mapped);						// ����ӳ���ڴ治Ϊ��
		memcpy(mapped, data, size);			// ���ñ�׼���memcpy����������ӳ���ڴ��ָ�롢���ݵ�ָ��ʹ�С�������ڴ渴��
	}

	// flush���������ڽ���������ӳ���ڴ�ˢ�µ��豸�ڴ�
	VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;		// ���ṹ����ڴ���Ϊ��ǰ�ڴ�
		mappedRange.offset = offset;		// ���ṹ���ƫ������Ϊ�����е�ƫ����
		mappedRange.size = size;			// ���ṹ��Ĵ�С��Ϊ�����еĴ�С
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	// invalidate���������ڽ����������豸�ڴ�ʧЧ��ʹ��ӳ���ڴ��ܹ���ȡ���µ�����
	VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	// destroy�������������ٻ��������ͷ��ڴ棬������Դ
	void Buffer::destroy()
	{
		if (buffer)							// �����������Ϊ��
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
		if (memory)							// ����ڴ治Ϊ��
		{
			vkFreeMemory(device, memory, nullptr);
		}
	}
};
