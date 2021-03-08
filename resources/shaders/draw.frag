/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform pushConstantBlock {
    int constColor;
    float mixerValue;
} colorBlock;

vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);

void main() {
    if(colorBlock.constColor == 1)
       outColor = red;
    else if(colorBlock.constColor == 2)
       outColor = green;
    else if(colorBlock.constColor == 3)
       outColor = blue;
    else
       outColor = colorBlock.mixerValue * color;

    outColor = color;
}
