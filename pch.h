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
using namespace std::string_literals;

#include "Resource.h"

#pragma warning(disable : 4819)
#define FMT_HEADER_ONLY
#include <third_party/fmt/format.h>
#pragma warning(default : 4819)

#define VK_USE_PLATFORM_WIN32_KHR
#pragma comment(lib, "vulkan-1.lib")
#include <vulkan/vulkan.h>

#pragma warning(disable : 4100)
#pragma warning(disable : 4127)
#pragma warning(disable : 4189)
#pragma warning(disable : 4324)
#include <third_party/VulkanMemoryAllocator/vk_mem_alloc.h>
#pragma warning(default : 4100)
#pragma warning(default : 4189)
#pragma warning(default : 4127)
#pragma warning(default : 4324)

#pragma warning(disable : 4201)
#define GLM_FORCE_RADIANS
#include <third_party/glm/glm.hpp>
#include <third_party/glm/ext.hpp>
#pragma warning(default : 4201)

#include "util_output.h"
#include "util_path.h"
#include "util_file.h"
