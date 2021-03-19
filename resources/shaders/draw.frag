/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform pushConstantBlock {
    int constColor;
    float mixerValue;
} colorBlock;

layout(binding = 1) uniform sampler2D tex;

void main() {
    outColor = texture(tex, outUV);
}
