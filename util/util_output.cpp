// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "util_output.h"

namespace Output {
    void Print(std::string&& msg) {
#if defined(WIN32)
        OutputDebugStringA(msg.c_str());
#endif
    }
}
