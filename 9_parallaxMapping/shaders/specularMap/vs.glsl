#version 450 core

layout (location = 0) in vec3 a_pos;

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

out vec3 v_local_position;

void main()
{
	v_local_position = a_pos;
	gl_Position = u_projection_matrix * u_view_matrix * vec4(v_local_position, 1.0);
}

