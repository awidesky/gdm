#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vPosition;
out vec2 vUV;
out vec3 vColor;

void main() {
    vUV = aUV;
    vColor = aColor;
    mat3 normalMat = transpose(inverse(mat3(uModel)));
    vNormal = normalize(normalMat * aPos);
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vPosition = worldPos.xyz;
    gl_Position = uProj * uView * worldPos;
}
