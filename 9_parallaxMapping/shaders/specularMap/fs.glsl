#version 450 core

#define PI 3.1415926535

in vec3 v_local_position;

uniform samplerCube u_env_map_sampler;
uniform float u_roughness;

out vec4 out_color;

float radicalInverseVDC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n)
{
	return vec2(float(i) / float(n), radicalInverseVDC(i));
}

vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * xi.x;
	float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

	vec3 h;
	h.x = cos(phi) * sin_theta;
	h.y = sin(phi) * sin_theta;
	h.z = cos_theta;

	vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tan = normalize(cross(up, n));
	vec3 bin = cross(n, tan);

	vec3 sample_vec = tan * h.x + bin * h.y + n * h.z;

	return normalize(sample_vec);
}

void main()
{
	vec3 n = normalize(v_local_position);
	vec3 r = n;
	vec3 v = r;

	const uint sample_count = 1024u;
	float total_weight = 0.0;
	vec3 prefiltered_color = vec3(0.0);

	for (uint i = 0u; i < sample_count; ++i)
	{
		vec2 xi = hammersley(i, sample_count);
		vec3 h = importanceSampleGGX(xi, n, u_roughness);
		vec3 l = normalize(2.0 * dot(v, h) * h - v);

		float n_dot_l = max(dot(n, l), 0.0);

		if (n_dot_l > 0.0)
		{
			prefiltered_color += texture(u_env_map_sampler, l).rgb * n_dot_l;
			total_weight += n_dot_l;
		}
	}

	prefiltered_color /= total_weight;

	out_color = vec4(prefiltered_color, 1.0);
}

