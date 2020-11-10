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
uniform float u_shininess;

out vec4 out_color;

void main() {
	vec3 tex = texture(u_sampler, v_tex).rgb;

	vec3 normal = normalize(v_nor);
	vec3 neg_light_dir = normalize(-u_dir_light.direction);

	float l_dot_n = max(dot(neg_light_dir, normal), 0.0);
	vec3 diffuse = tex * l_dot_n * u_dir_light.color;

	vec3 view_dir = normalize(u_view_pos - v_frag_pos);
	vec3 half_dir = normalize(view_dir + neg_light_dir);

	float spec = pow(max(dot(half_dir, normal), 0.0), u_shininess);
	vec3 specular = vec3(spec, spec, spec) * u_dir_light.color;

	out_color = vec4(diffuse + specular, 1.0);
}

