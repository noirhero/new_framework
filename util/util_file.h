// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace File {
    std::vector<char> Read(std::string&& path, std::ios::openmode mode);
    std::vector<char> Read(std::wstring&& path, std::ios::openmode mode);
}
