// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "Timer.h"

float Timer::Update() {
    const auto now = std::chrono::system_clock::now();
    _delta = std::chrono::duration<float, std::milli>(now - _prev).count();
    _prev = now;

    _accumulation += _delta;
    _second += _delta;
    if(1.0f <= _second) {
        _second = 0.0f;
        _fps = _frame + 1;
        _frame = 0;
    }
    else {
        ++_frame;
    }

    return _delta;
}
