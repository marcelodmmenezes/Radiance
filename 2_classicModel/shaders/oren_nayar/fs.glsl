#version 450 core

#define PI 3.1415926535

struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

in vec3 v_nor;
in vec2 v_tex;
in vec3 v_frag_pos;

uniform sampler2D u_sampler;
uniform DirectionalLight u_dir_light;
uniform vec3 u_view_pos;
uniform float u_roughness;

out vec4 out_color;

void main() {
	vec3 tex = texture(u_sampler, v_tex).rgb;

	vec3 normal = normalize(v_nor);
	vec3 neg_light_dir = normalize(-u_dir_light.direction);
	vec3 view_dir = normalize(u_view_pos - v_frag_pos);

	float l_dot_n = dot(neg_light_dir, normal);
	float v_dot_n = dot(view_dir, normal);

	float angle_l_dot_n = acos(l_dot_n);
	float angle_v_dot_n = acos(v_dot_n);

	float angle_difference = max(0.0,
		dot(normalize(view_dir - normal * v_dot_n),
			normalize(neg_light_dir - normal * l_dot_n)));

	float alpha = max(angle_v_dot_n, angle_l_dot_n);
	float beta = min(angle_v_dot_n, angle_l_dot_n);

	float roughness_squared = u_roughness * u_roughness;

	float a = 1.0 - (0.5 * roughness_squared) / (roughness_squared + 0.33);
	float b = (0.45 * roughness_squared) / (roughness_squared + 0.09);

	vec3 diffuse = tex * (a + b * angle_difference *
		sin(alpha) * tan(beta)) * max(l_dot_n, 0.0) *
		u_dir_light.color;

	out_color = vec4(diffuse, 1.0);
}

