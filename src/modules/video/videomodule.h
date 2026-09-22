#ifndef VIDEOMODULE_H
#define VIDEOMODULE_H

#include <QThread>

#include "core/imodule.h"

class OnvifClient;

class VideoWorker : public QObject
{
    Q_OBJECT

public:
    enum VideoStatus {
        Idle,
        Connecting,
        Detecting,
        Playing,
        ErrorAuth,
        ErrorAddress,
        ErrorTimeout,
        ErrorNetwork,
        ErrorUnknown
    };
    Q_ENUM(VideoStatus)

public slots:
    // 初始化视频模块，必要时做 ONVIF 探测并产出最终流地址。
    void initialize(const VideoConfig &config);
    void shutdown();
    void abort();
    // PTZ 方向控制和停止命令都从这里下发。
    void movePtz(const QString &direction);

signals:
    void streamUriChanged(const QString &streamUri);
    void onvifAvailabilityChanged(bool available);
    void onvifProfileSwitchSupportedChanged(bool supported);
    void onvifCurrentProfileChanged(const QString &profile);
    void ptzStatusChanged(const QString &status);
    void ptzAvailabilityChanged(bool available);

private:
    void resetUnavailable(const QString &ptzStatus);

    VideoConfig m_config;
    QString m_profileToken;
    QString m_ptzServiceUrl;
    class OnvifClient *m_onvifClient = nullptr;
};

class VideoModule : public IModule
{
    Q_OBJECT

public:
    // 对外暴露的视频模块壳，真正耗时逻辑在 VideoWorker 线程里执行。
    explicit VideoModule(GlobalState *globalState, QObject *parent = nullptr);
    ~VideoModule() override;

    QString name() const override;
    void applyConfig(const AppConfig &config) override;
    void start() override;
    void stop() override;
    // 这两个接口由 AppController 调用，再转发给工作线程。
    void movePtz(const QString &direction);

private:
    VideoConfig m_config;
    QThread m_workerThread;
    VideoWorker *m_worker;
};

#endif
