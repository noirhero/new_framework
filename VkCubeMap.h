// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkTexture.h"

namespace Vk
{
    struct Model;
    class Main;

    class CubeMap
    {
    public:
        bool                        Initialize(Main& main, const std::string& assetpath);
        void                        Release(VkDevice device);

        Model&                      GetSkybox() const;
        constexpr TextureCubeMap&   GetEnvironment() { return _environmentCube; }
        constexpr TextureCubeMap&   GetIrradiance() { return _irradianceCube; }
        constexpr TextureCubeMap&   GetPrefiltered() { return _prefilteredCube; }
        constexpr float             GetPrefilteredCubeMipLevels() const { return _prefilteredCubeMipLevels; }

    private:
        TextureCubeMap              _environmentCube;
        TextureCubeMap              _irradianceCube;
        TextureCubeMap              _prefilteredCube;

        float                       _prefilteredCubeMipLevels = 0.0f;
    };
}
