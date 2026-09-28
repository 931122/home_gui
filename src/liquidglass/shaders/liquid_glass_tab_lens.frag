#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;            // 0..63 (64 bytes)
    float qt_Opacity;          // 64..67 (4 bytes)
    float bevel;               // 68..71 (4 bytes)
    vec2 dropletPos;           // 72..79 (8 bytes)
    vec2 dropletSize;          // 80..87 (8 bytes)
    vec2 contentSize;          // 88..95 (8 bytes)
    vec2 tilt;                 // 96..103 (8 bytes)
    float refractPx;           // 104..107 (4 bytes)
    float dispersion;          // 108..111 (4 bytes)
    vec4 accentColor;          // 112..127 (16 bytes, aligned to 16)
} ubuf;

layout(binding = 1) uniform sampler2D source;

// 基础圆角矩形 SDF
float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    // 1. 真实物理透镜几何空间
    // 计算当前片元在整个 TabBar 内容坐标系中的精确位置 screenPos
    vec2 screenPos = ubuf.dropletPos + qt_TexCoord0 * ubuf.dropletSize;
    vec2 halfSize = (ubuf.dropletSize * 0.5) - vec2(0.5);
    vec2 p = (qt_TexCoord0 - vec2(0.5)) * ubuf.dropletSize;
    float rad = min(halfSize.x, halfSize.y);

    // 2. 真实圆角透镜 SDF 边缘与抗锯齿覆盖率
    float d = sdRoundedBox(p, halfSize, rad);
    float cov = clamp(0.5 - d / 1.5, 0.0, 1.0);
    if (cov <= 0.004) {
        fragColor = vec4(0.0);
        return;
    }

    // 3. SDF 数值梯度 -> 屏幕空间外法线 (Screen-Space Normal)
    vec2 eps = vec2(1.0, 0.0);
    vec2 n = vec2(
        sdRoundedBox(p + eps.xy, halfSize, rad) - sdRoundedBox(p - eps.xy, halfSize, rad),
        sdRoundedBox(p + eps.yx, halfSize, rad) - sdRoundedBox(p - eps.yx, halfSize, rad)
    );
    float nLen = length(n);
    n = (nLen > 0.0001) ? (n / nLen) : vec2(0.0, -1.0);

    // 4. 厚度与斜面剖面 (对齐 QWEA0/Liquid-Glass-Android 核心算法)
    // t=1 平坦内部 (slope=0 无任何畸变)，t=0 边缘 (slope=1 贴边弯折最大)
    float bevelW = max(ubuf.bevel, 1.0);
    float t = clamp(-d / bevelW, 0.0, 1.0);
    float edge = 1.0 - t;
    float slope = edge * edge; // 平方斜面：内部平坦、弯折在贴近边缘处自然升起

    // 5. 真实物理折射位移 (向内折射采样，贴边文字与背景被吸入透镜边缘)
    float refr = ubuf.refractPx;
    vec2 offset = -n * (slope * refr);

    // 6. 物理光谱色散 (Chromatic Dispersion along normal/offset)
    vec2 cR = screenPos + offset * (1.0 - ubuf.dispersion * slope);
    vec2 cG = screenPos + offset;
    vec2 cB = screenPos + offset * (1.0 + ubuf.dispersion * slope);

    vec2 rCoord = clamp(cR / ubuf.contentSize, 0.0, 1.0);
    vec2 gCoord = clamp(cG / ubuf.contentSize, 0.0, 1.0);
    vec2 bCoord = clamp(cB / ubuf.contentSize, 0.0, 1.0);

    vec3 col = vec3(
        texture(source, rCoord).r,
        texture(source, gCoord).g,
        texture(source, bCoord).b
    );

    // 7. 苹果冰晶透镜提亮与本体微透染色 (Apple Luminescence & Medium Translucency)
    // 滴叠加折射后比底层轨道更亮更凸 (Luminescence)
    col = col * 1.16 + vec3(0.05, 0.075, 0.10);

    // 顶部柔和弧面微天光
    float topSky = (1.0 - smoothstep(-halfSize.y * 0.4, halfSize.y, p.y)) * 0.06;
    col += vec3(topSky);

    // 晶体本体微染色
    if (ubuf.accentColor.a > 0.002) {
        vec3 tint = mix(ubuf.accentColor.rgb * 1.25, vec3(1.0), 0.15);
        col = mix(col, col * 0.88 + tint * 0.28, ubuf.accentColor.a);
    }

    // 8. 双对称角度瓣镜面高光 (Specular Lobes from QWEA0/Liquid-Glass-Android)
    vec2 toLight = normalize(vec2(-0.45 + ubuf.tilt.x * 0.5, -0.85 + ubuf.tilt.y * 0.5));
    float facing = dot(n, toLight);
    float lobeF = pow(max(facing, 0.0), 4.0);
    float lobeB = pow(max(-facing, 0.0), 4.0);

    // 贴边亮线 (中心在边内 1px，半宽 2px) + 迎光侧微辉光
    float bandW = clamp(bevelW * 0.40, 2.0, 6.0);
    float glowIn = clamp((-d - 1.0) / 2.0, 0.0, 1.0);
    float glow = glowIn * pow(clamp(1.0 - (-d - 2.5) / bandW, 0.0, 1.0), 1.5) * cov;
    float hair = clamp(1.0 - abs(d + 1.0) / 2.0, 0.0, 1.0) * cov;
    float spec = hair * 0.75 * (lobeF + lobeB * 0.5) + glow * 0.28 * lobeF + 0.16 * hair;
    col += vec3(spec);

    col = clamp(col, vec3(0.0), vec3(1.0));

    // 9. 输出抗锯齿预乘 Alpha (Premultiplied RGBA)
    fragColor = vec4(col * (cov * ubuf.qt_Opacity), cov * ubuf.qt_Opacity);
}
