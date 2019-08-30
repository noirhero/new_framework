// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <corecrt_math_defines.h>

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <gli/gli.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
