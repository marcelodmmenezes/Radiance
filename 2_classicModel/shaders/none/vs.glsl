#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_nor;
layout (location = 2) in vec2 a_tex;

uniform mat4 u_transform;
uniform mat3 u_nor_transform;

out vec3 v_nor;
out vec2 v_tex;

void main() {
	gl_Position = u_transform * vec4(a_pos, 1.0);
	v_nor = u_nor_transform * a_nor;
	v_tex = a_tex;
}

