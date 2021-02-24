// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace File {
    std::vector<char> Read(std::string&& path);
    std::vector<char> Read(std::wstring&& path);
}
