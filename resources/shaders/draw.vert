/* Copyright 2018-2021 TAP, Inc. All Rights Reserved. */

#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    outColor = inColor;
    gl_Position = inPos;

   // GL->VK conventions
   gl_Position.y = -gl_Position.y;
   gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
