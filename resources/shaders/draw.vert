/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout(std140, binding = 0) uniform uniformBuffer {
    mat4 mvp;
} transform;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main() {
    outUV = inUV;

    gl_Position = transform.mvp * vec4(inPos, 1.0);
    //gl_Position.y = -gl_Position.y;
    //gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
