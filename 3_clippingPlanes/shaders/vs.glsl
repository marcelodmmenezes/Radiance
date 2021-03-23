#version 450 core

#define MAX_CLIPPING_PLANES 4

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_nor;
layout (location = 2) in vec2 a_tex;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat3 u_nor_transform;

uniform bool u_plane_active[MAX_CLIPPING_PLANES];
uniform vec4 u_plane_equations[MAX_CLIPPING_PLANES];

out vec3 v_nor;
out vec2 v_tex;
out vec3 v_frag_pos;
out float gl_ClipDistance[MAX_CLIPPING_PLANES];

void main()
{
	vec4 world_position = u_model_matrix * vec4(a_pos, 1.0);
	gl_Position = u_projection_matrix * u_view_matrix * world_position;

	v_frag_pos = vec3(world_position);

	v_nor = u_nor_transform * a_nor;
	v_tex = a_tex;

	for (int i = 0; i < MAX_CLIPPING_PLANES; ++i)
	{
		if (u_plane_active[i])
		{
			gl_ClipDistance[i] = dot(world_position, u_plane_equations[i]);
		}
	}
}

