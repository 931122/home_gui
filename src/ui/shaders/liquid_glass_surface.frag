#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float hasSource;
    float time;
    float opacity_;
    vec4 tint;
    vec4 edgeColor;
    vec2 pointer;
    vec2 resolution;
    float tintStr;
    float noise;
    float distortion;
    float highlight;
    float fresnel;
    float hoverState;
    float pressState;
    float cornerRadius;
    float lensMagnification;
} ubuf;

layout(binding = 1) uniform sampler2D source;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

vec2 calcNormal(vec2 p, vec2 b, float r) {
    float d1 = sdRoundedBox(p + vec2(1.0, 0.0), b, r);
    float d2 = sdRoundedBox(p - vec2(1.0, 0.0), b, r);
    float d3 = sdRoundedBox(p + vec2(0.0, 1.0), b, r);
    float d4 = sdRoundedBox(p - vec2(0.0, 1.0), b, r);
    vec2 n = vec2(d1 - d2, d3 - d4);
    float l = length(n);
    return (l > 0.0001) ? (n / l) : vec2(0.0, -1.0);
}

void main() {
    vec2 uv = qt_TexCoord0;
    vec2 p = (uv - vec2(0.5)) * ubuf.resolution;
    vec2 halfSize = (ubuf.resolution * 0.5) - vec2(1.0);
    float rad = clamp(ubuf.cornerRadius, 1.0, min(halfSize.x, halfSize.y));
    float minDim = min(ubuf.resolution.x, ubuf.resolution.y);

    // 1. 真实圆角矩形 SDF 距离场与外法线
    float d = sdRoundedBox(p, halfSize, rad);
    float cov = clamp(0.5 - d / 1.5, 0.0, 1.0); // 边缘抗锯齿
    if (cov <= 0.003) {
        fragColor = vec4(0.0);
        return;
    }

    vec2 n = calcNormal(p, halfSize, rad);

    // 2. 自适应透镜几何
    float bevel = min(minDim * 0.28, 22.0);
    float refrBase = min(minDim * 0.38, 24.0);
    float bandW = clamp(bevel * 0.30, 2.0, 6.0);

    // 3. 引力透镜逆幂剖面
    float t = clamp(-d / bevel, 0.0, 1.0);
    float gB = 0.04;
    float slope = clamp((pow(1.0 + 4.0 * t, -2.0) - gB) / (1.0 - gB), 0.0, 1.0);

    // 4. 折射位移与水滴流体推挤波纹
    float refr = (refrBase + 10.0 * ubuf.pressState) * slope;
    vec2 offset = n * (-refr);

    // 凸透镜物理曲面放大
    if (ubuf.lensMagnification > 0.001) {
        float rNorm = clamp(length(p / halfSize), 0.0, 1.0);
        float lensCurvature = pow(clamp(1.0 - rNorm * rNorm, 0.0, 1.0), 0.85);
        offset -= p * (ubuf.lensMagnification * lensCurvature * 0.52);
    }

    if (ubuf.pressState > 0.01) {
        vec2 tp = p - (ubuf.pointer - vec2(0.5)) * ubuf.resolution;
        float tr = length(tp);
        float rMax = max(minDim * 0.42, 14.0);
        float bump = ubuf.pressState * exp(-(tr * tr) / (rMax * rMax * 0.5));
        float ripple = ubuf.pressState * sin(clamp(tr / rMax * 3.14159, 0.0, 3.14159)) * 0.75;
        if (tr > 0.5) {
            offset -= (tp / tr) * ((bump * 1.2 + ripple) * refr * 0.55);
        }
    }

    // 5. 光谱色散
    float disp = 0.12 * slope;
    vec2 uvG = clamp(uv + offset / ubuf.resolution, 0.0, 1.0);
    vec2 uvR = clamp(uv + (offset * (1.0 - disp)) / ubuf.resolution, 0.0, 1.0);
    vec2 uvB = clamp(uv + (offset * (1.0 + disp)) / ubuf.resolution, 0.0, 1.0);

    // 6. 背景高斯磨砂虚化
    vec3 bgColor = vec3(0.08, 0.12, 0.18);
    if (ubuf.hasSource > 0.5) {
        vec2 texel = vec2(1.0 / max(ubuf.resolution.x * 0.25, 1.0), 1.0 / max(ubuf.resolution.y * 0.25, 1.0)) * 2.6;
        vec3 c0 = texture(source, uvG).rgb * 0.2270;
        vec3 c1 = texture(source, clamp(uvG + vec2(-texel.x, -texel.y), 0.0, 1.0)).rgb * 0.1470;
        vec3 c2 = texture(source, clamp(uvG + vec2( texel.x, -texel.y), 0.0, 1.0)).rgb * 0.1470;
        vec3 c3 = texture(source, clamp(uvG + vec2(-texel.x,  texel.y), 0.0, 1.0)).rgb * 0.1470;
        vec3 c4 = texture(source, clamp(uvG + vec2( texel.x,  texel.y), 0.0, 1.0)).rgb * 0.1470;
        vec3 c5 = texture(source, clamp(uvG + vec2(-texel.x * 2.2, 0.0), 0.0, 1.0)).rgb * 0.0462;
        vec3 c6 = texture(source, clamp(uvG + vec2( texel.x * 2.2, 0.0), 0.0, 1.0)).rgb * 0.0462;
        vec3 c7 = texture(source, clamp(uvG + vec2(0.0, -texel.y * 2.2), 0.0, 1.0)).rgb * 0.0462;
        vec3 c8 = texture(source, clamp(uvG + vec2(0.0,  texel.y * 2.2), 0.0, 1.0)).rgb * 0.0462;

        vec3 blurred = c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8;
        float r = texture(source, uvR).r * 0.60 + blurred.r * 0.40;
        float b = texture(source, uvB).b * 0.60 + blurred.b * 0.40;
        bgColor = vec3(r, blurred.g, b);

        // 苹果 Vibrancy 饱和度提亮
        float lum = dot(bgColor, vec3(0.2126, 0.7152, 0.0722));
        float satNow = max(bgColor.r, max(bgColor.g, bgColor.b)) - min(bgColor.r, min(bgColor.g, bgColor.b));
        float room = 1.0 - smoothstep(0.20, 0.85, satNow);
        float hl = 1.0 - smoothstep(0.75, 0.98, lum);
        float amount = 1.0 + 0.28 * mix(0.3, 1.0, room * hl);
        bgColor = clamp(mix(vec3(lum), bgColor, amount), 0.0, 1.0);
    }

    // 7. 苹果 iOS 原版双对称角度瓣光照
    vec2 lightDir = normalize(vec2(-0.50, -0.86)); // 左偏上光源
    float facing = dot(n, -lightDir);
    float lobeF = pow(max(facing, 0.0), 4.5);   // 迎光面亮瓣
    float lobeB = pow(max(-facing, 0.0), 4.5);  // 背光面内壁全反射瓣

    // 贴边晶莹发丝亮线（1.8px 宽度）+ 迎光面柔和内辉光
    float hair = clamp(1.0 - abs(d + 1.0) / 1.8, 0.0, 1.0);
    float glowIn = clamp((-d - 1.0) / 2.0, 0.0, 1.0);
    float glow = glowIn * pow(clamp(1.0 - (-d - 2.5) / bandW, 0.0, 1.0), 1.5);
    float specRim = (hair * 0.85 * (lobeF + lobeB) + glow * 0.22 * lobeF) * ubuf.highlight;

    // 8. 真实厚玻璃微晶菲涅尔外环
    float fresnelRim = pow(1.0 - t, 2.5) * 0.28 * ubuf.highlight;

    // 9. 顶部透镜微弧光
    float topSheen = smoothstep(-0.2, 0.9, -n.y) * smoothstep(bevel * 1.5, 0.0, abs(d + bevel * 0.4)) * 0.20 * ubuf.highlight;

    // 10. 触控/滑动跟随晶莹流光 + 衍射外光环
    float pointerDist = length(uv - ubuf.pointer);
    float pointerHighlight = exp(-pointerDist * pointerDist * 36.0) * ubuf.highlight * (ubuf.hoverState * 0.28 + ubuf.pressState * 0.55);
    float touchHalo = exp(-pow(pointerDist - 0.18, 2.0) * 110.0) * ubuf.highlight * (ubuf.pressState * 0.25);

    // 11. 纯净微晶色彩吸收与透射融合
    vec3 glassColor = mix(bgColor, ubuf.tint.rgb, ubuf.tintStr);
    glassColor += vec3(1.0) * (specRim + fresnelRim + topSheen + pointerHighlight + touchHalo);

    float volumeOpacity = ubuf.opacity_ + hair * 0.20 + glow * 0.12 + fresnelRim * 0.15;
    volumeOpacity = clamp(volumeOpacity, 0.15, 0.96) * cov;

    fragColor = vec4(glassColor * volumeOpacity, volumeOpacity) * ubuf.qt_Opacity;
}
