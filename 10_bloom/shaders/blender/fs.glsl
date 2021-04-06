#version 450 core

in vec2 v_tex;

uniform sampler2D u_scene_sampler;
uniform sampler2D u_blur_sampler;

uniform float u_gamma;
uniform float u_exposure;

out vec4 out_color;

void main()
{
	vec3 scene = texture(u_scene_sampler, v_tex).rgb;
	vec3 blur = texture(u_blur_sampler, v_tex).rgb;

	scene += blur;

	out_color.rgb = vec3(1.0) - exp(-scene * u_exposure);
	out_color = vec4(pow(out_color.rgb, vec3(1.0 / u_gamma)), 1.0);
}

