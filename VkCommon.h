// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
#define VK_CHECK_RESULT(f) \
	{ \
		VkResult res = (f); \
		if (res != VK_SUCCESS) \
		{ \
			std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
			assert(res == VK_SUCCESS); \
		} \
	}

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint) \
	{ \
		fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
		if (fp##entrypoint == nullptr) \
		{ \
			exit(1); \
		} \
	}

#define GET_DEVICE_PROC_ADDR(dev, entrypoint) \
	{ \
		fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint)); \
		if (fp##entrypoint == nullptr) \
		{ \
			exit(1); \
		} \
	}

#define KEY_ESCAPE VK_ESCAPE 
#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_W 0x57
#define KEY_A 0x41
#define KEY_S 0x53
#define KEY_D 0x44
#define KEY_P 0x50
#define KEY_SPACE 0x20
#define KEY_KPADD 0x6B
#define KEY_KPSUB 0x6D
#define KEY_B 0x42
#define KEY_F 0x46
#define KEY_L 0x4C
#define KEY_N 0x4E
#define KEY_O 0x4F
#define KEY_T 0x54

	struct Settings
	{
		bool validation = false;
		bool fullscreen = false;
		bool vsync = false;
		bool multiSampling = true;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT;
		uint32_t width = 1280;
		uint32_t height = 800;
		uint32_t renderAhead = 2;
	};
}
