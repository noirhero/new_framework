// Copyright 2018-2020 TAP, Inc. All Rights Reserved.

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cassert>

#include <iostream>
#include <vector>
#include <functional>

#include "Resource.h"

#define FMT_HEADER_ONLY
#include <third_party/fmt/format.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <third_party/VulkanMemoryAllocator/vk_mem_alloc.h>
