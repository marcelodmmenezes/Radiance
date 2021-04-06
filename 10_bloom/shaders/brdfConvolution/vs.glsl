#version 450 core

layout (location = 0) in vec3 a_pos;

out vec2 v_tex;

void main()
{
	v_tex = a_pos.xy / 2.0 + 0.5;
	gl_Position = vec4(a_pos, 1.0);
}

