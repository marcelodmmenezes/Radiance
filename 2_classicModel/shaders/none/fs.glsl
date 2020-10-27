#version 450 core

in vec2 v_tex;

uniform sampler2D u_sampler;

out vec4 out_color;

void main() {
	out_color = texture(u_sampler, v_tex).rrra;
}

