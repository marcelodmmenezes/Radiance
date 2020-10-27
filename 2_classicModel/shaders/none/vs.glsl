#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex;

uniform mat4 u_transform;

out vec2 v_tex;

void main() {
	gl_Position = u_transform * vec4(a_pos, 1.0);
	v_tex = a_tex;
}

