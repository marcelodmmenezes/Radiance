#version 450 core

struct DirectionalLight
{
	vec3 direction;
	vec3 color;
};

in mat3 v_tbn;
in mat3 v_tbn_inv;
in vec2 v_tex;
in vec3 v_frag_pos;

uniform sampler2D u_color_sampler;
uniform sampler2D u_normal_sampler;
uniform samplerCube u_cube_sampler;

uniform DirectionalLight u_dir_light;
uniform vec3 u_view_pos;
uniform float u_shininess;
uniform bool u_bump_map_active;

uniform float u_diffuse;
uniform float u_reflection;
uniform float u_refraction;

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
	vec3 diffuse = tex * l_dot_n * u_dir_light.color * u_diffuse;

	vec3 half_dir = normalize(tangent_view_dir + tangent_light_dir);

	float spec = pow(max(dot(half_dir, normal), 0.0), u_shininess);
	vec3 specular = vec3(spec, spec, spec) * u_dir_light.color;

	out_color = vec4(diffuse + specular, 1.0);

	// Reflection
	vec3 tangent_reflection = reflect(-tangent_view_dir, normal);
	vec3 world_reflection = v_tbn_inv * tangent_reflection;

	out_color += vec4(texture(u_cube_sampler, world_reflection).rgb, 1.0) * u_reflection;

	// Refraction
	float ratio = 1.0 / 1.52;
	vec3 tangent_refraction = refract(-tangent_view_dir, normal, ratio);
	vec3 world_refraction = v_tbn_inv * tangent_refraction;

	out_color += vec4(texture(u_cube_sampler, world_refraction).rgb, 1.0) * u_refraction;
}

