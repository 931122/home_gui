# LiquidGlass QML Module

The module exposes reusable Qt Quick controls as `HomeGui.LiquidGlass 1.0`.

```qml
import QtQuick
import HomeGui.LiquidGlass 1.0

LiquidGlassSurface {
    width: 240
    height: 64
    cornerRadius: 32
}
```

The CMake target is `home_gui_liquidglass`; link it to a Qt Quick application to
embed the QML types and shader resources. Applications should create a
`GlassRuntime` and expose it as the `glassRuntime` context property to enable
backdrop capture, device tilt, and accessibility state. Components can still
render their fallback material without that property.

The qmake project embeds the same module through `resources/liquidglass.qrc`.
