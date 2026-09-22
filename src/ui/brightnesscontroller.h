#ifndef BRIGHTNESSCONTROLLER_H
#define BRIGHTNESSCONTROLLER_H

#include <QString>

class BrightnessController
{
public:
    bool initialize();
    qreal brightness() const;
    bool hardwareAvailable() const;
    QString brightnessPath() const;
    bool setBrightness(qreal brightness);
    QString detectPanelType() const;

private:
    static qreal clampBrightness(qreal brightness);
    static QString firstLine(const QString &value);

    QString m_backlightBrightnessPath;
    int m_backlightMaxBrightness = 0;
    qreal m_brightness = 1.0;
    bool m_hardwareBrightnessAvailable = false;
};

#endif
