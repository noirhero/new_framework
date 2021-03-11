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
#define VK_CHECK(result) result;
#endif

using VkExtensionPropertyArray     = std::vector<VkExtensionProperties>;
using VkQueueFamilyPropertiesArray = std::vector<VkQueueFamilyProperties>;
using VkImages                     = std::vector<VkImage>;
using VkImageViews                 = std::vector<VkImageView>;
using VkFrameBuffers               = std::vector<VkFramebuffer>;
