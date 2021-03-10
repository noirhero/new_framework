// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "util_path.h"

namespace Path {
    std::string  g_currentPathAnsi;
    std::wstring g_currentPath;

    std::string  g_resourcePathAnsi;
    std::wstring g_resourcePath;

    std::filesystem::path FindResourcePath(std::filesystem::path&& inPath) {
        auto checkPath = inPath;
        checkPath += "\\resources"s;
        if (std::filesystem::exists(checkPath))
            return checkPath;

        if (false == inPath.has_parent_path())
            return {};

        return FindResourcePath(inPath.parent_path());
    }

    void Initialize() {
        const auto currentPath = std::filesystem::current_path();
        g_currentPathAnsi = currentPath.generic_string() + "/";
        g_currentPath = currentPath.generic_wstring() + L"/";

        const auto resourcePath = FindResourcePath(std::filesystem::current_path());
        g_resourcePathAnsi = resourcePath.generic_string() + "/";
        g_resourcePath = resourcePath.generic_wstring() + L"/";
    }

    const char* GetCurrentPathAnsi() {
        return g_currentPathAnsi.c_str();
    }
    const wchar_t* GetCurrentPath() {
        return g_currentPath.c_str();
    }

    const char* GetResourcePathAnsi() {
        return g_resourcePathAnsi.c_str();
    }
    const wchar_t* GetResourcePath() {
        return g_resourcePath.c_str();
    }
}
