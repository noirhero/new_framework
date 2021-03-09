/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout(std140, binding = 0) uniform uniformBuffer {
    mat4 mvp;
} transform;

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;

void main() {
    outColor = inColor;
    outUV = inUV;

    gl_Position = transform.mvp * inPos;
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
