#version 450 core

in vec3 v_tex;

uniform samplerCube u_cube_sampler;

out vec4 out_color;

void main()
{
	out_color = texture(u_cube_sampler, v_tex);
}

