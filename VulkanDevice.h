// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    struct VulkanDevice {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice logicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceFeatures enabledFeatures{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkCommandPool commandPool = VK_NULL_HANDLE;

        struct {
            uint32_t graphics = 0;
            uint32_t compute = 0;
        } queueFamilyIndices;

        operator VkDevice() { return logicalDevice; };

        /**
        * Default constructor
        *
        * @param physicalDevice Physical device that is to be used
        */
        VulkanDevice(VkPhysicalDevice physicalDevice);

        /**
        * Default destructor
        *
        * @note Frees the logical device
        */
        ~VulkanDevice();

        /**
        * Get the index of a memory type that has all the requested property bits set
        *
        * @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
        * @param inProperties Bitmask of inProperties for the memory type to request
        * @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
        *
        * @return Index of the requested memory type
        *
        * @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested inProperties
        */
        uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags inProperties, VkBool32* memTypeFound = nullptr);

        /**
        * Get the index of a queue family that supports the requested queue flags
        *
        * @param queueFlags Queue flags to find a queue family index for
        *
        * @return Index of the queue family index that matches the flags
        *
        * @throw Throws an exception if no queue family index could be found that supports the requested flags
        */
        uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags);

        /**
        * Create the logical device based on the assigned physical device, also gets default queue family indices
        *
        * @param inEnabledFeatures Can be used to enable certain features upon device creation
        * @param enabledExtensions
        * @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
        *
        * @return VkResult of the device creation call
        */
        VkResult CreateLogicalDevice(VkPhysicalDeviceFeatures inEnabledFeatures, std::vector<const char*> enabledExtensions, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

        /**
        * Create a buffer on the device
        *
        * @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
        * @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
        * @param size Size of the buffer in byes
        * @param buffer Pointer to the buffer handle acquired by the function
        * @param memory Pointer to the memory handle acquired by the function
        * @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
        *
        * @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
        */
        VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr);

        /**
        * Create a command pool for allocation command buffers from
        *
        * @param queueFamilyIndex Family index of the queue to Create the command pool for
        * @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
        *
        * @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
        *
        * @return A handle to the created command buffer
        */
        VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        /**
        * Allocate a command buffer from the command pool
        *
        * @param level Level of the new command buffer (primary or secondary)
        * @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
        *
        * @return A handle to the allocated command buffer
        */
        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false);

        void BeginCommandBuffer(VkCommandBuffer commandBuffer);

        /**
        * Finish command buffer recording and submit it to a queue
        *
        * @param commandBuffer Command buffer to Flush
        * @param queue Queue to submit the command buffer to
        * @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
        *
        * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
        * @note Uses a fence to ensure command buffer has finished executing
        */
        void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
    };
}
