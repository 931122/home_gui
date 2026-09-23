import QtQuick 2.12

Item {
    id: sliderRoot

    // ============================================================
    // 公开属性 (Public Properties)
    // ============================================================
    property real progress: 0.0 // 0.0 ~ 1.0 滑动进度
    property bool isDragging: false
    property bool interactive: false // 若由外部 MouseArea 驱动则设为 false
    property string text: qsTr("向右滑动关闭")
    property string alertText: qsTr("松手以关闭")
    property color alertColor: "#ff453a"
    property Item backgroundSource: null
    property real scrollSync: 0

    // 交互信号
    signal triggered()
    signal canceled()

    // 内部缓动状态
    readonly property bool isAlert: progress >= 0.60
    property real alertFactor: isAlert ? 1.0 : 0.0
    Behavior on alertFactor { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }

    // 苹果物理手感：按住/拖动时水滴轻微饱满放大 (自然优雅，杜绝臃肿)
    property real scaleFactor: isDragging ? 1.08 : 1.0
    Behavior on scaleFactor {
        NumberAnimation {
            duration: 160
            easing.type: Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // 滑动受力横向轻微拉长拉伸
    property real stretchFactor: isDragging ? Math.min(0.22, progress * 0.32) : 0.0
    Behavior on stretchFactor { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

    // 回弹弹性动画
    NumberAnimation {
        id: springResetAnim
        target: sliderRoot
        property: "progress"
        to: 0.0
        duration: 320
        easing.type: Easing.OutBack
        easing.overshoot: 1.25
    }

    function reset() {
        springResetAnim.start()
    }

    // ============================================================
    // 1. 底层内容层：用于被 Shader 动态折射与物理放大
    // ============================================================
    Item {
        id: contentLayer
        anchors.fill: parent
        visible: false

        // 跑道半透明微晶槽基底
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: Qt.rgba(0.04, 0.08, 0.16, 0.52)
            border.color: sliderRoot.alertFactor > 0.1 
                          ? Qt.rgba(1.0, 0.42, 0.35, 0.45 * sliderRoot.alertFactor) 
                          : Qt.rgba(1.0, 1.0, 1.0, 0.18)
            border.width: 1

            // 跑道中心指引文字（供凸透镜水滴划过时真实物理放大）
            Text {
                id: trackLabel
                anchors.centerIn: parent
                text: sliderRoot.isAlert ? sliderRoot.alertText : sliderRoot.text
                color: sliderRoot.isAlert ? "#ffffff" : Qt.rgba(1.0, 1.0, 1.0, 0.65)
                font.pixelSize: Math.max(12, Math.round(sliderRoot.height * 0.35))
                font.bold: true
            }

            // 右侧终点指示图标
            Image {
                anchors.right: parent.right
                anchors.rightMargin: parent.height * 0.22
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(parent.height * 0.38)
                height: width
                source: sliderRoot.isAlert ? "qrc:/icons/power.svg" : "qrc:/icons/arrow-right.svg"
                sourceSize: Qt.size(width, height)
                opacity: sliderRoot.isAlert ? 0.95 : 0.40
            }
        }
    }

    // 捕获内容层作为 OpenGL 纹理
    ShaderEffectSource {
        id: contentSource
        sourceItem: contentLayer
        recursive: false
        live: sliderRoot.visible
        visible: false
    }

    // ============================================================
    // 2. 核心数学着色器：Metaball smin 融球流体、按压放大、凸透镜光学放大
    // ============================================================
    ShaderEffect {
        id: fluidShader
        anchors.fill: parent

        property variant source: contentSource
        property real progress: sliderRoot.progress
        property real stretchFactor: sliderRoot.stretchFactor
        property real scaleFactor: sliderRoot.scaleFactor
        property real alertFactor: sliderRoot.alertFactor
        property point resolution: Qt.point(Math.max(sliderRoot.width, 1), Math.max(sliderRoot.height, 1))

        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform lowp float qt_Opacity;
            uniform sampler2D source;

            uniform highp float progress;
            uniform highp float stretchFactor;
            uniform highp float scaleFactor;
            uniform highp float alertFactor;
            uniform highp vec2 resolution;

            // Inigo Quilez 经典多项式平滑最小值 (smin) —— 实现水银/水滴表面张力粘连
            highp float smin(highp float a, highp float b, highp float k) {
                highp float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
                return mix(b, a, h) - k * h * (1.0 - h);
            }

            highp float sdRoundedBox(highp vec2 p, highp vec2 b, highp float r) {
                highp vec2 q = abs(p) - b + vec2(r);
                return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
            }

            void main() {
                highp vec2 uv = qt_TexCoord0;
                highp vec2 p = (uv - vec2(0.5)) * resolution;
                highp vec2 halfSize = (resolution * 0.5) - vec2(1.0);
                highp float pad = 3.0;
                highp float trackR = halfSize.y - pad;

                // 1. 跑道边缘 SDF
                highp float dTrack = sdRoundedBox(p, halfSize - vec2(pad), trackR);
                highp float covTrack = clamp(0.5 - dTrack / 1.5, 0.0, 1.0);
                if (covTrack <= 0.002) {
                    gl_FragColor = vec4(0.0);
                    return;
                }

                // 2. 跑道两端坐标与饱满圆润水滴半径
                // 静止时宽厚饱满 (0.80)，按住滑动时微弹放大，触控面积与视觉分量更佳
                highp float baseKnobR = trackR * 0.80;
                highp float curKnobR  = baseKnobR * scaleFactor;
                highp float xMin = -halfSize.x + pad + trackR;
                highp float xMax =  halfSize.x - pad - trackR;

                highp vec2 p0 = vec2(xMin, 0.0);
                highp vec2 p1 = vec2(mix(xMin, xMax, progress), 0.0);

                // 3. 原点母水滴与滑动水滴
                highp float r0 = baseKnobR * clamp(0.70 - progress * 2.2, 0.0, 0.70);
                highp float d0 = length(p - p0) - r0;

                // 滑动水滴受力流线型拉长 (横向拉伸时纵向自然微收，保持流体体积守恒)
                highp vec2 p1_local = p - p1;
                p1_local.x /= (1.0 + stretchFactor * 0.22);
                p1_local.y *= (1.0 + stretchFactor * 0.12);
                highp float d1 = length(p1_local) - curKnobR;

                // 4. 表面张力流体融球 (Metaball Liquid Bridge)
                // blendK 随着圆球进一步调宽适配至 18，形成更具张力的流体拔丝水带
                highp float bridgeStrength = clamp(1.0 - smoothstep(0.08, 0.28, progress), 0.0, 1.0);
                highp float blendK = 18.0 * bridgeStrength;
                highp float dFluid = (blendK > 0.5 && r0 > 0.8) ? smin(d0, d1, blendK) : d1;

                // 5. 梯度法线场计算 (Gradient Normal Field)
                highp float eps = 1.0;
                highp vec2 p_px = p + vec2(eps, 0.0);
                highp vec2 p_mx = p - vec2(eps, 0.0);
                highp vec2 p_py = p + vec2(0.0, eps);
                highp vec2 p_my = p - vec2(0.0, eps);

                highp vec2 p1_px = p_px - p1; p1_px.x /= (1.0 + stretchFactor * 0.22); p1_px.y *= (1.0 + stretchFactor * 0.12);
                highp vec2 p1_mx = p_mx - p1; p1_mx.x /= (1.0 + stretchFactor * 0.22); p1_mx.y *= (1.0 + stretchFactor * 0.12);
                highp vec2 p1_py = p_py - p1; p1_py.x /= (1.0 + stretchFactor * 0.22); p1_py.y *= (1.0 + stretchFactor * 0.12);
                highp vec2 p1_my = p_my - p1; p1_my.x /= (1.0 + stretchFactor * 0.22); p1_my.y *= (1.0 + stretchFactor * 0.12);

                highp float dF_px = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_px - p0) - r0, length(p1_px) - curKnobR, blendK) : (length(p1_px) - curKnobR);
                highp float dF_mx = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_mx - p0) - r0, length(p1_mx) - curKnobR, blendK) : (length(p1_mx) - curKnobR);
                highp float dF_py = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_py - p0) - r0, length(p1_py) - curKnobR, blendK) : (length(p1_py) - curKnobR);
                highp float dF_my = (blendK > 0.5 && r0 > 0.8) ? smin(length(p_my - p0) - r0, length(p1_my) - curKnobR, blendK) : (length(p1_my) - curKnobR);

                highp vec2 n = normalize(vec2(dF_px - dF_mx, dF_py - dF_my) + vec2(0.0001));

                // 6. 凸透镜物理放大与折射计算 (Spherical Convex Magnifying Lens)
                // 在滑动水滴内部，光线向中心会聚，将背后的文字以凸透镜放大约 1.28x
                highp vec2 totalOffset = vec2(0.0);
                highp float distToCenter = length(p - p1);
                highp float normDist = distToCenter / max(curKnobR, 1.0);

                if (normDist < 1.0) {
                    highp float lensShape = sqrt(max(0.0, 1.0 - normDist * normDist));
                    highp float mag = 0.28 * pow(lensShape, 0.85);
                    highp vec2 magnifyOffset = -(p - p1) * mag;
                    highp vec2 edgeRefract = n * (-4.5 * (1.0 - lensShape));
                    totalOffset = magnifyOffset + edgeRefract;
                } else if (dFluid < 0.0) {
                    highp float bridgeSlope = clamp(1.0 - (-dFluid) / 4.0, 0.0, 1.0);
                    totalOffset = n * (-5.0 * bridgeSlope);
                }

                // 7. 色散 (Chromatic Aberration) 真实光学采样
                highp vec2 uvR = clamp(uv + (totalOffset * 1.02) / resolution, 0.0, 1.0);
                highp vec2 uvG = clamp(uv + totalOffset / resolution, 0.0, 1.0);
                highp vec2 uvB = clamp(uv + (totalOffset * 0.98) / resolution, 0.0, 1.0);

                highp vec4 baseCol = texture2D(source, uv);
                highp vec3 refrCol = vec3(
                    texture2D(source, uvR).r,
                    texture2D(source, uvG).g,
                    texture2D(source, uvB).b
                );

                // 8. 苹果双对称高光瓣（沿水滴及融球液桥边缘包裹流动）
                highp vec2 lightDir = normalize(vec2(-0.55, -0.83));
                highp float facing = dot(n, -lightDir);
                highp float lobeF = pow(max(facing, 0.0), 4.5);
                highp float lobeB = pow(max(-facing, 0.0), 3.5) * 0.65;

                highp float hair = clamp(1.0 - abs(dFluid + 0.8) / 1.2, 0.0, 1.0);
                highp float specRim = hair * (lobeF + lobeB) * 0.98;

                // 水滴表面穹顶光泽与中心高光
                highp float domeSheen = smoothstep(-0.2, 0.9, -n.y) * clamp(-dFluid / curKnobR, 0.0, 1.0) * 0.25;
                highp float centerSpot = pow(clamp(1.0 - normDist, 0.0, 1.0), 3.0) * 0.15;

                // 9. 颜色调和与警报红宝石流体
                highp vec3 normalGlassTint = vec3(0.95, 0.97, 1.0);
                highp vec3 alertGlassTint  = vec3(1.0, 0.24, 0.20);
                highp vec3 currentTint = mix(normalGlassTint, alertGlassTint, alertFactor);

                highp vec3 waterDropColor = mix(refrCol, currentTint, mix(0.14, 0.60, alertFactor));
                waterDropColor += vec3(1.0) * (specRim + domeSheen + centerSpot);

                // 10. 最终覆盖合成
                highp float covFluid = clamp(0.5 - dFluid / 1.5, 0.0, 1.0);
                highp float fluidAlpha = mix(0.40, 0.72, alertFactor) * covFluid;

                highp vec3 finalRGB = mix(baseCol.rgb, waterDropColor, covFluid);
                highp float finalAlpha = max(baseCol.a, fluidAlpha) * covTrack;

                gl_FragColor = vec4(finalRGB * finalAlpha, finalAlpha) * qt_Opacity;
            }
        "
    }

    // ============================================================
    // 3. 独立手势控制层 (Touch & Drag Gesture Handler)
    // ============================================================
    MouseArea {
        id: dragArea
        anchors.fill: parent
        enabled: sliderRoot.interactive
        hoverEnabled: sliderRoot.interactive
        preventStealing: sliderRoot.isDragging

        property real startX: 0
        property real startProgress: 0

        onPressed: {
            startX = mouse.x
            startProgress = sliderRoot.progress
            springResetAnim.stop()
            sliderRoot.isDragging = true
        }

        onPositionChanged: {
            if (!sliderRoot.isDragging) return
            var dx = mouse.x - startX
            var travelDist = Math.max(1, sliderRoot.width - sliderRoot.height)
            var curP = startProgress + dx / travelDist
            sliderRoot.progress = Math.max(0.0, Math.min(1.0, curP))
        }

        onReleased: {
            sliderRoot.isDragging = false
            if (sliderRoot.progress >= 0.60) {
                sliderRoot.progress = 1.0
                sliderRoot.triggered()
            } else {
                sliderRoot.reset()
                sliderRoot.canceled()
            }
        }

        onCanceled: {
            sliderRoot.isDragging = false
            sliderRoot.reset()
            sliderRoot.canceled()
        }
    }
}
