// Copyright 2018-2020 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "allocator_cpu.h"

#include "renderer_common.h"

namespace Renderer {
    void* const CALLBACK_USER_DATA = reinterpret_cast<void*>(static_cast<intptr_t>(43564544));
    std::atomic_uint32_t g_allocCount;

    void* CustomCpuAllocation(void* userData, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) {
        assert(CALLBACK_USER_DATA == userData);
        (void)userData;

        void* const result = _aligned_malloc(size, alignment);
        if (result) {
            ++g_allocCount;
        }
        return result;
    }

    void* CustomCpuReallocation(void* userData, void* original, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) {
        assert(CALLBACK_USER_DATA == userData);
        (void)userData;

        void* const result = _aligned_realloc(original, size, alignment);
        if (original && !result) {
            --g_allocCount;
        }
        else if (!original && result) {
            ++g_allocCount;
        }
        return result;
    }

    static void CustomCpuFree(void* userData, void* memory) {
        assert(CALLBACK_USER_DATA == userData);
        (void)userData;

        if (nullptr != memory) {
            const uint32_t oldAllocCount = g_allocCount.fetch_sub(1);
            TEST(oldAllocCount > 0);

            _aligned_free(memory);
        }
    }

    const VkAllocationCallbacks g_cpuAllocator = { CALLBACK_USER_DATA, &CustomCpuAllocation, &CustomCpuReallocation, &CustomCpuFree };
    const VkAllocationCallbacks* Allocator::CPU() {
        return &g_cpuAllocator;
    }
}
