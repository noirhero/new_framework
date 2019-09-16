// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

class Timer {
public:
    float                                   Update();
    void                                    Reset() { _accum = 0.0f; }

    float                                   Delta() const { return _delta; }
    float                                   Accum() const { return _accum; }
    uint32_t                                FPS() const { _fps; }

private:
    std::chrono::system_clock::time_point   _prev = std::chrono::system_clock::now();

    float                                   _delta = 0.0f;
    float                                   _accum = 0.0f;
    float                                   _second = 0.0f;
    uint32_t                                _frame = 0;
    uint32_t                                _fps = 0;
};
