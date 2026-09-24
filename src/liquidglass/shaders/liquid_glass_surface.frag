#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

// ============================================================
// Liquid Glass 2.0 std140 统一内存块 (严格 16 字节对齐)
// ============================================================
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;          // 0..63
    float qt_Opacity;        // 64..67
    float hasSource;         // 68..71
    float time;              // 72..75
    float opacity_;          // 76..79  [对齐 16]
    vec4 tint;               // 80..95  [对齐 16]
    vec4 edgeColor;          // 96..111 [对齐 16]
    vec2 pointer;            // 112..119
    vec2 resolution;         // 120..127 [对齐 16]
    float tintStr;           // 128..131
    float noise;             // 132..135
    float distortion;        // 136..139
    float highlight;         // 140..143 [对齐 16]
    float fresnel;           // 144..147
    float hoverState;        // 148..151
    float pressState;        // 152..155
    float cornerRadius;      // 156..159 [对齐 16]
    float lensMagnification;
    float refractionHeight;
    float bevelWidth;
    float refractionFalloff;
    float refractionNoFold;
    float refractionOutward;
    float adaptiveLensScale;
    float blurAmount;
    float saturation;
    float aberrationIntensity;
    float edgeHighlightEnabled;
    float edgeHighlightWidth;
    float edgeHighlightOpacity;
    float sensorHighlightEnabled;
    float adaptiveTint;
    float downsampleScale;
    vec2 capturePadding;
    float materialStyle;
    float dispersion;
    float progressiveMode;
    vec2 tilt;
    vec2 secondaryPos;
    vec2 secondarySize;
    float secondaryRadius;
    float secondaryActive;
    float sminFactor;
    float pressBulge;
    float _pad0;
    float _pad1;
} ubuf;

layout(binding = 1) uniform sampler2D source;

vec2 backdropUV(vec2 glassUV)
{
    return (ubuf.capturePadding + glassUV * ubuf.resolution)
            / (ubuf.resolution + ubuf.capturePadding * 2.0);
}

vec3 sampleBackdrop(vec2 glassUV)
{
    return texture(source, clamp(backdropUV(glassUV), 0.0, 1.0)).rgb;
}

// 微晶噪点哈希
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// 基础圆角矩形 SDF
float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

// Inigo Quilez 多项式平滑最小值 (smin - 双形状液态张力黏连融合)
float smin(float a, float b, float k) {
    if (k <= 0.001) return min(a, b);
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}

// 综合 SDF 距离场计算 (支持次级形状平滑黏连与按压物理凸起/凹陷)
float evalSceneSDF(vec2 p, vec2 halfSize, float rad) {
    // 1. 主形状 SDF
    float d = sdRoundedBox(p, halfSize, rad);

    // 2. 次级液态形状 smin 融合 (Metaball 粘连拉丝)
    if (ubuf.secondaryActive > 0.01) {
        vec2 p2 = p - ubuf.secondaryPos;
        float d2 = sdRoundedBox(p2, ubuf.secondarySize, ubuf.secondaryRadius);
        float k = max(ubuf.sminFactor, 4.0) * ubuf.secondaryActive;
        d = smin(d, d2, k);
    }

    // 3. 手指按压液态形变 (Press Fluid Dent & Bulge)
    if (ubuf.pressState > 0.01) {
        vec2 tp = p - (ubuf.pointer - vec2(0.5)) * ubuf.resolution;
        float tr = length(tp);
        float rPress = max(min(ubuf.resolution.x, ubuf.resolution.y) * 0.35, 16.0);
        // 按压中心为凹陷，周围环形隆起
        float dent = (tr - rPress) * 0.8 * max(ubuf.pressBulge, 0.0);
        float bulgeK = 18.0 * ubuf.pressState * max(ubuf.pressBulge, 0.0);
        d = smin(d, dent, bulgeK);
    }

    return d;
}

// 有限差分法计算平滑法线
vec2 calcSDFNormal(vec2 p, vec2 halfSize, float rad) {
    vec2 e = vec2(1.0, 0.0);
    float d1 = evalSceneSDF(p + e.xy, halfSize, rad);
    float d2 = evalSceneSDF(p - e.xy, halfSize, rad);
    float d3 = evalSceneSDF(p + e.yx, halfSize, rad);
    float d4 = evalSceneSDF(p - e.yx, halfSize, rad);
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

    // ============================================================
    // 1. 真实 SDF 形状与抗锯齿覆盖度
    // ============================================================
    float d = evalSceneSDF(p, halfSize, rad);
    float cov = clamp(0.5 - d / 1.5, 0.0, 1.0);
    if (cov <= 0.003) {
        fragColor = vec4(0.0);
        return;
    }

    vec2 n = calcSDFNormal(p, halfSize, rad);

    // ============================================================
    // 2. 真实透镜几何与边缘背景压缩环 (Lens Compression Ring)
    // ============================================================
    float scale = 1.0;
    if (ubuf.adaptiveLensScale > 0.5) {
        scale = mix(0.18, 1.0, smoothstep(24.0, 110.0, minDim));
    }
    float bevel = max(2.0, min(ubuf.bevelWidth * scale, minDim * 0.48));
    float refrBase = max(0.0, ubuf.refractionHeight * scale)
            * (ubuf.materialStyle > 0.5 ? 1.25 : 1.0);
    float bandW = clamp(bevel * 0.32, 2.0, max(2.0, bevel * 0.5));

    float t = clamp(-d / bevel, 0.0, 1.0);
    // 引力透镜逆幂剖面
    float falloff = max(0.0, ubuf.refractionFalloff);
    float slope;
    slope = falloff < 0.001
            ? 1.0 - t
            : clamp((pow(1.0 + 4.2 * t, -falloff) - pow(5.2, -falloff)) / (1.0 - pow(5.2, -falloff)), 0.0, 1.0);

    // 透镜边缘焦散压缩环 (Caustic Compression Ring)
    // 在靠近边缘转折处背景像素向心强烈聚拢，呈现真实厚玻璃杯底的压缩环
    float compressionRing = sin(clamp(t * 3.14159, 0.0, 3.14159)) * exp(-1.8 * t);
    float totalRefr = (refrBase + minDim * 0.12 * ubuf.pressState + ubuf.distortion * minDim) * (slope + compressionRing * 0.75);
    if (ubuf.refractionNoFold > 0.5) {
        totalRefr = min(totalRefr, bevel * 0.5);
    }
    vec2 offset = n * totalRefr * (ubuf.refractionOutward > 0.5 ? 1.0 : -1.0);

    // Broad convex-lens magnification: compress source coordinates toward the lens center.
    float lensStrength = clamp(ubuf.lensMagnification, 0.0, 0.85);
    if (lensStrength > 0.001) {
        float rNorm = clamp(length(p / halfSize), 0.0, 1.0);
        float lensCurvature = pow(clamp(1.0 - rNorm * rNorm, 0.0, 1.0), 0.85);
        offset -= p * (lensStrength * lensCurvature);
    }


    // ============================================================
    // 3. 物理三通道光谱色散 (Chromatic Aberration)
    // ============================================================
    float dispFactor = max(ubuf.dispersion, 0.0) * max(ubuf.aberrationIntensity, 0.0)
            * (ubuf.materialStyle > 0.5 ? 1.8 : 1.0);
    float disp = dispFactor * (slope + compressionRing * 0.5);

    vec2 uvG = uv + offset / ubuf.resolution;
    vec2 uvR = uv + (offset * (1.0 - disp)) / ubuf.resolution;
    vec2 uvB = uv + (offset * (1.0 + disp)) / ubuf.resolution;

    // ============================================================
    // 4. 背景高斯磨砂虚化与滚动渐进模糊 (Progressive Blur)
    // ============================================================
    float blurWeight = clamp(ubuf.blurAmount, 0.0, 1.0);

    // 渐进模糊模式判断 (ScrollEdgeBlurView)
    if (ubuf.progressiveMode > 0.5) {
        if (ubuf.progressiveMode < 1.5) {
            // 顶部渐进：从 top 模糊渐变到 bottom 清晰
            blurWeight *= 1.0 - smoothstep(0.0, 1.0, uv.y);
        } else {
            // 底部渐进：从 bottom 模糊渐变到 top 清晰
            blurWeight *= smoothstep(0.0, 1.0, uv.y);
        }
    }

    vec3 bgColor = vec3(0.08, 0.12, 0.18);
    float detectedLum = 0.5;

    if (ubuf.hasSource > 0.5) {
        vec2 sourceSize = ubuf.resolution + ubuf.capturePadding * 2.0;
        vec2 texel = vec2(1.0 / max(sourceSize.x / max(ubuf.downsampleScale, 1.0), 1.0),
                          1.0 / max(sourceSize.y / max(ubuf.downsampleScale, 1.0), 1.0)) * mix(0.35, 3.2, clamp(ubuf.blurAmount, 0.0, 1.0));
        vec3 c0 = sampleBackdrop(uvG) * 0.2270;
        vec3 c1 = sampleBackdrop(uvG + vec2(-texel.x, -texel.y)) * 0.1470;
        vec3 c2 = sampleBackdrop(uvG + vec2( texel.x, -texel.y)) * 0.1470;
        vec3 c3 = sampleBackdrop(uvG + vec2(-texel.x,  texel.y)) * 0.1470;
        vec3 c4 = sampleBackdrop(uvG + vec2( texel.x,  texel.y)) * 0.1470;
        vec3 c5 = sampleBackdrop(uvG + vec2(-texel.x * 2.2, 0.0)) * 0.0462;
        vec3 c6 = sampleBackdrop(uvG + vec2( texel.x * 2.2, 0.0)) * 0.0462;
        vec3 c7 = sampleBackdrop(uvG + vec2(0.0, -texel.y * 2.2)) * 0.0462;
        vec3 c8 = sampleBackdrop(uvG + vec2(0.0,  texel.y * 2.2)) * 0.0462;

        vec3 blurred = c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8;

        // 三通道色散结合清晰与模糊
        float rCol = sampleBackdrop(uvR).r * (1.0 - blurWeight) + blurred.r * blurWeight;
        float gCol = sampleBackdrop(uvG).g * (1.0 - blurWeight) + blurred.g * blurWeight;
        float bCol = sampleBackdrop(uvB).b * (1.0 - blurWeight) + blurred.b * blurWeight;
        bgColor = vec3(rCol, gCol, bCol);

        // 苹果 Vibrancy 饱和度提亮
        detectedLum = dot(bgColor, vec3(0.2126, 0.7152, 0.0722));
        float satNow = max(bgColor.r, max(bgColor.g, bgColor.b)) - min(bgColor.r, min(bgColor.g, bgColor.b));
        float room = 1.0 - smoothstep(0.20, 0.85, satNow);
        float hl = 1.0 - smoothstep(0.75, 0.98, detectedLum);
        float satFactor = max(0.0, ubuf.saturation);
        float amount = satFactor <= 1.0
                ? satFactor
                : 1.0 + (satFactor - 1.0) * mix(0.3, 1.0, room * hl);
        bgColor = clamp(mix(vec3(detectedLum), bgColor, amount), 0.0, 1.0);
    }

    // ============================================================
    // 5. 传感器与重力倾斜法线光照 (Sensor & Tilt Normal Lighting)
    // ============================================================
    // 倾斜向量带动 3D 虚拟光源移动
    vec2 tilt = ubuf.sensorHighlightEnabled > 0.5 ? ubuf.tilt : vec2(0.0);
    vec3 lightDir3D = normalize(vec3(-0.48 + tilt.x * 0.75, -0.84 + tilt.y * 0.75, 1.15));
    vec3 viewDir = vec3(0.0, 0.0, 1.0);

    // 构建微拱顶 3D 法线
    float domeZ = sqrt(max(0.001, 1.0 - slope * slope));
    vec3 normal3D = normalize(vec3(n.x * slope, n.y * slope, domeZ));

    // Blinn-Phong 镜面高光反射
    vec3 halfVec = normalize(lightDir3D + viewDir);
    float NdotH = max(dot(normal3D, halfVec), 0.0);
    float spec3D = pow(NdotH, 36.0);

    // 2D 双对称角度光瓣 (迎光面亮瓣 + 背光内壁反射瓣)
    vec2 lightDir2D = normalize(vec2(lightDir3D.x, lightDir3D.y));
    float facing = dot(n, -lightDir2D);
    float lobeF = pow(max(facing, 0.0), 4.5);
    float lobeB = pow(max(-facing, 0.0), 4.5);

    // 晶莹发丝亮线与内辉光
    float hair = clamp(1.0 - abs(d + ubuf.edgeHighlightWidth * 0.5) / max(ubuf.edgeHighlightWidth, 0.5), 0.0, 1.0) * ubuf.edgeHighlightEnabled * ubuf.edgeHighlightOpacity;
    float glowIn = clamp((-d - 1.0) / 2.0, 0.0, 1.0);
    float glow = glowIn * pow(clamp(1.0 - (-d - 2.5) / bandW, 0.0, 1.0), 1.5);
    float specRim = (hair * 0.85 * (lobeF + lobeB) + glow * 0.22 * lobeF + spec3D * 0.65) * ubuf.highlight;

    // ============================================================
    // 6. 菲涅尔与顶部微弧光
    // ============================================================
    float fresnelFactor = max(0.1, ubuf.fresnel);
    float fresnelRim = pow(1.0 - t, fresnelFactor) * (ubuf.materialStyle > 0.5 ? 0.45 : 0.28) * ubuf.highlight * ubuf.edgeHighlightEnabled * ubuf.edgeHighlightOpacity;
    float topSheen = smoothstep(-0.2, 0.9, -n.y) * smoothstep(bevel * 1.5, 0.0, abs(d + bevel * 0.4)) * 0.22 * ubuf.highlight;

    // ============================================================
    // 7. 材质变体 (Regular / Clear) 与亮度自适应合成
    // ============================================================
    vec3 glassColor;
    float volumeOpacity;

    if (ubuf.materialStyle > 0.5) {
        // [Clear 材质]：极致高透、清冽冰晶、透光度高、保留背景明度
        vec3 clearBed = (detectedLum < 0.35) ? vec3(0.08, 0.14, 0.22) : vec3(0.02, 0.04, 0.08);
        glassColor = mix(bgColor, clearBed, 0.14);
        vec3 adaptiveTint = mix(ubuf.tint.rgb, vec3(1.0) - ubuf.tint.rgb, smoothstep(0.72, 0.96, detectedLum) * ubuf.adaptiveTint);
        glassColor = mix(glassColor, adaptiveTint, ubuf.tintStr * 0.6);
        glassColor += mix(vec3(1.0), ubuf.edgeColor.rgb, 0.35) * (specRim * 1.25 + fresnelRim * 1.35 + topSheen);
        volumeOpacity = ubuf.opacity_ * 0.72 + hair * 0.24 + fresnelRim * 0.22;
        volumeOpacity = clamp(volumeOpacity, 0.10, 0.92) * cov;
    } else {
        // [Regular 材质]：Apple 磨砂经典、高可读性深色吸光层
        vec3 darkBed = vec3(0.04, 0.08, 0.14);
        glassColor = mix(bgColor, darkBed, 0.38);
        vec3 adaptiveTint = mix(ubuf.tint.rgb, vec3(1.0) - ubuf.tint.rgb, smoothstep(0.72, 0.96, detectedLum) * ubuf.adaptiveTint);
        glassColor = mix(glassColor, adaptiveTint, ubuf.tintStr);
        glassColor += mix(vec3(1.0), ubuf.edgeColor.rgb, 0.35) * (specRim + fresnelRim + topSheen);
        volumeOpacity = ubuf.opacity_ + hair * 0.20 + glow * 0.12 + fresnelRim * 0.15;
        volumeOpacity = clamp(volumeOpacity, 0.15, 0.96) * cov;
    }

    // 微晶噪点 (Subsurface Micro-Grain)
    float noiseGrain = (hash(uv * ubuf.resolution + vec2(1.7, 3.4)) - 0.5) * 0.022 * ubuf.noise;
    glassColor += vec3(noiseGrain);

    fragColor = vec4(glassColor * volumeOpacity, volumeOpacity) * ubuf.qt_Opacity;
}
