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
	vec4 tex = texture(u_sampler, v_tex).rrra;

	vec3 normal = normalize(v_nor);
	vec3 neg_light_dir = normalize(-u_dir_light.direction);

	float l_dot_n = max(dot(neg_light_dir, normal), 0.0);
	vec4 diffuse = tex * l_dot_n * vec4(u_dir_light.color, 1.0);

	out_color = diffuse;
}

