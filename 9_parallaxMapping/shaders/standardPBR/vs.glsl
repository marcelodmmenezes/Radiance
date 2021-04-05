#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_nor;
layout (location = 2) in vec2 a_tex;
layout (location = 3) in vec3 a_tan;

uniform mat4 u_model_matrix;
uniform mat4 u_pv_matrix;
uniform mat3 u_nor_transform;

out mat3 v_tbn;
out mat3 v_tbn_inv;
out vec2 v_tex;
out vec3 v_world_pos;

void main()
{
	vec3 n = normalize(u_nor_transform * a_nor);
	vec3 t = normalize(u_nor_transform * a_tan);
	t = normalize(t - dot(t, n) * n);
	vec3 b = normalize(cross(n, t));

	v_tbn = mat3(t, b, n);
	v_tbn_inv = transpose(v_tbn);
	v_tex = a_tex;

	vec4 world_pos = u_model_matrix * vec4(a_pos, 1.0);
	v_world_pos = vec3(world_pos);

	gl_Position = u_pv_matrix * world_pos;
}

