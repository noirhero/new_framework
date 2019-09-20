// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

class Timer {
public:
    float                                   Update();
    void                                    Reset() { _accumulation = 0.0f; }

    float                                   Delta() const { return _delta; }
    float                                   Accumulation() const { return _accumulation; }
    uint32_t                                Fps() const { return _fps; }

private:
    std::chrono::system_clock::time_point   _prev = std::chrono::system_clock::now();

    float                                   _delta = 0.0f;
    float                                   _accumulation = 0.0f;
    float                                   _second = 0.0f;
    uint32_t                                _frame = 0;
    uint32_t                                _fps = 0;
};
