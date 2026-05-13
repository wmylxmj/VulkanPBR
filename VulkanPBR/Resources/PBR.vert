#version 430

layout(location=0) in vec4 position;
layout(location=2) in vec4 texcoord;
layout(location=1) in vec4 normal;

layout(binding=0) uniform ub0 {
	mat4 modelMat;
    mat4 viewMat;
    mat4 projMat;
    mat4 itModelMat;
    mat4 reserved[1020];
};

layout (location=0) out vec4 v_texcoord;
layout (location=1) out vec3 v_normal;
layout (location=2) out vec3 v_worldPosition;

void main() {
    v_normal = normalize(itModelMat * normal).xyz;
    v_texcoord = texcoord;
    v_worldPosition = (modelMat * position).xyz;
    gl_Position = projMat * viewMat * vec4(v_worldPosition.xyz, 1.0);
}
