#pragma once
#include "vulkan/vulkan.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Cetus
{
	namespace debug
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t srcObject,
			size_t location,
			int32_t msgCode,
			const char* pLayerPrefix,
			const char* pMsg,
			void* pUserData);

		void setupDebugging(VkInstance instance);
		void freeDebugCallback(VkInstance instance);
	}

	namespace debugutils
	{
		void setup(VkInstance instance);
		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
		void cmdEndLabel(VkCommandBuffer cmdbuffer);
	}
}
