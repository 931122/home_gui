#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

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

void main()
{
    qt_TexCoord0 = qt_MultiTexCoord0;
    gl_Position = ubuf.qt_Matrix * qt_Vertex;
}
