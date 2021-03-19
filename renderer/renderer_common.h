// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

#ifdef _DEBUG
#define TEST(expr) \
    do { \
        if(!(expr)) { \
            assert(0 && #expr); \
        } \
    } while(0)
#else
#define TEST(expr) \
    do { \
        if(!(expr)) { \
            throw std::runtime_error("TEST FAILED: " #expr); \
        } \
    } while(0)
#endif

#ifdef _DEBUG
#define VK_CHECK(result) assert(VK_SUCCESS == (result))
#else
#define VK_CHECK(result) result
#endif

#ifdef _DEBUG
#define BOOL_CHECK(result) assert(true == (result))
#else
#define BOOL_CHECK(result) result
#endif

using VkLayerPropertyArray               = std::vector<VkLayerProperties>;
using VkPhysicalDevices                  = std::vector<VkPhysicalDevice>;
using VkExtensionPropertyArray           = std::vector<VkExtensionProperties>;
using VkQueueFamilyPropertiesArray       = std::vector<VkQueueFamilyProperties>;
using VkSurfaceFormats                   = std::vector<VkSurfaceFormatKHR>;
using VkPresentModes                     = std::vector<VkPresentModeKHR>;
using VkImages                           = std::vector<VkImage>;
using VkImageViews                       = std::vector<VkImageView>;
using VkFrameBuffers                     = std::vector<VkFramebuffer>;
using VkWriteDescriptorSets              = std::vector<VkWriteDescriptorSet>;
using VkCommandBuffers                   = std::vector<VkCommandBuffer>;
using VkVertexInputAttributeDescriptions = std::vector< VkVertexInputAttributeDescription>;

constexpr uint32_t INVALID_IDX           = std::numeric_limits<uint32_t>::max();
