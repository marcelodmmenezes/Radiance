#version 450 core

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
	vec4 tex = texture(u_sampler, v_tex).rrra;

	vec3 normal = normalize(v_nor);
	vec3 neg_light_dir = normalize(-u_dir_light.direction);

	vec3 view_dir = normalize(u_view_pos - v_frag_pos);
	float l_dot_n = max(dot(neg_light_dir, normal), 0.0);
	float v_dot_n = max(dot(view_dir, normal), 0.0);

	float minnaert = clamp(l_dot_n * pow(l_dot_n * v_dot_n, u_roughness), 0.0, 1.0);

	vec4 diffuse = tex * minnaert * vec4(u_dir_light.color, 1.0);

	out_color = diffuse;
}

