// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Path {
    bool            SetAssetPath(std::string&& assetPath);

    std::string     Apply(std::string&& path);
}
