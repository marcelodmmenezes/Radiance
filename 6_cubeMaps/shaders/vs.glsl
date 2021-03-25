#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_nor;
layout (location = 2) in vec2 a_tex;
layout (location = 3) in vec3 a_tan;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat3 u_nor_transform;

uniform float u_uv_multiplier;

out mat3 v_tbn;
out vec2 v_tex;
out vec3 v_frag_pos;

void main()
{
	vec3 n = normalize(u_nor_transform * a_nor);
	vec3 t = normalize(u_nor_transform * a_tan);
	vec3 b = normalize(cross(n, t));

	v_tbn = transpose(mat3(t, b, n));

	vec4 world_position = u_model_matrix * vec4(a_pos, 1.0);
	gl_Position = u_projection_matrix * u_view_matrix * world_position;

	v_frag_pos = vec3(world_position);

	v_tex = u_uv_multiplier * a_tex;
}

