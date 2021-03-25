#version 450 core

struct DirectionalLight
{
	vec3 direction;
	vec3 color;
};

in mat3 v_tbn;
in vec2 v_tex;
in vec3 v_frag_pos;

uniform sampler2D u_color_sampler;
uniform sampler2D u_normal_sampler;

uniform DirectionalLight u_dir_light;
uniform vec3 u_view_pos;
uniform float u_shininess;
uniform bool u_bump_map_active;

out vec4 out_color;

void main()
{
	vec3 tex = texture(u_color_sampler, v_tex).rgb;
	vec3 normal;

	if (u_bump_map_active)
	{
		normal = normalize(texture(u_normal_sampler, v_tex).rgb * 2.0 - 1.0);
	}
	else
	{
		normal = vec3(0.0, 0.0, 1.0);
	}

	vec3 tangent_frag_pos = v_tbn * v_frag_pos;
	vec3 tangent_view_dir = normalize(v_tbn * u_view_pos - tangent_frag_pos);
	vec3 tangent_light_dir = normalize(v_tbn * (-u_dir_light.direction));

	float l_dot_n = max(dot(tangent_light_dir, normal), 0.0);
	vec3 diffuse = tex * l_dot_n * u_dir_light.color;

	vec3 half_dir = normalize(tangent_view_dir + tangent_light_dir);

	float spec = pow(max(dot(half_dir, normal), 0.0), u_shininess);
	vec3 specular = vec3(spec, spec, spec) * u_dir_light.color;

	out_color = vec4(diffuse + specular, 1.0);
}

