import QtQuick 2.12

Item {
    id: root

    // ============================================================
    // 公开属性 (Public Properties - 对标 OliverZhaohaibin/Qt-liquid-glass-widgets)
    // ============================================================

    // 背景捕获源（若未指定则启用独立微晶材质渲染）
    property Item backgroundSource: null

    // 材质参数（通透纯白微晶，彻底消除暗紫色杂色）
    property real baseOpacity: 0.42
    property color tintColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
    property real tintStrength: 0.25
    property real noiseAmount: 0.016
    property real distortionStrength: 0.015
    property real highlightIntensity: 0.85
    property real edgeFresnelPower: 2.2
    property color edgeHighlightColor: Qt.rgba(1.0, 1.0, 1.0, 0.95)
    property real cornerRadius: 14
    property real elevation: 0
    property color shadowColor: Qt.rgba(0, 0, 0, 0.3)

    // 交互状态追踪
    property bool hovered: false
    property bool pressed: false
    property point pointerPosition: Qt.point(width / 2, height / 2)
    // 列表滚动或外部移动同步驱动因子（用于在 Flickable 滑动时实时驱动背景采样更新）
    property real scrollSync: 0
    // 凸透镜物理放大率（0.0 为标准折射，>0 产生水滴凸透镜光学放大感）
    property real lensMagnification: 0.0

    // 动画时间基准
    property real animationTime: 0

    // 内容插槽
    default property alias content: contentContainer.data

    // 内部计算状态
    property real _effectiveHighlight: highlightIntensity
    property real _effectiveDistortion: distortionStrength

    readonly property point _normalizedPointer: Qt.point(
        pointerPosition.x / Math.max(width, 1),
        pointerPosition.y / Math.max(height, 1)
    )

    Timer {
        id: animTimer
        running: root.visible && root.opacity > 0.01
        repeat: true
        interval: 32 // ~30fps 节省算力并保持有机流动感
        onTriggered: root.animationTime += 0.032
    }

    onHoveredChanged: updateEffectiveValues()
    onPressedChanged: updateEffectiveValues()
    onHighlightIntensityChanged: updateEffectiveValues()
    onDistortionStrengthChanged: updateEffectiveValues()

    function updateEffectiveValues() {
        var baseHighlight = highlightIntensity
        if (pressed) baseHighlight += 0.35
        else if (hovered) baseHighlight += 0.20
        _effectiveHighlight = baseHighlight

        var baseDistortion = distortionStrength
        if (pressed) baseDistortion += 0.008
        else if (hovered) baseDistortion += 0.004
        _effectiveDistortion = baseDistortion
    }

    Behavior on _effectiveHighlight {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }
    Behavior on _effectiveDistortion {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    // ============================================================
    // 背景捕获与安全回退纹理 (ShaderEffectSource - 杜绝非法纹理类型)
    // ============================================================
    readonly property rect _capturedRect: {
        var _sync = root.scrollSync // 显式绑定滚动与位移，滑动列表时每帧同步重新计算背景物理采样区域
        if (!root.backgroundSource || !root.visible || root.width <= 0 || root.height <= 0) {
            return Qt.rect(0, 0, 1, 1)
        }
        try {
            var pt = root.mapToItem(root.backgroundSource, 0, 0)
            return Qt.rect(Math.max(0, pt.x), Math.max(0, pt.y), Math.max(1, root.width), Math.max(1, root.height))
        } catch(e) {
            return Qt.rect(0, 0, 1, 1)
        }
    }

    ShaderEffectSource {
        id: bgCapture
        sourceItem: root.backgroundSource
        sourceRect: root._capturedRect
        textureSize: Qt.size(Math.max(1, root.width * 0.25), Math.max(1, root.height * 0.25))
        smooth: true
        anchors.fill: parent
        recursive: false
        live: root.visible && (root.backgroundSource !== null)
        visible: false
    }

    // 安全默认微晶底板（必须通过 ShaderEffectSource 提供合法的 OpenGL 纹理，杜绝驱动崩溃为洋红）
    Item {
        id: fallbackItem
        width: 64
        height: 64
        visible: false
        Rectangle {
            anchors.fill: parent
            color: "#16202e"
        }
    }

    ShaderEffectSource {
        id: fallbackCapture
        sourceItem: fallbackItem
        recursive: false
        visible: false
    }

    // ============================================================
    // 真实光学液态玻璃 Shader (Qt 5.15 GLSL 适配)
    // 包含：圆角 SDF 法线、引力透镜逆幂折射、双对称高光瓣、菲涅尔微晶边缘
    // ============================================================
    ShaderEffect {
        id: glassShader
        anchors.fill: parent

        property variant source: root.backgroundSource ? bgCapture : fallbackCapture
        property real hasSource: root.backgroundSource ? 1.0 : 0.0

        property real time: root.animationTime
        property real opacity_: root.baseOpacity
        property color tint: root.tintColor
        property real tintStr: root.tintStrength
        property real noise: root.noiseAmount
        property real distortion: root._effectiveDistortion
        property real highlight: root._effectiveHighlight
        property real fresnel: root.edgeFresnelPower
        property color edgeColor: root.edgeHighlightColor
        property point pointer: root._normalizedPointer
        property real hoverState: root.hovered ? 1.0 : 0.0
        property real pressState: root.pressed ? 1.0 : 0.0
        property point resolution: Qt.point(root.width, root.height)
        property real cornerRadius: root.cornerRadius
        property real lensMagnification: root.lensMagnification

        vertexShader: "
            uniform highp mat4 qt_Matrix;
            attribute highp vec4 qt_Vertex;
            attribute highp vec2 qt_MultiTexCoord0;
            varying highp vec2 qt_TexCoord0;

            void main() {
                qt_TexCoord0 = qt_MultiTexCoord0;
                gl_Position = qt_Matrix * qt_Vertex;
            }
        "

        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform lowp float qt_Opacity;
            uniform sampler2D source;

            uniform highp float time;
            uniform highp float opacity_;
            uniform highp vec4 tint;
            uniform highp float tintStr;
            uniform highp float noise;
            uniform highp float distortion;
            uniform highp float highlight;
            uniform highp float fresnel;
            uniform highp vec4 edgeColor;
            uniform highp vec2 pointer;
            uniform highp float hoverState;
            uniform highp float pressState;
            uniform highp vec2 resolution;
            uniform highp float cornerRadius;
            uniform highp float lensMagnification;
            uniform lowp float hasSource;

            highp float hash(highp vec2 p) {
                return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
            }

            highp float sdRoundedBox(highp vec2 p, highp vec2 b, highp float r) {
                highp vec2 q = abs(p) - b + vec2(r);
                return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
            }

            highp vec2 calcNormal(highp vec2 p, highp vec2 b, highp float r) {
                highp float d1 = sdRoundedBox(p + vec2(1.0, 0.0), b, r);
                highp float d2 = sdRoundedBox(p - vec2(1.0, 0.0), b, r);
                highp float d3 = sdRoundedBox(p + vec2(0.0, 1.0), b, r);
                highp float d4 = sdRoundedBox(p - vec2(0.0, 1.0), b, r);
                highp vec2 n = vec2(d1 - d2, d3 - d4);
                highp float l = length(n);
                return (l > 0.0001) ? (n / l) : vec2(0.0, -1.0);
            }

            void main() {
                highp vec2 uv = qt_TexCoord0;
                highp vec2 p = (uv - vec2(0.5)) * resolution;
                highp vec2 halfSize = (resolution * 0.5) - vec2(1.0);
                highp float rad = clamp(cornerRadius, 1.0, min(halfSize.x, halfSize.y));
                highp float minDim = min(resolution.x, resolution.y);

                // 1. 真实圆角矩形 SDF 距离场与外法线
                highp float d = sdRoundedBox(p, halfSize, rad);
                highp float cov = clamp(0.5 - d / 1.5, 0.0, 1.0); // 边缘抗锯齿
                if (cov <= 0.003) {
                    gl_FragColor = vec4(0.0);
                    return;
                }

                highp vec2 n = calcNormal(p, halfSize, rad);

                // 2. 自适应透镜几何
                highp float bevel = min(minDim * 0.28, 22.0);
                highp float refrBase = min(minDim * 0.38, 24.0);
                highp float bandW = clamp(bevel * 0.30, 2.0, 6.0);

                // 3. 引力透镜逆幂剖面
                highp float t = clamp(-d / bevel, 0.0, 1.0);
                highp float gB = 0.04;
                highp float slope = clamp((pow(1.0 + 4.0 * t, -2.0) - gB) / (1.0 - gB), 0.0, 1.0);

                // 4. 折射位移与水滴流体推挤波纹 (Viscous Liquid Ripple & Squishy Displacement)
                highp float refr = (refrBase + 10.0 * pressState) * slope;
                highp vec2 offset = n * (-refr);

                // 凸透镜物理曲面放大（Convex Lens Magnification）
                // 模拟真实水滴放大镜：中心光线向内折射会聚，使透过玻璃看下方的文字和背景真实放大
                if (lensMagnification > 0.001) {
                    highp float rNorm = clamp(length(p / halfSize), 0.0, 1.0);
                    highp float lensCurvature = pow(clamp(1.0 - rNorm * rNorm, 0.0, 1.0), 0.85);
                    offset -= p * (lensMagnification * lensCurvature * 0.52);
                }

                if (pressState > 0.01) {
                    highp vec2 tp = p - (pointer - vec2(0.5)) * resolution;
                    highp float tr = length(tp);
                    highp float rMax = max(minDim * 0.42, 14.0);
                    // 按压中心水滴微透镜凹陷 + 外圈水波推挤凸起
                    highp float bump = pressState * exp(-(tr * tr) / (rMax * rMax * 0.5));
                    highp float ripple = pressState * sin(clamp(tr / rMax * 3.14159, 0.0, 3.14159)) * 0.75;
                    if (tr > 0.5) {
                        offset -= (tp / tr) * ((bump * 1.2 + ripple) * refr * 0.55);
                    }
                }

                // 5. 光谱色散
                highp float disp = 0.12 * slope;
                highp vec2 uvG = clamp(uv + offset / resolution, 0.0, 1.0);
                highp vec2 uvR = clamp(uv + (offset * (1.0 - disp)) / resolution, 0.0, 1.0);
                highp vec2 uvB = clamp(uv + (offset * (1.0 + disp)) / resolution, 0.0, 1.0);

                // 6. 背景高斯磨砂虚化
                highp vec3 bgColor = vec3(0.08, 0.12, 0.18);
                if (hasSource > 0.5) {
                    highp vec2 texel = vec2(1.0 / max(resolution.x * 0.25, 1.0), 1.0 / max(resolution.y * 0.25, 1.0)) * 2.6;
                    highp vec3 c0 = texture2D(source, uvG).rgb * 0.2270;
                    highp vec3 c1 = texture2D(source, clamp(uvG + vec2(-texel.x, -texel.y), 0.0, 1.0)).rgb * 0.1470;
                    highp vec3 c2 = texture2D(source, clamp(uvG + vec2( texel.x, -texel.y), 0.0, 1.0)).rgb * 0.1470;
                    highp vec3 c3 = texture2D(source, clamp(uvG + vec2(-texel.x,  texel.y), 0.0, 1.0)).rgb * 0.1470;
                    highp vec3 c4 = texture2D(source, clamp(uvG + vec2( texel.x,  texel.y), 0.0, 1.0)).rgb * 0.1470;
                    highp vec3 c5 = texture2D(source, clamp(uvG + vec2(-texel.x * 2.2, 0.0), 0.0, 1.0)).rgb * 0.0462;
                    highp vec3 c6 = texture2D(source, clamp(uvG + vec2( texel.x * 2.2, 0.0), 0.0, 1.0)).rgb * 0.0462;
                    highp vec3 c7 = texture2D(source, clamp(uvG + vec2(0.0, -texel.y * 2.2), 0.0, 1.0)).rgb * 0.0462;
                    highp vec3 c8 = texture2D(source, clamp(uvG + vec2(0.0,  texel.y * 2.2), 0.0, 1.0)).rgb * 0.0462;

                    highp vec3 blurred = c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8;
                    highp float r = texture2D(source, uvR).r * 0.60 + blurred.r * 0.40;
                    highp float b = texture2D(source, uvB).b * 0.60 + blurred.b * 0.40;
                    bgColor = vec3(r, blurred.g, b);

                    // 苹果 Vibrancy 饱和度提亮
                    highp float lum = dot(bgColor, vec3(0.2126, 0.7152, 0.0722));
                    highp float satNow = max(bgColor.r, max(bgColor.g, bgColor.b)) - min(bgColor.r, min(bgColor.g, bgColor.b));
                    highp float room = 1.0 - smoothstep(0.20, 0.85, satNow);
                    highp float hl = 1.0 - smoothstep(0.75, 0.98, lum);
                    highp float amount = 1.0 + 0.28 * mix(0.3, 1.0, room * hl);
                    bgColor = clamp(mix(vec3(lum), bgColor, amount), 0.0, 1.0);
                }

                // 7. 苹果 iOS 26 原版双对称角度瓣光照（迎光亮瓣 + 背光内壁全反射）
                highp vec2 lightDir = normalize(vec2(-0.50, -0.86)); // 左偏上光源
                highp float facing = dot(n, -lightDir);
                highp float lobeF = pow(max(facing, 0.0), 4.5);   // 迎光面亮瓣
                highp float lobeB = pow(max(-facing, 0.0), 4.5);  // 背光面内壁全反射瓣

                // 贴边晶莹发丝亮线（1.8px 宽度）+ 迎光面柔和内辉光
                highp float hair = clamp(1.0 - abs(d + 1.0) / 1.8, 0.0, 1.0);
                highp float glowIn = clamp((-d - 1.0) / 2.0, 0.0, 1.0);
                highp float glow = glowIn * pow(clamp(1.0 - (-d - 2.5) / bandW, 0.0, 1.0), 1.5);
                highp float specRim = (hair * 0.85 * (lobeF + lobeB) + glow * 0.22 * lobeF) * highlight;

                // 8. 真实厚玻璃微晶菲涅尔外环（让玻璃在任何背景上都具有纯净通透的晶体厚度）
                highp float fresnelRim = pow(1.0 - t, 2.5) * 0.28 * highlight;

                // 9. 顶部透镜微弧光（模拟液态表面张力反光）
                highp float topSheen = smoothstep(-0.2, 0.9, -n.y) * smoothstep(bevel * 1.5, 0.0, abs(d + bevel * 0.4)) * 0.20 * highlight;

                // 10. 触控/滑动跟随晶莹流光 + 衍射外光环 (Touch Fluid Spotlight & Halo - 优雅微晶，杜绝过曝死白)
                highp float pointerDist = length(uv - pointer);
                highp float pointerHighlight = exp(-pointerDist * pointerDist * 36.0) * highlight * (hoverState * 0.28 + pressState * 0.55);
                highp float touchHalo = exp(-pow(pointerDist - 0.18, 2.0) * 110.0) * highlight * (pressState * 0.25);

                // 11. 纯净微晶色彩吸收与透射融合（Pre-multiplied Alpha 模式）
                highp vec3 glassColor = mix(bgColor, tint.rgb, tintStr);
                glassColor += vec3(1.0) * (specRim + fresnelRim + topSheen + pointerHighlight + touchHalo);

                highp float volumeOpacity = opacity_ + hair * 0.20 + glow * 0.12 + fresnelRim * 0.15;
                volumeOpacity = clamp(volumeOpacity, 0.15, 0.96) * cov;

                // 严格 Pre-multiplied Alpha 输出，保证混合清澈透亮
                gl_FragColor = vec4(glassColor * volumeOpacity, volumeOpacity) * qt_Opacity;
            }
        "
    }

    // 物理圆角边缘保护遮罩（仅微弱保底边框，杜绝任何人工死白线）
    Rectangle {
        id: maskRect
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.10)
        border.width: 1
    }

    // 内容容器
    Item {
        id: contentContainer
        anchors.fill: parent
    }
}
