#version 450 core

layout (location = 0) in vec3 a_pos;

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

out vec3 v_tex;

void main()
{
	gl_Position = (u_projection_matrix * u_view_matrix * vec4(a_pos, 1.0)).xyww;

	v_tex = a_pos;
}

