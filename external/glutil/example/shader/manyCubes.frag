#version 330 core

in vec3 vNormal;
in vec3 vPosition;
in vec2 vUV;
in vec3 vColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uHasTexture;
uniform vec3 uViewPos;
uniform float uTime;
uniform float uLightDensity;

const int NUM_LIGHTS = 8;

vec3 computeLight(vec3 pos, vec3 normal) {
    vec3 result = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; ++i) {
        float fi = float(i);
        vec3 lightPos = vec3(5.0 * sin(uTime * (0.2 + 0.05 * fi) + fi),
                             3.0 * cos(uTime * (0.12 + 0.03 * fi) - fi),
                             5.0 * sin(uTime * (0.15 + 0.04 * fi) + fi * 0.7));

        vec3 L = normalize(lightPos - pos);
        float diff = max(dot(normal, L), 0.0);

        vec3 V = normalize(uViewPos - pos);
        vec3 H = normalize(V + L);
        float spec = pow(max(dot(normal, H), 0.0), 32.0);

        float dist = length(lightPos - pos);
        float att = 1.0 / (1.0 + 0.2 * dist + 0.05 * dist * dist);

        vec3 lightCol = vec3(0.25 + 0.35 * sin(fi + uTime),
                             0.20 + 0.35 * cos(fi * 0.7 + uTime * 0.6),
                             0.18 + 0.30 * sin(fi * 1.3 - uTime * 0.4));
        result += (diff * vec3(0.28) + spec * vec3(0.08)) * lightCol * att;
    }

    float heavy = 0.0;
    for (int k = 1; k <= 16; ++k) {
        heavy += abs(sin(dot(pos, vec3(float(k) * 0.13, float(k) * 0.17, float(k) * 0.19)) + uTime * 2.0));
        heavy += abs(cos(dot(normal, vec3(float(k) * 0.11, float(k) * 0.07, float(k) * 0.09)) * 1.3 - uTime * 0.7));
    }
    heavy = heavy / 32.0;

    return result + vec3(heavy * 0.12) * uLightDensity;
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 base = (uHasTexture == 1) ? texture(uTexture, vUV).rgb : vColor;
    base = mix(base, vec3(dot(base, vec3(0.3333))), 0.08);
    base = base * 0.95 + vec3(0.02, 0.03, 0.04);

    vec3 light = computeLight(vPosition, n);
    vec3 color = clamp(base * 0.85 + light * uLightDensity, 0.0, 1.0);
    color = pow(color, vec3(1.18));
    FragColor = vec4(color, 1.0);
}
