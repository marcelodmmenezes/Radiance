#version 450 core

in vec3 v_tex;

uniform samplerCube u_cube_sampler;

uniform float u_gamma;
uniform float u_exposure;

out vec4 out_color;

void main()
{
	vec3 tex = pow(texture(u_cube_sampler, v_tex).rgb, vec3(u_gamma));

	tex = vec3(1.0) - exp(-tex * u_exposure);

	out_color = vec4(pow(tex, vec3(1.0 / u_gamma)), 1.0);
}

