#version 450 core

#define PI 3.1415926535

in mat3 v_tbn;
in mat3 v_tbn_inv;
in vec2 v_tex;
in vec3 v_world_pos;

uniform vec3 u_view_pos;

uniform samplerCube u_irradiance_sampler;
uniform samplerCube u_specular_sampler;
uniform sampler2D u_brdf_lut_sampler;

uniform int u_parallax_algorithm;
uniform float u_depth_scale;
uniform int u_n_parallax_layers;

uniform bool u_has_normal_map;
uniform bool u_has_depth_map;
uniform bool u_has_ao_map;
uniform bool u_has_metallic_map;
uniform bool u_has_roughness_map;

uniform sampler2D u_albedo_sampler;
uniform sampler2D u_normal_sampler;
uniform sampler2D u_depth_sampler;
uniform sampler2D u_ao_sampler;
uniform sampler2D u_metallic_sampler;
uniform sampler2D u_roughness_sampler;

uniform float u_metallic;
uniform float u_roughness;

uniform float u_gamma;
uniform float u_exposure;

out vec4 out_color;

vec3 fresnelSchlick(float h_dot_v, vec3 f_0, float roughness)
{
	return f_0 + (max(vec3(1.0 - roughness), f_0) - f_0) * pow(max(1.0 - h_dot_v, 0.0), 5.0);
}

vec2 defaultParallax(vec3 v, vec2 t)
{
	float depth = 1.0 - texture(u_depth_sampler, t).r;
	return t - (v.xy / v.z) * u_depth_scale * depth;
}

vec2 parallaxOffset(vec3 v, vec2 t)
{
	float depth = 1.0 - texture(u_depth_sampler, t).r;
	return t - v.xy * u_depth_scale * depth;
}

vec2 steepParallax(vec3 v, vec2 t)
{
	float layer_depth = 1.0 / u_n_parallax_layers;
	float current_layer_depth = 0.0;

	vec2 delta = (u_depth_scale * -v.xy) / u_n_parallax_layers;

	float current_sample = 1.0 - texture(u_depth_sampler, t).r;

	while (current_layer_depth < current_sample)
	{
		t += delta;
		current_sample = 1.0 - texture(u_depth_sampler, t).r;
		current_layer_depth += layer_depth;
	}

	return t;
}

vec2 parallaxOcclusion(vec3 v, vec2 t)
{
	float layer_depth = 1.0 / u_n_parallax_layers;
	float current_layer_depth = 0.0;

	vec2 delta = (u_depth_scale * -v.xy) / u_n_parallax_layers;

	float current_sample = 1.0 - texture(u_depth_sampler, t).r;

	while (current_layer_depth < current_sample)
	{
		t += delta;
		current_sample = 1.0 - texture(u_depth_sampler, t).r;
		current_layer_depth += layer_depth;
	}

	float depth_after = current_sample - current_layer_depth;

	vec2 t_0 = t - delta;
	current_sample = 1.0 - texture(u_depth_sampler, t_0).r;
	float depth_before = current_sample - current_layer_depth + layer_depth;

	float w = depth_after / (depth_after - depth_before);

	return mix(t, t_0, w);
}

vec2 parallax(vec3 v, vec2 t)
{
	switch (u_parallax_algorithm)
	{
	case 0:
		return defaultParallax(v, t);

	case 1:
		return parallaxOffset(v, t);

	case 2:
		return steepParallax(v, t);

	case 3:
		return parallaxOcclusion(v, t);
	}
}

void main()
{
	vec2 tex = v_tex;
	vec3 v = normalize(u_view_pos - v_world_pos);

	if (u_has_depth_map)
	{
		tex = parallax(v_tbn_inv * v, tex); 

		if (tex.x < 0.0 || tex.x > 1.0 ||
			tex.y < 0.0 || tex.y > 1.0)
		{
			discard;
		}
	}

	vec3 albedo = pow(texture(u_albedo_sampler, tex).rgb, vec3(u_gamma));
	float ao = 1.0;
	float metallic = u_metallic;
	float roughness = u_roughness;

	if (u_has_ao_map)
	{
		ao = texture(u_ao_sampler, tex).r;
	}

	if (u_has_metallic_map)
	{
		metallic = texture(u_metallic_sampler, tex).r;
	}

	if (u_has_roughness_map)
	{
		roughness = texture(u_roughness_sampler, tex).r;
	}

	vec3 n;

	if (u_has_normal_map)
	{
		n = normalize(texture(u_normal_sampler, tex).rgb * 2.0 - 1.0);
	}
	else
	{
		n = vec3(0.0, 0.0, 1.0);
	}

	n = v_tbn * n;
	float n_dot_v = max(dot(n, v), 0.0);

	vec3 f_0 = mix(vec3(0.04), albedo, metallic);
	vec3 f = fresnelSchlick(n_dot_v, f_0, roughness);
	vec3 k_d = (vec3(1.0) - f) * (1.0 - metallic);

	vec3 irradiance = texture(u_irradiance_sampler, n).rgb;
	vec3 env_diffuse = irradiance * albedo;

	vec3 r = reflect(-v, n);
	vec3 prefiltered_color = textureLod(u_specular_sampler, r, roughness * 4.0).rgb;

	f = fresnelSchlick(n_dot_v, f_0, roughness);

	vec2 env_brdf = texture(u_brdf_lut_sampler, vec2(n_dot_v, roughness)).rg;
	vec3 env_specular = prefiltered_color * (f * env_brdf.x + env_brdf.y);

	vec3 radiance = (k_d * env_diffuse + env_specular) * ao;

	out_color.rgb = vec3(1.0) - exp(-radiance * u_exposure);
	out_color = vec4(pow(out_color.rgb, vec3(1.0 / u_gamma)), 1.0);
}

