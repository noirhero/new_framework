// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Surface {
    VkSurfaceKHR Get();

    void         Destroy();
}

#if defined(_WINDOWS)
namespace Surface::Windows {
    bool         Create();
}
#endif
