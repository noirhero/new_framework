// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "shader.h"

#include "allocator_cpu.h"
#include "logical.h"

namespace Shader {
    using namespace Renderer;

    Module::~Module() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroyShaderModule(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    ModuleUPtr Create(std::string&& path) {
        const auto readFile = File::Read(std::move(path), std::ios::binary);
        if (readFile.empty()) {
            return nullptr;
        }

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = static_cast<decltype(info.codeSize)>(readFile.size());
        info.pCode = reinterpret_cast<decltype(info.pCode)>(readFile.data());

        VkShaderModule module = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateShaderModule(Logical::Device::Get(), &info, Allocator::CPU(), &module)) {
            return nullptr;
        }

        return std::make_unique<Module>(module);
    }
}
