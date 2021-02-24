// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "util_file.h"

namespace File {
    template<typename T>
    std::vector<char> TRead(const T& path) {
        std::ifstream stream(path);
        if (false == stream.is_open())
            return {};

        const auto begin = stream.tellg();
        stream.seekg(0, std::ios::end);
        const auto end = stream.tellg();
        const auto size = end - begin;

        std::vector<char> result(size);

        stream.seekg(0, std::ios::beg);
        stream.read(result.data(), size);
        stream.close();

        return result;
    }

    std::vector<char> Read(std::string&& path) {
        return TRead(path);
    }

    std::vector<char> Read(std::wstring&& path) {
        return TRead(path);
    }
}
