#version 450 core

in vec3 v_local_position;

uniform sampler2D u_env_map_sampler;

out vec4 out_color;

void main()
{
	vec3 pos = normalize(v_local_position);

	vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
	uv *= vec2(0.1591, 0.3183);
	uv += 0.5;

	out_color = vec4(texture(u_env_map_sampler, uv).rgb, 1.0);
}

