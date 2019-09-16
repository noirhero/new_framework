// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <corecrt_math_defines.h>

#include <array>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <chrono>
#include <filesystem>
#include <string>
using namespace std::literals::string_literals;

#pragma warning(disable : 4127)
#pragma warning(disable : 4201)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#pragma warning(default : 4201)
#pragma warning(default : 4127)

#pragma warning(disable : 4100)
#pragma warning(disable : 4458)
#include <gli/gli.hpp>
#pragma warning(default : 4458)
#pragma warning(default : 4100)

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
