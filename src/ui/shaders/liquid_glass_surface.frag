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
    float lensMagnification; // 160..163
    float materialStyle;     // 164..167 (0.0=Regular 磨砂可读性, 1.0=Clear 高透光强折射)
    float dispersion;        // 168..171 (物理光谱色散强度)
    float progressiveMode;   // 172..175 (0.0=全局, 1.0=顶部滚动渐进, 2.0=底部滚动渐进) [对齐 16]
    vec2 tilt;               // 176..183 (倾斜/光源微调向量)
    vec2 secondaryPos;       // 184..191 [对齐 16] (次级形状相对中心位置)
    vec2 secondarySize;      // 192..199
    float secondaryRadius;   // 200..203
    float secondaryActive;   // 204..207 [对齐 16]
    float sminFactor;        // 208..211 (融合平滑系数 k)
    float pressBulge;        // 212..215 (按压水银凸起强度)
    float _pad0;             // 216..219
    float _pad1;             // 220..223 [对齐 16]
} ubuf;

layout(binding = 1) uniform sampler2D source;

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
        float dent = (tr - rPress) * 0.8;
        float bulgeK = 18.0 * ubuf.pressState;
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
    float bevel = min(minDim * 0.32, 26.0);
    float refrBase = min(minDim * 0.42, 28.0) * (ubuf.materialStyle > 0.5 ? 1.6 : 1.0);
    float bandW = clamp(bevel * 0.32, 2.0, 7.0);

    float t = clamp(-d / bevel, 0.0, 1.0);
    // 引力透镜逆幂剖面
    float gB = 0.035;
    float slope = clamp((pow(1.0 + 4.2 * t, -2.0) - gB) / (1.0 - gB), 0.0, 1.0);

    // 透镜边缘焦散压缩环 (Caustic Compression Ring)
    // 在靠近边缘转折处背景像素向心强烈聚拢，呈现真实厚玻璃杯底的压缩环
    float compressionRing = sin(clamp(t * 3.14159, 0.0, 3.14159)) * exp(-1.8 * t);
    float totalRefr = (refrBase + 12.0 * ubuf.pressState) * (slope + compressionRing * 0.75);
    vec2 offset = n * (-totalRefr);

    // 凸透镜物理曲面放大 (Lens Magnification)
    if (ubuf.lensMagnification > 0.001) {
        float rNorm = clamp(length(p / halfSize), 0.0, 1.0);
        float lensCurvature = pow(clamp(1.0 - rNorm * rNorm, 0.0, 1.0), 0.85);
        offset -= p * (ubuf.lensMagnification * lensCurvature * 0.55);
    }

    // 手指按压水银波纹流体推挤
    if (ubuf.pressState > 0.01) {
        vec2 tp = p - (ubuf.pointer - vec2(0.5)) * ubuf.resolution;
        float tr = length(tp);
        float rMax = max(minDim * 0.45, 16.0);
        float bump = ubuf.pressState * exp(-(tr * tr) / (rMax * rMax * 0.45));
        float ripple = ubuf.pressState * sin(clamp(tr / rMax * 3.14159, 0.0, 3.14159)) * 0.85;
        if (tr > 0.5) {
            offset -= (tp / tr) * ((bump * 1.35 + ripple) * totalRefr * 0.6);
        }
    }

    // ============================================================
    // 3. 物理三通道光谱色散 (Chromatic Aberration)
    // ============================================================
    float dispFactor = max(ubuf.dispersion, 0.05) * (ubuf.materialStyle > 0.5 ? 1.8 : 1.0);
    float disp = dispFactor * (slope + compressionRing * 0.5);

    vec2 uvG = clamp(uv + offset / ubuf.resolution, 0.0, 1.0);
    vec2 uvR = clamp(uv + (offset * (1.0 - disp)) / ubuf.resolution, 0.0, 1.0);
    vec2 uvB = clamp(uv + (offset * (1.0 + disp)) / ubuf.resolution, 0.0, 1.0);

    // ============================================================
    // 4. 背景高斯磨砂虚化与滚动渐进模糊 (Progressive Blur)
    // ============================================================
    float blurWeight = (ubuf.materialStyle > 0.5) ? 0.42 : 0.88;

    // 渐进模糊模式判断 (ScrollEdgeBlurView)
    if (ubuf.progressiveMode > 0.5) {
        if (ubuf.progressiveMode < 1.5) {
            // 顶部渐进：从 top 模糊渐变到 bottom 清晰
            blurWeight *= smoothstep(1.0, 0.0, uv.y);
        } else {
            // 底部渐进：从 bottom 模糊渐变到 top 清晰
            blurWeight *= smoothstep(0.0, 1.0, uv.y);
        }
    }

    vec3 bgColor = vec3(0.08, 0.12, 0.18);
    float detectedLum = 0.5;

    if (ubuf.hasSource > 0.5) {
        vec2 texel = vec2(1.0 / max(ubuf.resolution.x * 0.25, 1.0), 1.0 / max(ubuf.resolution.y * 0.25, 1.0)) * 2.8;
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

        // 三通道色散结合清晰与模糊
        float rCol = texture(source, uvR).r * (1.0 - blurWeight) + blurred.r * blurWeight;
        float gCol = texture(source, uvG).g * (1.0 - blurWeight) + blurred.g * blurWeight;
        float bCol = texture(source, uvB).b * (1.0 - blurWeight) + blurred.b * blurWeight;
        bgColor = vec3(rCol, gCol, bCol);

        // 苹果 Vibrancy 饱和度提亮
        detectedLum = dot(bgColor, vec3(0.2126, 0.7152, 0.0722));
        float satNow = max(bgColor.r, max(bgColor.g, bgColor.b)) - min(bgColor.r, min(bgColor.g, bgColor.b));
        float room = 1.0 - smoothstep(0.20, 0.85, satNow);
        float hl = 1.0 - smoothstep(0.75, 0.98, detectedLum);
        float amount = 1.0 + 0.30 * mix(0.3, 1.0, room * hl);
        bgColor = clamp(mix(vec3(detectedLum), bgColor, amount), 0.0, 1.0);
    }

    // ============================================================
    // 5. 传感器与重力倾斜法线光照 (Sensor & Tilt Normal Lighting)
    // ============================================================
    // 倾斜向量带动 3D 虚拟光源移动
    vec3 lightDir3D = normalize(vec3(-0.48 + ubuf.tilt.x * 0.75, -0.84 + ubuf.tilt.y * 0.75, 1.15));
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
    float hair = clamp(1.0 - abs(d + 1.0) / 1.8, 0.0, 1.0);
    float glowIn = clamp((-d - 1.0) / 2.0, 0.0, 1.0);
    float glow = glowIn * pow(clamp(1.0 - (-d - 2.5) / bandW, 0.0, 1.0), 1.5);
    float specRim = (hair * 0.85 * (lobeF + lobeB) + glow * 0.22 * lobeF + spec3D * 0.65) * ubuf.highlight;

    // ============================================================
    // 6. 菲涅尔与顶部微弧光
    // ============================================================
    float fresnelFactor = (ubuf.materialStyle > 0.5) ? 2.0 : 2.5;
    float fresnelRim = pow(1.0 - t, fresnelFactor) * (ubuf.materialStyle > 0.5 ? 0.45 : 0.28) * ubuf.highlight;
    float topSheen = smoothstep(-0.2, 0.9, -n.y) * smoothstep(bevel * 1.5, 0.0, abs(d + bevel * 0.4)) * 0.22 * ubuf.highlight;

    // 触摸流光与衍射光环
    float pointerDist = length(uv - ubuf.pointer);
    float pointerHighlight = exp(-pointerDist * pointerDist * 36.0) * ubuf.highlight * (ubuf.hoverState * 0.28 + ubuf.pressState * 0.60);
    float touchHalo = exp(-pow(pointerDist - 0.18, 2.0) * 110.0) * ubuf.highlight * (ubuf.pressState * 0.28);

    // ============================================================
    // 7. 材质变体 (Regular / Clear) 与亮度自适应合成
    // ============================================================
    vec3 glassColor;
    float volumeOpacity;

    if (ubuf.materialStyle > 0.5) {
        // [Clear 材质]：极致高透、清冽冰晶、透光度高、保留背景明度
        vec3 clearBed = (detectedLum < 0.35) ? vec3(0.08, 0.14, 0.22) : vec3(0.02, 0.04, 0.08);
        glassColor = mix(bgColor, clearBed, 0.14);
        glassColor = mix(glassColor, ubuf.tint.rgb, ubuf.tintStr * 0.6);
        glassColor += vec3(1.0) * (specRim * 1.25 + fresnelRim * 1.35 + topSheen + pointerHighlight + touchHalo);
        volumeOpacity = ubuf.opacity_ * 0.72 + hair * 0.24 + fresnelRim * 0.22;
        volumeOpacity = clamp(volumeOpacity, 0.10, 0.92) * cov;
    } else {
        // [Regular 材质]：Apple 磨砂经典、高可读性深色吸光层
        vec3 darkBed = vec3(0.04, 0.08, 0.14);
        glassColor = mix(bgColor, darkBed, 0.38);
        glassColor = mix(glassColor, ubuf.tint.rgb, ubuf.tintStr);
        glassColor += vec3(1.0) * (specRim + fresnelRim + topSheen + pointerHighlight + touchHalo);
        volumeOpacity = ubuf.opacity_ + hair * 0.20 + glow * 0.12 + fresnelRim * 0.15;
        volumeOpacity = clamp(volumeOpacity, 0.15, 0.96) * cov;
    }

    // 微晶噪点 (Subsurface Micro-Grain)
    float noiseGrain = (hash(uv * ubuf.resolution + vec2(1.7, 3.4)) - 0.5) * 0.022 * ubuf.noise;
    glassColor += vec3(noiseGrain);

    // 预乘 Alpha 输出
    fragColor = vec4(glassColor * volumeOpacity, volumeOpacity) * ubuf.qt_Opacity;
}
