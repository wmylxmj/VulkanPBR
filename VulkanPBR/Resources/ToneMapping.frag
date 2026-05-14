#version 430

layout (location=0) in vec4 v_texcoord;
layout (binding=4) uniform sampler2D u_hdrTexture;

layout (location=0) out vec4 outColor0;

void main() {
	vec3 hdrColor = texture(u_hdrTexture, v_texcoord.xy).rgb;
	vec3 mappedColor = hdrColor / (hdrColor + vec3(1.0));
	outColor0 = vec4(pow(mappedColor, vec3(1.0/2.2)), 1.0);
}