#version 450 core

struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

in vec3 v_nor;
in vec2 v_tex;

uniform sampler2D u_sampler;
uniform DirectionalLight u_dir_light;
uniform float u_wrap_value;

out vec4 out_color;

void main() {
	vec4 tex = texture(u_sampler, v_tex).rrra;

	vec3 normal = normalize(v_nor);
	vec3 light_dir = normalize(-u_dir_light.direction);

	float diff = max(dot(normal, light_dir), 0.0);

	diff = diff * u_wrap_value + (1 - u_wrap_value);
	diff *= diff;

	vec4 diffuse = tex * diff * vec4(u_dir_light.color, 1.0);

	out_color = diffuse;
}

