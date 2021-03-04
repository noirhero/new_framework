/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout (std140, binding = 0) uniform uniformBuffer {
    mat4 mvp;
} transform;

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = inColor;

    gl_Position = transform.mvp * inPos;
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
