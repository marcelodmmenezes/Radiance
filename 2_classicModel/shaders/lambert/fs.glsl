#version 450 core

in vec3 v_nor;
in vec2 v_tex;

uniform sampler2D u_sampler;

out vec4 out_color;

void main() {
	//out_color = texture(u_sampler, v_tex);
	out_color = vec4(v_nor.z, v_nor.z, v_tex.x, 1.0);
}

