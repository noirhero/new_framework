// Copyright 2018-2020 TAP, Inc. All Rights Reserved.

#pragma once

#ifdef _DEBUG
#define TEST(expr) \
    do { \
        if(!(expr)) { \
            assert(0 && #expr); \
        } \
    } while(0)
#else
#define TEST(expr) \
    do { \
        if(!(expr)) { \
            throw std::runtime_error("TEST FAILED: " #expr); \
        } \
    } while(0)
#endif

namespace Renderer {
    
}
