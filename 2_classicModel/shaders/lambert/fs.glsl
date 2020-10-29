#version 450 core

struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

in vec3 v_nor;
in vec2 v_tex;

uniform sampler2D u_sampler;
uniform DirectionalLight u_dir_light;

out vec4 out_color;

void main() {
	vec3 tex = texture(u_sampler, v_tex).rrr;

	vec3 normal = normalize(v_nor);
	vec3 neg_light_dir = normalize(-u_dir_light.direction);

	float l_dot_n = max(dot(neg_light_dir, normal), 0.0);
	vec3 diffuse = tex * l_dot_n * u_dir_light.color;

	out_color = vec4(diffuse, 1.0);
}

