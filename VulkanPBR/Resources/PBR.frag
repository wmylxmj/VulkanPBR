#version 430

layout (location=0) in vec4 v_texcoord;
layout (location=1) in vec3 v_normal;
layout (location=2) in vec3 v_worldPosition;

layout (location=0) out vec4 outColor0;

layout(binding=3) uniform ub3 {
	mat4 cameraWorldPosition;
    mat4 reserved[1023];
};

void main() {
	vec3 n = normalize(v_normal);
	vec3 ambientColor = vec3(0.0);
    vec3 diffuseColor = n;
	vec3 specularColor = v_worldPosition;
	vec3 finalColor = ambientColor + diffuseColor + specularColor;
	outColor0 = vec4(finalColor, 1.0);
}