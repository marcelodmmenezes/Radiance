#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_nor;
layout (location = 2) in vec2 a_tex;
layout (location = 3) in vec3 a_tan;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat3 u_nor_transform;

uniform vec3 u_light_dir;
uniform vec3 u_view_pos;

out vec2 v_tex;
out mat3 v_tbn;
out vec3 v_l;
out vec3 v_v;
out vec3 v_h;
out vec3 v_color;

void main()
{
	vec3 n = normalize(u_nor_transform * a_nor);
	vec3 t = normalize(u_nor_transform * a_tan);
	t = normalize(t - dot(t, n) * n);
	vec3 b = normalize(cross(n, t));

	vec4 world_pos = u_model_matrix * vec4(a_pos, 1.0);
	gl_Position = u_projection_matrix * u_view_matrix * world_pos;

	v_tex = a_tex;

	v_tbn = mat3(t, b, n);
	mat3 tbn_inv = transpose(v_tbn);

	v_v = normalize(tbn_inv * (u_view_pos - vec3(world_pos)));
	v_l = normalize(tbn_inv * -u_light_dir);
	v_h = normalize(v_v + v_l);
}

