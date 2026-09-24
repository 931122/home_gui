#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 resolution;
    float blurStrength;
    float _pad;
} ubuf;

layout(binding = 1) uniform sampler2D source;

// 苹果磨砂微晶噪点生成器
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = qt_TexCoord0;
    vec2 res = max(ubuf.resolution, vec2(1.0));
    
    // 自适应模糊步长 (在 1/4 降采样纹理上进一步扩散，产生梦幻般的强力失焦虚化)
    vec2 step = (vec2(4.5) / res) * ubuf.blurStrength;
    
    // 9-Tap 旋转交错高斯加权模糊采样
    vec4 col = texture(source, uv) * 0.2270;
    col += texture(source, clamp(uv + vec2(-step.x, -step.y), 0.0, 1.0)) * 0.1470;
    col += texture(source, clamp(uv + vec2( step.x, -step.y), 0.0, 1.0)) * 0.1470;
    col += texture(source, clamp(uv + vec2(-step.x,  step.y), 0.0, 1.0)) * 0.1470;
    col += texture(source, clamp(uv + vec2( step.x,  step.y), 0.0, 1.0)) * 0.1470;
    col += texture(source, clamp(uv + vec2(-step.x * 2.2, 0.0), 0.0, 1.0)) * 0.0462;
    col += texture(source, clamp(uv + vec2( step.x * 2.2, 0.0), 0.0, 1.0)) * 0.0462;
    col += texture(source, clamp(uv + vec2(0.0, -step.y * 2.2), 0.0, 1.0)) * 0.0462;
    col += texture(source, clamp(uv + vec2(0.0,  step.y * 2.2), 0.0, 1.0)) * 0.0462;

    // 苹果 Vibrancy 饱和度与明度自适应补偿 (让被模糊的文字与视频卡片在暗色背景下呈现梦幻光晕)
    float lum = dot(col.rgb, vec3(0.2126, 0.7152, 0.0722));
    col.rgb = mix(vec3(lum), col.rgb, 1.25);

    // 苹果深空夜空磨砂物理基底融合 (iOS System Material Dark: #0a111c, 0.58 混合度)
    vec3 darkMaterial = vec3(0.04, 0.08, 0.14);
    vec3 blended = mix(col.rgb, darkMaterial, 0.58);

    // 微晶磨砂噪点 (Subsurface Micro-Grain: 细腻质感，彻底消除塑料感)
    float noise = (hash(uv * res + vec2(1.7, 3.4)) - 0.5) * 0.024;
    blended += vec3(noise);

    // 预乘 Alpha 输出
    float finalAlpha = ubuf.blurStrength * ubuf.qt_Opacity;
    fragColor = vec4(blended * finalAlpha, finalAlpha);
}
