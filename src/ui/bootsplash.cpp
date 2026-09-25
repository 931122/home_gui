#include "bootsplash.h"
#include "core/platformhelper.h"

#include <QBackingStore>
#include <QColor>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QWindow>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

class BootSplash::Impl
{
public:
    explicit Impl(const PlatformConfig &config)
        : backingStore(&window)
        , isDesktop(PlatformHelper::isDesktopEnvironment())
    {
        const QSize configSize(qMax(320, config.width), qMax(240, config.height));

        if (isDesktop) {
            size = configSize;
            window.setTitle(QStringLiteral("home_gui boot"));
            window.setFlags(Qt::SplashScreen | Qt::FramelessWindowHint);
            const QRect avail = QGuiApplication::primaryScreen()
                    ? QGuiApplication::primaryScreen()->availableGeometry()
                    : QRect(QPoint(0, 0), configSize);
            const int x = avail.x() + qMax(0, (avail.width() - size.width()) / 2);
            const int y = avail.y() + qMax(0, (avail.height() - size.height()) / 2);
            window.setGeometry(QRect(QPoint(x, y), size));
        } else {
            const QRect screenGeometry = QGuiApplication::primaryScreen()
                    ? QGuiApplication::primaryScreen()->geometry()
                    : QRect(QPoint(0, 0), configSize);
            size = screenGeometry.size().isValid() ? screenGeometry.size() : configSize;

            window.setTitle(QStringLiteral("home_gui boot"));
            window.setFlags(Qt::FramelessWindowHint);
            window.setGeometry(QRect(screenGeometry.topLeft(), size));
        }

        showAndFlush(80);
        visibleTimer.start();
    }

    void close()
    {
        window.close();
    }

    void raise()
    {
        showAndFlush(30);
    }

    bool minimumVisibleElapsed(int milliseconds) const
    {
        return visibleTimer.isValid() && visibleTimer.elapsed() >= milliseconds;
    }

private:
    void showAndFlush(int eventBudgetMs)
    {
        if (isDesktop) {
            window.show();
        } else {
            window.showFullScreen();
        }
        window.raise();
        window.requestActivate();
        render();
        QGuiApplication::processEvents(QEventLoop::AllEvents, eventBudgetMs);
    }

    void render()
    {
        if (!size.isValid()) {
            return;
        }

        backingStore.resize(size);
        backingStore.beginPaint(QRegion(QRect(QPoint(0, 0), size)));

        QPainter painter(backingStore.paintDevice());
        painter.setRenderHint(QPainter::Antialiasing, true);

        QLinearGradient background(0, 0, size.width(), size.height());
        background.setColorAt(0.0, QColor(QStringLiteral("#123044")));
        background.setColorAt(0.52, QColor(QStringLiteral("#081018")));
        background.setColorAt(1.0, QColor(QStringLiteral("#030507")));
        painter.fillRect(QRect(QPoint(0, 0), size), background);

        const qreal scale = qMin(size.width() / 800.0, size.height() / 480.0);
        const int logoSize = qMax(72, qRound(118 * scale));
        const QPoint center(size.width() / 2, size.height() / 2 - qRound(28 * scale));
        const QRectF logoRect(center.x() - logoSize / 2.0, center.y() - logoSize / 2.0, logoSize, logoSize);

        QRadialGradient glow(logoRect.center(), logoSize * 1.8);
        glow.setColorAt(0.0, QColor(76, 184, 210, 110));
        glow.setColorAt(1.0, QColor(76, 184, 210, 0));
        painter.setBrush(glow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(logoRect.adjusted(-logoSize * 0.62, -logoSize * 0.62,
                                              logoSize * 0.62, logoSize * 0.62));

        painter.setBrush(QColor(QStringLiteral("#102433")));
        painter.setPen(QPen(QColor(QStringLiteral("#5bc4d8")), qMax(2, qRound(2 * scale))));
        painter.drawRoundedRect(logoRect, logoSize * 0.24, logoSize * 0.24);

        painter.setPen(QPen(QColor(QStringLiteral("#f2fbff")), qMax(5, qRound(7 * scale)),
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF a(logoRect.left() + logoSize * 0.28, logoRect.top() + logoSize * 0.62);
        const QPointF b(logoRect.left() + logoSize * 0.45, logoRect.top() + logoSize * 0.38);
        const QPointF c(logoRect.left() + logoSize * 0.64, logoRect.top() + logoSize * 0.62);
        const QPointF d(logoRect.left() + logoSize * 0.78, logoRect.top() + logoSize * 0.42);
        painter.drawLine(a, b);
        painter.drawLine(b, c);
        painter.drawLine(c, d);

        QFont titleFont = QGuiApplication::font();
        titleFont.setPixelSize(qMax(28, qRound(38 * scale)));
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(QColor(QStringLiteral("#eef8fb")));
        painter.drawText(QRect(0, center.y() + logoSize / 2 + qRound(22 * scale),
                               size.width(), qRound(48 * scale)),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         QStringLiteral("HOME GUI"));

        QFont subtitleFont = QGuiApplication::font();
        subtitleFont.setPixelSize(qMax(14, qRound(17 * scale)));
        painter.setFont(subtitleFont);
        painter.setPen(QColor(QStringLiteral("#8aa4b3")));
        painter.drawText(QRect(0, center.y() + logoSize / 2 + qRound(68 * scale),
                               size.width(), qRound(28 * scale)),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         QStringLiteral("正在启动智能中控"));

        painter.end();
        backingStore.endPaint();
        backingStore.flush(QRegion(QRect(QPoint(0, 0), size)), &window);
    }

    QWindow window;
    QBackingStore backingStore;
    QSize size;
    QElapsedTimer visibleTimer;
    bool isDesktop;
};

BootSplash::BootSplash(const PlatformConfig &config)
    : m_impl(new Impl(config))
{
}

BootSplash::~BootSplash() = default;

void BootSplash::close()
{
    if (m_impl) {
        m_impl->close();
    }
}

void BootSplash::raise()
{
    if (m_impl) {
        m_impl->raise();
    }
}

bool BootSplash::minimumVisibleElapsed(int milliseconds) const
{
    return m_impl && m_impl->minimumVisibleElapsed(milliseconds);
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
namespace {

bool earlyFramebufferSplashEnabled(const PlatformConfig &config)
{
    if (PlatformHelper::isDesktopEnvironment()) {
        return false;
    }

    const QString renderMode = config.renderMode.trimmed().toLower();
    return renderMode == QStringLiteral("linuxfb");
}

void writePixel(uchar *framebuffer,
                const fb_var_screeninfo &varInfo,
                const fb_fix_screeninfo &fixInfo,
                int x,
                int y,
                int red,
                int green,
                int blue)
{
    if (x < 0 || y < 0 || x >= static_cast<int>(varInfo.xres) || y >= static_cast<int>(varInfo.yres)) {
        return;
    }

    uchar *pixel = framebuffer + y * fixInfo.line_length + x * (varInfo.bits_per_pixel / 8);
    if (varInfo.bits_per_pixel == 32) {
        pixel[0] = static_cast<uchar>(blue);
        pixel[1] = static_cast<uchar>(green);
        pixel[2] = static_cast<uchar>(red);
        pixel[3] = 0xff;
    } else if (varInfo.bits_per_pixel == 24) {
        pixel[0] = static_cast<uchar>(blue);
        pixel[1] = static_cast<uchar>(green);
        pixel[2] = static_cast<uchar>(red);
    } else if (varInfo.bits_per_pixel == 16) {
        const quint16 value = static_cast<quint16>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
        pixel[0] = static_cast<uchar>(value & 0xff);
        pixel[1] = static_cast<uchar>((value >> 8) & 0xff);
    }
}

void fillRect(uchar *framebuffer,
              const fb_var_screeninfo &varInfo,
              const fb_fix_screeninfo &fixInfo,
              int left,
              int top,
              int width,
              int height,
              int red,
              int green,
              int blue)
{
    const int right = qMin<int>(left + width, varInfo.xres);
    const int bottom = qMin<int>(top + height, varInfo.yres);
    for (int y = qMax(0, top); y < bottom; ++y) {
        for (int x = qMax(0, left); x < right; ++x) {
            writePixel(framebuffer, varInfo, fixInfo, x, y, red, green, blue);
        }
    }
}

} // namespace
#endif

void drawEarlyFramebufferSplash(const PlatformConfig &config)
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (!earlyFramebufferSplashEnabled(config)) {
        return;
    }

    QFile blankFile(QStringLiteral("/sys/class/graphics/fb0/blank"));
    if (blankFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        blankFile.write("0\n");
        blankFile.flush();
    }

    const int framebufferFd = ::open("/dev/fb0", O_RDWR);
    if (framebufferFd < 0) {
        return;
    }

    fb_var_screeninfo varInfo;
    fb_fix_screeninfo fixInfo;
    if (::ioctl(framebufferFd, FBIOGET_VSCREENINFO, &varInfo) != 0
        || ::ioctl(framebufferFd, FBIOGET_FSCREENINFO, &fixInfo) != 0
        || (varInfo.bits_per_pixel != 16 && varInfo.bits_per_pixel != 24 && varInfo.bits_per_pixel != 32)) {
        ::close(framebufferFd);
        return;
    }

    const size_t mapSize = static_cast<size_t>(fixInfo.line_length) * varInfo.yres;
    uchar *framebuffer = static_cast<uchar *>(::mmap(nullptr, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, framebufferFd, 0));
    if (framebuffer == MAP_FAILED) {
        ::close(framebufferFd);
        return;
    }

    const int width = qMin<int>(varInfo.xres, qMax(320, config.width));
    const int height = qMin<int>(varInfo.yres, qMax(240, config.height));
    for (int y = 0; y < height; ++y) {
        const int red = 5 + (18 * y / qMax(1, height));
        const int green = 9 + (38 * (height - y) / qMax(1, height));
        const int blue = 13 + (52 * (height - y) / qMax(1, height));
        for (int x = 0; x < width; ++x) {
            writePixel(framebuffer, varInfo, fixInfo, x, y, red, green, blue);
        }
    }

    const int scale = qMax(1, qMin(width / 800, height / 480));
    const int logoSize = qMax(70, 112 * scale);
    const int logoLeft = width / 2 - logoSize / 2;
    const int logoTop = height / 2 - logoSize / 2 - 24 * scale;
    fillRect(framebuffer, varInfo, fixInfo, logoLeft, logoTop, logoSize, logoSize, 14, 36, 50);
    fillRect(framebuffer, varInfo, fixInfo, logoLeft + 8 * scale, logoTop + 8 * scale,
             logoSize - 16 * scale, logoSize - 16 * scale, 18, 52, 70);

    const int line = qMax(4, 7 * scale);
    fillRect(framebuffer, varInfo, fixInfo, logoLeft + 28 * scale, logoTop + 62 * scale,
             22 * scale, line, 238, 248, 251);
    fillRect(framebuffer, varInfo, fixInfo, logoLeft + 42 * scale, logoTop + 44 * scale,
             line, 28 * scale, 238, 248, 251);
    fillRect(framebuffer, varInfo, fixInfo, logoLeft + 58 * scale, logoTop + 62 * scale,
             24 * scale, line, 238, 248, 251);
    fillRect(framebuffer, varInfo, fixInfo, logoLeft + 76 * scale, logoTop + 44 * scale,
             line, 28 * scale, 238, 248, 251);

    ::msync(framebuffer, mapSize, MS_SYNC);
    ::munmap(framebuffer, mapSize);
    ::close(framebufferFd);
#else
    Q_UNUSED(config)
#endif
}
