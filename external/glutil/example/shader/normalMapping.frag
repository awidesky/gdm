#version 330 core

in vec2 vUV;
in vec3 vPos_world;
in vec3 vLightDir_tangent;
in vec3 vEyeDir_tangent;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalTex;
uniform sampler2D uSpecularTex;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform float uLightPower;

out vec4 FragColor;

void main() {
    // Sample textures
    vec3 diffuseColor = texture(uDiffuseTex, vUV).rgb;
    vec3 normalMap = texture(uNormalTex, vUV).rgb;
    vec3 specularColor = texture(uSpecularTex, vUV).rgb;
    
    // Decode normal from map (convert from [0,1] to [-1,1])
    vec3 normal_tangent = normalize(normalMap * 2.0 - 1.0);
    
    // Normalize interpolated directions
    vec3 lightDir = normalize(vLightDir_tangent);
    vec3 eyeDir = normalize(vEyeDir_tangent);
    
    // Diffuse component (Lambertian)
    float diffuse = max(dot(normal_tangent, lightDir), 0.0);
    
    // Specular component (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + eyeDir);
    float specular = pow(max(dot(normal_tangent, halfDir), 0.0), 32.0);
    
    // Distance attenuation (approximate)
    float dist = length(uLightPos - vPos_world);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.02 * dist * dist);
    
    // Combine lighting with light color and power
    vec3 ambient = 0.1 * diffuseColor * uLightColor;
    vec3 diffuseLight = diffuseColor * diffuse * attenuation * uLightPower * uLightColor;
    vec3 specularLight = specularColor * specular * attenuation * uLightPower * uLightColor;
    
    FragColor = vec4(ambient + diffuseLight + specularLight, 1.0);
}