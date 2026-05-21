#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat3 uNormalMat;  // inverse(transpose(mat3(uModel)))
uniform vec3 uLightPos;

out vec2 vUV;
out vec3 vPos_world;
out vec3 vLightDir_tangent;
out vec3 vEyeDir_tangent;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
    
    // Position in world space
    vec3 pos_world = (uModel * vec4(aPos, 1.0)).xyz;
    vPos_world = pos_world;
    
    // Transform TBN to world space
    vec3 normal_world = normalize(uNormalMat * aNormal);
    vec3 tangent_world = normalize(uNormalMat * aTangent);
    vec3 bitangent_world = normalize(uNormalMat * aBitangent);
    
    // Build TBN matrix (transforms from world to tangent space)
    mat3 TBN = transpose(mat3(tangent_world, bitangent_world, normal_world));
    
    // Light direction in tangent space
    vec3 lightDir = normalize(uLightPos - pos_world);
    vLightDir_tangent = TBN * lightDir;
    
    // Eye direction in tangent space (camera is at origin in view space)
    vec3 eyeDir = normalize(-pos_world);  // Approximate: camera far away
    vEyeDir_tangent = TBN * eyeDir;
}