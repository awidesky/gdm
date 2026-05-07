#version 330 core

in vec3 vNormalWorld;
in vec3 vPositionWorld;
in vec2 vUV;
in vec3 vColor;
in vec3 vLightDirTangent;
in vec3 vEyeDirTangent;

out vec4 FragColor;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalTex;
uniform sampler2D uSpecularTex;
uniform int uUseNormalMap;
uniform vec3 uLightPos;
uniform vec3 uViewPos;

void main() {
    const vec3 lightColor = vec3(2.0);
    const float ambientStrength = 0.18;
    const float lightPower = 2.5;

    vec3 diffuseColor = vColor;
    vec3 normalDir = normalize(vNormalWorld);
    vec3 lightDir = normalize(uLightPos - vPositionWorld);
    vec3 viewDir = normalize(uViewPos - vPositionWorld);
    vec3 specularColor = vec3(1.0);
    float shininess = 64.0;

    if (uUseNormalMap == 1) {
        diffuseColor = texture(uDiffuseTex, vUV).rgb;
        vec3 normalMap = texture(uNormalTex, vUV).rgb;
        normalDir = normalize(normalMap * 2.0 - 1.0);
        lightDir = normalize(vLightDirTangent);
        viewDir = normalize(vEyeDirTangent);
        specularColor = texture(uSpecularTex, vUV).rgb;
        shininess = 32.0;
    } else if (uUseNormalMap == -1) { // the bottom plane, only texture image is used
        diffuseColor = texture(uDiffuseTex, vUV).rgb;
    }

    float diff = max(dot(normalDir, lightDir), 0.0);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normalDir, halfDir), 0.0), shininess);

    float dist = length(uLightPos - vPositionWorld);
    float att = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist);

    vec3 ambient = ambientStrength * diffuseColor * lightColor;
    vec3 diffuse = diff * diffuseColor * lightColor * lightPower;
    vec3 specular = spec * specularColor * lightColor * lightPower;

    vec3 color = (ambient + (diffuse + specular) * att);
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
