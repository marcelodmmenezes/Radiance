#version 450 core

#define PI 3.1415926535

in vec2 v_tex;

out vec2 out_color;

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

float geometrySchlickGGX(float n_dot_v, float r)
{
	float k = (r * r) / 2.0;
	float den = n_dot_v * (1.0 - k) + k;

	return n_dot_v / den;
}

float geometrySmith(float n_dot_v, float n_dot_l, float r)
{
	return geometrySchlickGGX(n_dot_v, r) * geometrySchlickGGX(n_dot_l, r);
}

void main()
{
	float n_dot_v = v_tex.x;
	float roughness = v_tex.y;

	vec3 v;
	v.x = sqrt(1.0 - n_dot_v * n_dot_v);
	v.y = 0.0;
	v.z = n_dot_v;

	float a = 0.0;
	float b = 0.0;

	vec3 n = vec3(0.0, 0.0, 1.0);

	const uint sample_count = 1024u;

	for (uint i = 0u; i < sample_count; ++i)
	{
		vec2 xi = hammersley(i, sample_count);
		vec3 h = importanceSampleGGX(xi, n, roughness);
		vec3 l = normalize(2.0 * dot(v, h) * h - v);

		float n_dot_l = max(l.z, 0.0);
		float n_dot_h = max(h.z, 0.0);
		float h_dot_v = max(dot(h, v), 0.0);

		if (n_dot_l > 0.0)
		{
			float g = geometrySmith(n_dot_v, n_dot_l, roughness);
			float g_vis = (g * h_dot_v) / (n_dot_h * n_dot_v);
			float fc = pow(1.0 - h_dot_v, 5.0);

			a += (1.0 - fc) * g_vis;
			b += fc * g_vis;
		}
	}

	a /= float(sample_count);
	b /= float(sample_count);

	out_color = vec2(a, b);
}

