// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cassert>

#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <string>
#include <filesystem>
#include <atomic>
#include <span>
using namespace std::string_literals;

#include "Resource.h"

#define VK_USE_PLATFORM_WIN32_KHR
#pragma comment(lib, "vulkan-1.lib")
#include <vulkan/vulkan.h>

#pragma warning(disable : 4100)
#pragma warning(disable : 4127)
#pragma warning(disable : 4189)
#pragma warning(disable : 4324)
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#pragma warning(default : 4100)
#pragma warning(default : 4189)
#pragma warning(default : 4127)
#pragma warning(default : 4324)

#pragma warning(disable : 4819)
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#pragma warning(default : 4819)

#pragma warning(disable : 4201)
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#pragma warning(default : 4201)

#pragma warning(disable : 4100)
#pragma warning(disable : 4244)
#pragma warning(disable : 4458)
#pragma warning(disable : 5054)
#include <gli/gli.hpp>
#pragma warning(default : 4100)
#pragma warning(default : 4244)
#pragma warning(default : 4458)
#pragma warning(default : 5054)

#pragma warning(disable : 4245)
#pragma warning(disable : 4267)
#if defined(_DEBUG)
#pragma comment(lib, "gainput-d.lib")
#else
#pragma comment(lib, "gainput.lib")
#endif
#include <gainput/gainput.h>
#pragma warning(default : 4245)
#pragma warning(default : 4267)

#include "util/util_output.h"
#include "util/util_path.h"
#include "util/util_file.h"
#include "util/util_timer.h"

#include "renderer/renderer_common.h"
