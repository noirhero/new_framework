// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cassert>

#include <iostream>
#include <vector>
#include <functional>
#include <string>
using namespace std::string_literals;

#include "Resource.h"

#define FMT_HEADER_ONLY
#include <third_party/fmt/format.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#pragma warning(disable : 4100)
#pragma warning(disable : 4127)
#pragma warning(disable : 4324)
#include <third_party/VulkanMemoryAllocator/vk_mem_alloc.h>
#pragma warning(default : 4100)
#pragma warning(default : 4127)
#pragma warning(default : 4324)

#include "util_output.h"
