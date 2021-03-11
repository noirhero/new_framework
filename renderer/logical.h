// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Logical::Device {
    VkDevice Get();

    bool     Create();
    void     Destroy();
}

namespace Logical::SwapChain {
    bool     Create();
    void     Destroy();
}

namespace Logical {
    class CommandPool {
    public:
        CommandPool(VkCommandPool cmdPool) : _handle(cmdPool) {}
        ~CommandPool();

    private:
        VkCommandPool _handle = VK_NULL_HANDLE;
    };
    using CommandPoolUPtr = std::unique_ptr<CommandPool>;

    CommandPoolUPtr AllocateGPUCommandPool();
}
