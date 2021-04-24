#version 450 core

in vec2 v_tex;

uniform sampler2D u_sampler;

uniform vec2 u_texture_size;
uniform float u_weights[5];
uniform bool u_horizontal;

out vec4 out_color;

void main()
{
	vec2 tex_offset = 1.0 / u_texture_size;
	vec3 result = texture(u_sampler, v_tex).rgb * u_weights[0];

	if (u_horizontal)
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(u_sampler, v_tex + vec2(tex_offset.x * i, 0.0)).rgb * u_weights[i];
			result += texture(u_sampler, v_tex - vec2(tex_offset.x * i, 0.0)).rgb * u_weights[i];
		}
	}
	else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(u_sampler, v_tex + vec2(0.0, tex_offset.y * i)).rgb * u_weights[i];
			result += texture(u_sampler, v_tex - vec2(0.0, tex_offset.y * i)).rgb * u_weights[i];
		}
	}

	out_color = vec4(result, 1.0);
}

