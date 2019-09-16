// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "Path.h"

namespace Path {
    using namespace std::experimental;

    std::string _assetPath;

    bool SetAssetPath(std::string&& assetPath) {
        if(false == filesystem::exists(assetPath)) {
            return false;
        }

        _assetPath = assetPath;
        return true;
    }

    std::string Apply(std::string&& path) {
        return _assetPath + path;
    }

}
