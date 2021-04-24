#version 450 core

in vec3 v_tex;

uniform samplerCube u_cube_sampler;
uniform float u_mipmap_level;

uniform float u_gamma;
uniform float u_exposure;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_bright;

void main()
{
	vec3 tex = pow(textureLod(u_cube_sampler, v_tex, u_mipmap_level).rgb, vec3(u_gamma));

	// Tonemapping render target 0
	out_color = vec4(vec3(1.0) - exp(-tex * u_exposure), 1.0);

	// Bloom
	float brightness = dot(out_color.rgb, vec3(0.2126, 0.7152, 0.0722));

	if (brightness > 1.0)
	{
		out_bright = vec4(tex, 1.0);
	}
	else
	{
		out_bright = vec4(0.0, 0.0, 0.0, 1.0);
	}
}

