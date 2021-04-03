#version 450 core

#define PI 3.1415926535

in vec2 v_tex;
in mat3 v_tbn;

in vec3 v_view_pos;
in vec3 v_light_pos;
in vec3 v_frag_pos;

uniform vec3 u_light_color;

uniform samplerCube u_irradiance_sampler;

uniform sampler2D u_color_sampler;
uniform sampler2D u_normal_sampler;
uniform sampler2D u_ao_sampler;

uniform bool u_has_metallic_map;
uniform sampler2D u_metallic_sampler;
uniform float u_metallic;

uniform bool u_has_roughness_map;
uniform sampler2D u_roughness_sampler;
uniform float u_roughness;

uniform bool u_bump_map_active;
uniform bool u_ao_map_active;

uniform vec3 u_f_0;
uniform bool u_fresnel_ative;
uniform bool u_ndf_active;
uniform bool u_masking_active;

uniform float u_gamma;
uniform float u_exposure;

out vec4 out_color;

vec3 fresnelSchlick(float h_dot_v, vec3 f_0)
{
	return f_0 + (1.0 - f_0) * pow(max(1.0 - h_dot_v, 0.0), 5.0);
}

float distributionGGX(float n_dot_h, float r)
{
	float a = r * r;
	float a_2 = a * a;

	float den = (n_dot_h * n_dot_h) * (a_2 - 1.0) + 1.0;
	den = PI * den * den;

	return a_2 / max(den, 0.0000001);
}

float geometrySchlickGGX(float n_dot_v, float r)
{
	r += 1.0;
	float k = (r * r) / 8.0;

	float den = n_dot_v * (1.0 - k) + k;

	return n_dot_v / den;
}

float geometrySmith(float n_dot_v, float n_dot_l, float r)
{
	return geometrySchlickGGX(n_dot_v, r) * geometrySchlickGGX(n_dot_l, r);
}

void main()
{
	vec3 albedo = pow(texture(u_color_sampler, v_tex).rgb, vec3(u_gamma));
	float metallic = u_metallic;
	float roughness = u_roughness;

	if (u_has_metallic_map)
	{
		metallic = texture(u_metallic_sampler, v_tex).r;
	}

	if (u_has_roughness_map)
	{
		roughness = texture(u_roughness_sampler, v_tex).r;
	}

	vec3 f_0 = mix(u_f_0, albedo, metallic);

	vec3 n;

	if (u_bump_map_active)
	{
		n = normalize(texture(u_normal_sampler, v_tex).rgb * 2.0 - 1.0);
	}
	else
	{
		n = vec3(0.0, 0.0, 1.0);
	}

	vec3 v = normalize(v_view_pos - v_frag_pos);
	vec3 l = normalize(v_light_pos);
	vec3 h = normalize(v + l);

	float n_dot_l = max(dot(n, l), 0.0);
	float n_dot_h = max(dot(n, h), 0.0);
	float n_dot_v = max(dot(n, v), 0.0);
	float h_dot_v = max(dot(h, v), 0.0);

	// DirectionalLight
	vec3 f = fresnelSchlick(h_dot_v, f_0);
	float ndf = distributionGGX(n_dot_h, roughness);
	float g = geometrySmith(n_dot_v, n_dot_l, roughness);

	vec3 brdf = vec3(1.0f);

	if (u_ndf_active)
	{
		brdf *= vec3(ndf);
	}

	if (u_masking_active)
	{
		brdf *= vec3(g);
	}

	if (u_fresnel_ative)
	{
		brdf *= f;
	}

	brdf /= (4 * n_dot_v * n_dot_l + 0.001);

	vec3 k_d = (vec3(1.0) - f) * (1.0 - metallic);

	vec3 diffuse = k_d * albedo / PI;
	vec3 specular = brdf;

	// Ambient
	float ao = 1.0;

	if (u_ao_map_active)
	{
		ao = texture(u_ao_sampler, v_tex).r;
	}

	f = fresnelSchlick(n_dot_v, f_0);
	k_d = (vec3(1.0) - f) * (1.0 - metallic);
	vec3 irradiance = texture(u_irradiance_sampler, v_tbn * n).rgb;
	vec3 ambient = k_d * irradiance * albedo * ao;

	// Result
	vec3 hdr_color = ambient + ((diffuse + specular) * u_light_color * n_dot_l);

	out_color.rgb = vec3(1.0) - exp(-hdr_color * u_exposure);
	out_color = vec4(pow(out_color.rgb, vec3(1.0 / u_gamma)), 1.0);
}

