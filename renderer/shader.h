// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Shader {
    class Module {
    public:
        Module(VkShaderModule handle) : _handle(handle) {}
        ~Module();

        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        VkShaderModule Get() const noexcept { return _handle; }

    private:
        VkShaderModule _handle = VK_NULL_HANDLE;
    };
    using ModuleUPtr = std::unique_ptr<Module>;

    ModuleUPtr Create(std::string&& path);
}
