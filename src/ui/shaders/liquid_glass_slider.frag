#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float progress;
    float stretchFactor;
    float scaleFactor;
    float alertFactor;
    vec2 resolution;
} ubuf;

layout(binding = 1) uniform sampler2D source;

float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 uv = qt_TexCoord0;
    vec2 p = (uv - vec2(0.5)) * ubuf.resolution;
    vec2 halfSize = (ubuf.resolution * 0.5) - vec2(1.0);
    float pad = 3.0;
    float trackR = halfSize.y - pad;

    // 1. 跑道边缘 SDF
    float dTrack = sdRoundedBox(p, halfSize - vec2(pad), trackR);
    float covTrack = clamp(0.5 - dTrack / 1.5, 0.0, 1.0);
    if (covTrack <= 0.002) {
        fragColor = vec4(0.0);
        return;
    }

    // 2. 跑道两端坐标与饱满圆润水滴半径
    float baseKnobR = trackR * 0.80;
    float curKnobR  = baseKnobR * ubuf.scaleFactor;
    float xMin = -halfSize.x + pad + trackR;
    float xMax =  halfSize.x - pad - trackR;

    vec2 p0 = vec2(xMin, 0.0);
    vec2 p1 = vec2(mix(xMin, xMax, ubuf.progress), 0.0);

    // 3. 原点母水滴与滑动水滴
    float r0 = baseKnobR * clamp(0.70 - ubuf.progress * 2.2, 0.0, 0.70);
    float d0 = length(p - p0) - r0;

    // 滑动水滴受力流线型拉长
    vec2 p1_local = p - p1;
    p1_local.x /= (1.0 + ubuf.stretchFactor * 0.22);
    p1_local.y *= (1.0 + ubuf.stretchFactor * 0.12);
    float d1 = length(p1_local) - curKnobR;

    // 4. 表面张力流体融球 (Metaball Liquid Bridge)
    float bridgeStrength = clamp(1.0 - smoothstep(0.08, 0.28, ubuf.progress), 0.0, 1.0);
    float blendK = 18.0 * bridgeStrength;
    float dFluid = (blendK > 0.5 && r0 > 0.8) ? smin(d0, d1, blendK) : d1;

    // 5. 梯度法线场计算
    float eps = 1.0;
    vec2 p_px = p + vec2(eps, 0.0);
    vec2 p_mx = p - vec2(eps, 0.0);
    vec2 p_py = p + vec2(0.0, eps);
    vec2 p_my = p - vec2(0.0, eps);

    vec2 p1_px = p_px - p1; p1_px.x /= (1.0 + ubuf.stretchFactor * 0.22); p1_px.y *= (1.0 + ubuf.stretchFactor * 0.12);
    vec2 p1_mx = p_mx - p1; p1_mx.x /= (1.0 + ubuf.stretchFactor * 0.22); p1_mx.y *= (1.0 + ubuf.stretchFactor * 0.12);
    vec2 p1_py = p_py - p1; p1_py.x /= (1.0 + ubuf.stretchFactor * 0.22); p1_py.y *= (1.0 + ubuf.stretchFactor * 0.12);
    vec2 p1_my = p_my - p1; p1_my.x /= (1.0 + ubuf.stretchFactor * 0.22); p1_my.y *= (1.0 + ubuf.stretchFactor * 0.12);

    float dF_px = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_px - p0) - r0, length(p1_px) - curKnobR, blendK) : (length(p1_px) - curKnobR);
    float dF_mx = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_mx - p0) - r0, length(p1_mx) - curKnobR, blendK) : (length(p1_mx) - curKnobR);
    float dF_py = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_py - p0) - r0, length(p1_py) - curKnobR, blendK) : (length(p1_py) - curKnobR);
    float dF_my = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_my - p0) - r0, length(p1_my) - curKnobR, blendK) : (length(p1_my) - curKnobR);

    vec2 n = normalize(vec2(dF_px - dF_mx, dF_py - dF_my) + vec2(0.0001));

    // 6. 凸透镜物理放大与折射计算
    vec2 totalOffset = vec2(0.0);
    float distToCenter = length(p - p1);
    float normDist = distToCenter / max(curKnobR, 1.0);

    if (normDist < 1.0) {
        float lensShape = sqrt(max(0.0, 1.0 - normDist * normDist));
        float mag = 0.28 * pow(lensShape, 0.85);
        vec2 magnifyOffset = -(p - p1) * mag;
        vec2 edgeRefract = n * (-4.5 * (1.0 - lensShape));
        totalOffset = magnifyOffset + edgeRefract;
    } else if (dFluid < 0.0) {
        float bridgeSlope = clamp(1.0 - (-dFluid) / 4.0, 0.0, 1.0);
        totalOffset = n * (-5.0 * bridgeSlope);
    }

    // 7. 色散真实光学采样
    vec2 uvR = clamp(uv + (totalOffset * 1.02) / ubuf.resolution, 0.0, 1.0);
    vec2 uvG = clamp(uv + totalOffset / ubuf.resolution, 0.0, 1.0);
    vec2 uvB = clamp(uv + (totalOffset * 0.98) / ubuf.resolution, 0.0, 1.0);

    vec4 baseCol = texture(source, uv);
    vec3 refrCol = vec3(
        texture(source, uvR).r,
        texture(source, uvG).g,
        texture(source, uvB).b
    );

    // 8. 苹果双对称高光瓣
    vec2 lightDir = normalize(vec2(-0.55, -0.83));
    float facing = dot(n, -lightDir);
    float lobeF = pow(max(facing, 0.0), 4.5);
    float lobeB = pow(max(-facing, 0.0), 3.5) * 0.65;

    float hair = clamp(1.0 - abs(dFluid + 0.8) / 1.2, 0.0, 1.0);
    float specRim = hair * (lobeF + lobeB) * 0.98;

    // 水滴表面穹顶光泽与中心高光
    float domeSheen = smoothstep(-0.2, 0.9, -n.y) * clamp(-dFluid / curKnobR, 0.0, 1.0) * 0.25;
    float centerSpot = pow(clamp(1.0 - normDist, 0.0, 1.0), 3.0) * 0.15;

    // 9. 颜色调和与警报红宝石流体
    vec3 normalGlassTint = vec3(0.95, 0.97, 1.0);
    vec3 alertGlassTint  = vec3(1.0, 0.24, 0.20);
    vec3 currentTint = mix(normalGlassTint, alertGlassTint, ubuf.alertFactor);

    vec3 waterDropColor = mix(refrCol, currentTint, mix(0.14, 0.60, ubuf.alertFactor));
    waterDropColor += vec3(1.0) * (specRim + domeSheen + centerSpot);

    // 10. 最终覆盖合成
    float covFluid = clamp(0.5 - dFluid / 1.5, 0.0, 1.0);
    float fluidAlpha = mix(0.40, 0.72, ubuf.alertFactor) * covFluid;

    vec3 finalRGB = mix(baseCol.rgb, waterDropColor, covFluid);
    float finalAlpha = max(baseCol.a, fluidAlpha) * covTrack;

    fragColor = vec4(finalRGB * finalAlpha, finalAlpha) * ubuf.qt_Opacity;
}
