#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aUV;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec3 vNormalWorld;
out vec3 vPositionWorld;
out vec2 vUV;
out vec3 vColor;
out vec3 vLightDirTangent;
out vec3 vEyeDirTangent;

void main() {
    vUV = aUV;
    vColor = aColor;
    mat3 normalMat = transpose(inverse(mat3(uModel)));
    vec3 normalWorld = normalize(normalMat * aNormal);
    vec3 tangentWorld = normalize(normalMat * aTangent);
    vec3 bitangentWorld = normalize(normalMat * aBitangent);

    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vPositionWorld = worldPos.xyz;
    vNormalWorld = normalWorld;

    mat3 TBN = transpose(mat3(tangentWorld, bitangentWorld, normalWorld));
    vec3 lightDirWorld = normalize(uLightPos - vPositionWorld);
    vec3 eyeDirWorld = normalize(uViewPos - vPositionWorld);
    vLightDirTangent = TBN * lightDirWorld;
    vEyeDirTangent = TBN * eyeDirWorld;
    gl_Position = uProj * uView * worldPos;
}
