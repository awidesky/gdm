#version 330 core

/*
    Fragment shader for encoding validation
    --------------------------------------------
    This shader is intentionally longer than necessary
    to help evaluate file loading and encoding conversion.

    UTF-8 인코딩 성능 테스트
    비 ASCII 문자 포함:
    가나다라마바사
    あいうえお
    你好世界
*/

in vec2 vUV;
out vec4 FragColor;

float gridLine(vec2 uv, float scale)
{
    vec2 g = fract(uv * scale);
    float lineX = step(g.x, 0.02) + step(0.98, g.x);
    float lineY = step(g.y, 0.02) + step(0.98, g.y);
    return clamp(lineX + lineY, 0.0, 1.0);
}

float circle(vec2 uv, vec2 center, float radius)
{
    float dist = distance(uv, center);
    return smoothstep(radius, radius - 0.01, dist);
}

vec3 gradientColor(vec2 uv)
{
    vec3 c1 = vec3(uv.x, uv.y, 0.5);
    vec3 c2 = vec3(0.2, 0.4, 0.8);

    return mix(c1, c2, 0.3);
}

vec3 applyGrid(vec3 color, vec2 uv)
{
    float g1 = gridLine(uv, 10.0);
    float g2 = gridLine(uv, 20.0);

    color += vec3(g1 * 0.15);
    color += vec3(g2 * 0.08);

    return color;
}

vec3 applyCircle(vec3 color, vec2 uv)
{
    float c1 = circle(uv, vec2(0.5, 0.5), 0.25);
    float c2 = circle(uv, vec2(0.3, 0.7), 0.12);
    float c3 = circle(uv, vec2(0.7, 0.3), 0.12);

    color = mix(color, vec3(1.0, 0.8, 0.2), c1);
    color = mix(color, vec3(0.9, 0.2, 0.3), c2);
    color = mix(color, vec3(0.2, 1.0, 0.4), c3);

    return color;
}

vec3 vignette(vec3 color, vec2 uv)
{
    vec2 centered = uv - 0.5;
    float d = dot(centered, centered);

    float factor = smoothstep(0.5, 0.1, d);

    return color * factor;
}

void main()
{
    vec2 uv = vUV;

    vec3 color = gradientColor(uv);

    color = applyGrid(color, uv);
    color = applyCircle(color, uv);
    color = vignette(color, uv);

    vec3 test한글 = vec3(0.0f);
    color += test한글;

    FragColor = vec4(color, 1.0);