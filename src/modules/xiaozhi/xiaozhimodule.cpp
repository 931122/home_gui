#include "xiaozhimodule.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSslError>
#include <QUrl>
#include <QUuid>

#include "core/globalstate.h"

namespace {

constexpr quint16 AudioPortUp = 5676;
constexpr quint16 AudioPortDown = 5677;
constexpr quint16 UiPortUp = 5678;
constexpr quint16 UiPortDown = 5679;

QString normalizedWebSocketPath(QString path)
{
    path = path.trimmed();
    if (path.isEmpty()) {
        return QStringLiteral("/xiaozhi/v1/");
    }
    if (!path.startsWith(QLatin1Char('/'))) {
        path.prepend(QLatin1Char('/'));
    }
    return path;
}

QByteArray compactJson(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

} // namespace

void XiaozhiWorker::initialize(const XiaozhiConfig &config)
{
    m_config = config;
    if (m_networkAccessManager == nullptr) {
        m_networkAccessManager = new QNetworkAccessManager(this);
    }
    if (m_activationRetryTimer == nullptr) {
        m_activationRetryTimer = new QTimer(this);
        m_activationRetryTimer->setSingleShot(true);
        connect(m_activationRetryTimer, &QTimer::timeout, this, &XiaozhiWorker::activateDevice);
    }
    if (m_reconnectTimer == nullptr) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, &XiaozhiWorker::connectWebSocket);
    }
    if (m_overlayHideTimer == nullptr) {
        m_overlayHideTimer = new QTimer(this);
        m_overlayHideTimer->setSingleShot(true);
        connect(m_overlayHideTimer, &QTimer::timeout, this, [this]() {
            setOverlayVisible(false);
        });
    }
    if (m_gpioPollTimer == nullptr) {
        m_gpioPollTimer = new QTimer(this);
        connect(m_gpioPollTimer, &QTimer::timeout, this, &XiaozhiWorker::handleGpioPoll);
    }

    shutdown();
    m_config = config;
    m_shuttingDown = false;
    if (!m_config.enabled) {
        emit availableChanged(false);
        emit statusChanged(tr("Xiaozhi disabled"));
        return;
    }

    emit availableChanged(true);
    setState(StateStarting);
    emit statusChanged(tr("Xiaozhi starting"));
    ensureUuid();

    if (!configureSockets()) {
        setState(StateFatalError);
        emit statusChanged(tr("Xiaozhi UDP bind failed"));
        return;
    }

    if (m_config.autoStartAudio) {
        startAudioProcess();
    }
    prepareGpio();
    activateDevice();
}

void XiaozhiWorker::shutdown()
{
    m_shuttingDown = true;
    if (m_activationRetryTimer != nullptr) {
        m_activationRetryTimer->stop();
    }
    if (m_reconnectTimer != nullptr) {
        m_reconnectTimer->stop();
    }
    if (m_overlayHideTimer != nullptr) {
        m_overlayHideTimer->stop();
    }
    if (m_gpioPollTimer != nullptr) {
        m_gpioPollTimer->stop();
    }
    if (m_activationReply != nullptr) {
        disconnect(m_activationReply, nullptr, this, nullptr);
        m_activationReply->abort();
        m_activationReply->deleteLater();
        m_activationReply = nullptr;
    }
    if (m_webSocket != nullptr) {
        disconnect(m_webSocket, nullptr, this, nullptr);
        m_webSocket->close();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    stopAudioProcess();
    closeSockets();
    releaseGpio();
    m_sessionId.clear();
    m_audioUploadEnabled = true;
    m_pendingWake = false;
    m_reconnectDelayMs = 3000;
    setOverlayVisible(false);
    emit availableChanged(false);
    emit statusChanged(tr("Xiaozhi stopped"));
    m_shuttingDown = false;
}

void XiaozhiWorker::wake()
{
    if (!m_config.enabled) {
        return;
    }

    setOverlayVisible(true);
    setState(StateListening);
    m_audioUploadEnabled = true;
    sendUiText(tr("我在听"));
    if (m_webSocket != nullptr && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        sendStartListening(QStringLiteral("auto"));
    } else if (m_reconnectTimer != nullptr && !m_reconnectTimer->isActive()) {
        m_pendingWake = true;
        connectWebSocket();
    }
}

void XiaozhiWorker::hideOverlay()
{
    setOverlayVisible(false);
}

bool XiaozhiWorker::configureSockets()
{
    closeSockets();

    m_audioSocket = new QUdpSocket(this);
    if (!m_audioSocket->bind(QHostAddress::LocalHost, AudioPortUp, QUdpSocket::ShareAddress)) {
        delete m_audioSocket;
        m_audioSocket = nullptr;
        return false;
    }
    connect(m_audioSocket, &QUdpSocket::readyRead, this, &XiaozhiWorker::handleAudioDatagrams);

    m_uiSocket = new QUdpSocket(this);
    if (m_uiSocket->bind(QHostAddress::LocalHost, UiPortDown, QUdpSocket::ShareAddress)) {
        connect(m_uiSocket, &QUdpSocket::readyRead, this, &XiaozhiWorker::handleUiDatagrams);
    } else {
        delete m_uiSocket;
        m_uiSocket = nullptr;
    }

    return true;
}

void XiaozhiWorker::closeSockets()
{
    if (m_audioSocket != nullptr) {
        m_audioSocket->close();
        m_audioSocket->deleteLater();
        m_audioSocket = nullptr;
    }
    if (m_uiSocket != nullptr) {
        m_uiSocket->close();
        m_uiSocket->deleteLater();
        m_uiSocket = nullptr;
    }
}

void XiaozhiWorker::startAudioProcess()
{
    if (m_audioProcess != nullptr || m_config.soundAppPath.isEmpty()) {
        return;
    }

    QString program = m_config.soundAppPath;
    if (!QFileInfo(program).isAbsolute()) {
        const QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(program);
        if (QFileInfo::exists(bundled)) {
            program = bundled;
        }
    }

    m_audioProcess = new QProcess(this);
    connect(m_audioProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        emit statusChanged(tr("Xiaozhi audio exited: %1").arg(exitCode));
        if (m_audioProcess != nullptr) {
            m_audioProcess->deleteLater();
            m_audioProcess = nullptr;
        }
        if (!m_shuttingDown && m_config.enabled && m_config.autoStartAudio) {
            QTimer::singleShot(3000, this, &XiaozhiWorker::startAudioProcess);
        }
    });
    connect(m_audioProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        Q_UNUSED(error)
        emit statusChanged(tr("Xiaozhi audio start failed"));
    });
    m_audioProcess->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    m_audioProcess->start(program, QStringList());
}

void XiaozhiWorker::stopAudioProcess()
{
    if (m_audioProcess == nullptr) {
        return;
    }

    QProcess *process = m_audioProcess;
    m_audioProcess = nullptr;
    process->terminate();
    if (!process->waitForFinished(1500)) {
        process->kill();
        process->waitForFinished(1000);
    }
    disconnect(process, nullptr, this, nullptr);
    process->deleteLater();
}

void XiaozhiWorker::ensureUuid()
{
    m_uuid = readUuid();
    if (m_uuid.isEmpty()) {
        m_uuid = generateUuid();
        writeUuid(m_uuid);
    }
}

QString XiaozhiWorker::readUuid() const
{
    QFile file(m_config.configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return QString();
    }
    return document.object().value(QStringLiteral("uuid")).toString().trimmed();
}

bool XiaozhiWorker::writeUuid(const QString &uuid) const
{
    const QFileInfo info(m_config.configPath);
    QDir().mkpath(info.absolutePath());

    QFile file(m_config.configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonObject object;
    object.insert(QStringLiteral("uuid"), uuid);
    return file.write(QJsonDocument(object).toJson(QJsonDocument::Indented)) > 0;
}

QString XiaozhiWorker::generateUuid() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString XiaozhiWorker::deviceId() const
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (!(interface.flags() & QNetworkInterface::IsUp)
            || (interface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        const QString hardwareAddress = interface.hardwareAddress().trimmed();
        if (!hardwareAddress.isEmpty()) {
            return hardwareAddress.toLower();
        }
    }
    return QStringLiteral("00:00:00:00:00:00");
}

void XiaozhiWorker::activateDevice()
{
    if (!m_config.enabled || m_networkAccessManager == nullptr) {
        return;
    }
    if (m_activationReply != nullptr) {
        return;
    }

    QNetworkRequest request(QUrl(m_config.otaUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Device-Id", deviceId().toUtf8());
    request.setRawHeader("User-Agent", m_config.userAgent.toUtf8());
    request.setRawHeader("Accept-Language", m_config.language.toUtf8());

    QJsonObject application;
    application.insert(QStringLiteral("name"), m_config.applicationName);
    application.insert(QStringLiteral("version"), m_config.applicationVersion);

    QJsonObject board;
    board.insert(QStringLiteral("type"), m_config.boardType);
    board.insert(QStringLiteral("name"), m_config.boardName);

    QJsonObject payload;
    payload.insert(QStringLiteral("uuid"), m_uuid);
    payload.insert(QStringLiteral("application"), application);
    payload.insert(QStringLiteral("ota"), QJsonObject());
    payload.insert(QStringLiteral("board"), board);

    setState(StateActivating);
    emit statusChanged(tr("Xiaozhi activating"));
    m_activationReply = m_networkAccessManager->post(request, compactJson(payload));
    connect(m_activationReply, &QNetworkReply::finished, this, &XiaozhiWorker::handleActivationReply);

    QPointer<QNetworkReply> replyWatcher(m_activationReply);
    QTimer::singleShot(10000, this, [this, replyWatcher]() {
        if (!m_shuttingDown && replyWatcher && replyWatcher == m_activationReply && replyWatcher->isRunning()) {
            qWarning() << "Xiaozhi activation request timed out after 10 seconds";
            replyWatcher->abort();
        }
    });
}

void XiaozhiWorker::handleActivationReply()
{
    QNetworkReply *reply = m_activationReply;
    m_activationReply = nullptr;
    if (reply == nullptr) {
        return;
    }

    const QByteArray payload = reply->readAll();
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (!networkOk) {
        emit statusChanged(tr("Xiaozhi activation failed: %1").arg(errorText));
        if (m_activationRetryTimer != nullptr) {
            m_activationRetryTimer->start(5000);
        }
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    const QJsonObject object = document.object();
    const QJsonObject activation = object.value(QStringLiteral("activation")).toObject();
    const QString code = activation.value(QStringLiteral("code")).toString().trimmed();
    if (!code.isEmpty()) {
        setState(StateActivating);
        sendUiText(tr("Active-Code: %1").arg(code));
        setOverlayVisible(true);
        if (m_activationRetryTimer != nullptr) {
            m_activationRetryTimer->start(5000);
        }
        return;
    }

    setState(StateIdle);
    sendUiText(tr("设备已经激活"));
    connectWebSocket();
}

void XiaozhiWorker::connectWebSocket()
{
    if (!m_config.enabled) {
        return;
    }

    if (m_webSocket != nullptr) {
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    const QUrl url(QStringLiteral("wss://%1:%2%3")
                   .arg(m_config.websocketHost)
                   .arg(m_config.websocketPort)
                   .arg(normalizedWebSocketPath(m_config.websocketPath)));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.authToken).toUtf8());
    request.setRawHeader("Protocol-Version", "1");
    request.setRawHeader("Device-Id", deviceId().toUtf8());
    request.setRawHeader("Client-Id", m_uuid.toUtf8());

    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        m_reconnectDelayMs = 3000;
        setState(StateConnecting);
        emit statusChanged(tr("Xiaozhi websocket connected"));
        sendHello();
    });
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &XiaozhiWorker::handleTextMessage);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived,
            this, &XiaozhiWorker::handleBinaryMessage);
    connect(m_webSocket,
            static_cast<void (QWebSocket::*)(const QList<QSslError> &)>(&QWebSocket::sslErrors),
            this,
            [this](const QList<QSslError> &errors) {
        QStringList errorMessages;
        for (const QSslError &err : errors) {
            errorMessages << err.errorString();
        }
        qWarning() << "Xiaozhi TLS verification failed:" << errorMessages.join(QStringLiteral("; "));
        emit statusChanged(tr("Xiaozhi TLS error: %1").arg(errorMessages.value(0)));
        if (m_webSocket != nullptr) {
            m_webSocket->close(QWebSocketProtocol::CloseCodePolicyViolated, QStringLiteral("TLS error"));
        }
    });
    connect(m_webSocket,
            &QWebSocket::errorOccurred,
            this,
            [this](QAbstractSocket::SocketError error) {
        Q_UNUSED(error)
        emit statusChanged(tr("Xiaozhi websocket error"));
    });
    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        emit statusChanged(tr("Xiaozhi websocket disconnected"));
        if (!m_shuttingDown && m_config.enabled && m_reconnectTimer != nullptr) {
            m_reconnectTimer->start(m_reconnectDelayMs);
            m_reconnectDelayMs = qMin(60000, m_reconnectDelayMs * 2);
        }
    });

    setState(StateConnecting);
    emit statusChanged(tr("Xiaozhi websocket connecting"));
    m_webSocket->open(request);
}

void XiaozhiWorker::sendHello()
{
    QJsonObject audioParams;
    audioParams.insert(QStringLiteral("format"), QStringLiteral("opus"));
    audioParams.insert(QStringLiteral("sample_rate"), 16000);
    audioParams.insert(QStringLiteral("channels"), 1);
    audioParams.insert(QStringLiteral("frame_duration"), 60);

    QJsonObject hello;
    hello.insert(QStringLiteral("type"), QStringLiteral("hello"));
    hello.insert(QStringLiteral("version"), 1);
    hello.insert(QStringLiteral("transport"), QStringLiteral("websocket"));
    hello.insert(QStringLiteral("audio_params"), audioParams);
    sendWebSocketText(hello);
}

void XiaozhiWorker::sendStartListening(const QString &mode)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("session_id"), m_sessionId);
    payload.insert(QStringLiteral("type"), QStringLiteral("listen"));
    payload.insert(QStringLiteral("state"), QStringLiteral("start"));
    payload.insert(QStringLiteral("mode"), mode);
    sendWebSocketText(payload);
}

void XiaozhiWorker::sendIotDescriptors()
{
    const auto descriptorPayload = [this](const QString &name,
                                          const QString &description,
                                          const QJsonObject &properties,
                                          const QJsonObject &methods) {
        QJsonObject descriptor;
        descriptor.insert(QStringLiteral("name"), name);
        descriptor.insert(QStringLiteral("description"), description);
        descriptor.insert(QStringLiteral("properties"), properties);
        descriptor.insert(QStringLiteral("methods"), methods);

        QJsonObject payload;
        payload.insert(QStringLiteral("session_id"), m_sessionId);
        payload.insert(QStringLiteral("type"), QStringLiteral("iot"));
        payload.insert(QStringLiteral("update"), true);
        QJsonArray descriptors;
        descriptors.append(descriptor);
        payload.insert(QStringLiteral("descriptors"), descriptors);
        sendWebSocketText(payload);
    };

    QJsonObject volumeProperty;
    volumeProperty.insert(QStringLiteral("description"), QStringLiteral("当前音量值"));
    volumeProperty.insert(QStringLiteral("type"), QStringLiteral("number"));
    QJsonObject volumeParameters;
    volumeParameters.insert(QStringLiteral("volume"), volumeProperty);
    QJsonObject setVolumeMethod;
    setVolumeMethod.insert(QStringLiteral("description"), QStringLiteral("设置音量"));
    setVolumeMethod.insert(QStringLiteral("parameters"), volumeParameters);
    descriptorPayload(QStringLiteral("Speaker"),
                      QStringLiteral("扬声器"),
                      QJsonObject{{QStringLiteral("volume"), volumeProperty}},
                      QJsonObject{{QStringLiteral("SetVolume"), setVolumeMethod}});

    QJsonObject brightnessProperty;
    brightnessProperty.insert(QStringLiteral("description"), QStringLiteral("当前亮度百分比"));
    brightnessProperty.insert(QStringLiteral("type"), QStringLiteral("number"));
    QJsonObject brightnessParameters;
    brightnessParameters.insert(QStringLiteral("brightness"), brightnessProperty);
    QJsonObject setBrightnessMethod;
    setBrightnessMethod.insert(QStringLiteral("description"), QStringLiteral("设置亮度"));
    setBrightnessMethod.insert(QStringLiteral("parameters"), brightnessParameters);
    descriptorPayload(QStringLiteral("Backlight"),
                      QStringLiteral("屏幕背光"),
                      QJsonObject{{QStringLiteral("brightness"), brightnessProperty}},
                      QJsonObject{{QStringLiteral("SetBrightness"), setBrightnessMethod}});

    QJsonObject levelProperty;
    levelProperty.insert(QStringLiteral("description"), QStringLiteral("当前电量百分比"));
    levelProperty.insert(QStringLiteral("type"), QStringLiteral("number"));
    QJsonObject chargingProperty;
    chargingProperty.insert(QStringLiteral("description"), QStringLiteral("是否充电中"));
    chargingProperty.insert(QStringLiteral("type"), QStringLiteral("boolean"));
    descriptorPayload(QStringLiteral("Battery"),
                      QStringLiteral("电池管理"),
                      QJsonObject{{QStringLiteral("level"), levelProperty},
                                  {QStringLiteral("charging"), chargingProperty}},
                      QJsonObject());
}

void XiaozhiWorker::sendIotState()
{
    QJsonObject speakerState;
    speakerState.insert(QStringLiteral("volume"), 80);
    QJsonObject speaker;
    speaker.insert(QStringLiteral("name"), QStringLiteral("Speaker"));
    speaker.insert(QStringLiteral("state"), speakerState);

    QJsonObject backlightState;
    backlightState.insert(QStringLiteral("brightness"), 75);
    QJsonObject backlight;
    backlight.insert(QStringLiteral("name"), QStringLiteral("Backlight"));
    backlight.insert(QStringLiteral("state"), backlightState);

    QJsonObject batteryState;
    batteryState.insert(QStringLiteral("level"), 0);
    batteryState.insert(QStringLiteral("charging"), false);
    QJsonObject battery;
    battery.insert(QStringLiteral("name"), QStringLiteral("Battery"));
    battery.insert(QStringLiteral("state"), batteryState);

    QJsonObject payload;
    payload.insert(QStringLiteral("session_id"), m_sessionId);
    payload.insert(QStringLiteral("type"), QStringLiteral("iot"));
    payload.insert(QStringLiteral("update"), true);
    QJsonArray states;
    states.append(speaker);
    states.append(backlight);
    states.append(battery);
    payload.insert(QStringLiteral("states"), states);
    sendWebSocketText(payload);
}

void XiaozhiWorker::sendWebSocketText(const QJsonObject &payload)
{
    if (m_webSocket == nullptr || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    m_webSocket->sendTextMessage(QString::fromUtf8(compactJson(payload)));
}

void XiaozhiWorker::handleAudioDatagrams()
{
    while (m_audioSocket != nullptr && m_audioSocket->hasPendingDatagrams()) {
        QByteArray payload;
        payload.resize(static_cast<int>(m_audioSocket->pendingDatagramSize()));
        m_audioSocket->readDatagram(payload.data(), payload.size());
        if (!m_audioUploadEnabled
            || m_webSocket == nullptr
            || m_webSocket->state() != QAbstractSocket::ConnectedState) {
            continue;
        }
        m_webSocket->sendBinaryMessage(payload);
    }
}

void XiaozhiWorker::handleUiDatagrams()
{
    while (m_uiSocket != nullptr && m_uiSocket->hasPendingDatagrams()) {
        QByteArray payload;
        payload.resize(static_cast<int>(m_uiSocket->pendingDatagramSize()));
        m_uiSocket->readDatagram(payload.data(), payload.size());

        const QJsonDocument document = QJsonDocument::fromJson(payload);
        if (!document.isObject()) {
            continue;
        }
        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("state"))) {
            setState(static_cast<DeviceState>(object.value(QStringLiteral("state")).toInt(StateUnknown)));
        }
        if (object.contains(QStringLiteral("text"))) {
            emit textChanged(object.value(QStringLiteral("text")).toString());
            setOverlayVisible(true);
        }
    }
}

void XiaozhiWorker::handleTextMessage(const QString &message)
{
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    if (!document.isObject()) {
        return;
    }
    processServerJson(document.object());
}

void XiaozhiWorker::handleBinaryMessage(const QByteArray &message)
{
    if (m_audioSocket == nullptr || message.isEmpty()) {
        return;
    }
    m_audioSocket->writeDatagram(message, QHostAddress::LocalHost, AudioPortDown);
}

void XiaozhiWorker::processServerJson(const QJsonObject &object)
{
    const QString type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("hello")) {
        m_sessionId = object.value(QStringLiteral("session_id")).toString();
        sendIotDescriptors();
        sendIotState();
        if (m_config.autoListenOnConnect || m_pendingWake) {
            m_pendingWake = false;
            sendStartListening(QStringLiteral("auto"));
            m_audioUploadEnabled = true;
            setState(StateListening);
        } else {
            m_audioUploadEnabled = false;
            setState(StateIdle);
        }
        return;
    }

    if (type == QStringLiteral("tts")) {
        const QString state = object.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("start")) {
            m_audioUploadEnabled = false;
            setState(StateListening);
        } else if (state == QStringLiteral("stop")) {
            QTimer::singleShot(2000, this, [this]() {
                sendStartListening(QStringLiteral("auto"));
                m_audioUploadEnabled = true;
                setState(StateListening);
            });
        } else if (state == QStringLiteral("sentence_start")) {
            const QString text = object.value(QStringLiteral("text")).toString();
            if (!text.isEmpty()) {
                sendUiText(text);
            }
            setState(StateSpeaking);
        }
        return;
    }

    if (type == QStringLiteral("stt")) {
        const QString text = object.value(QStringLiteral("text")).toString();
        if (!text.isEmpty()) {
            sendUiText(text);
        }
        setState(StateListening);
        return;
    }

    if (type == QStringLiteral("llm")) {
        const QString emotion = object.value(QStringLiteral("emotion")).toString();
        if (!emotion.isEmpty()) {
            emit statusChanged(tr("Xiaozhi emotion: %1").arg(emotion));
        }
    }
}

void XiaozhiWorker::sendUiState(DeviceState state)
{
    setState(state);
    if (m_uiSocket == nullptr) {
        return;
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("state"), static_cast<int>(state));
    m_uiSocket->writeDatagram(compactJson(payload), QHostAddress::LocalHost, UiPortDown);
}

void XiaozhiWorker::sendUiText(const QString &text)
{
    emit textChanged(text);
    setOverlayVisible(true);
    if (m_uiSocket == nullptr) {
        return;
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("text"), text);
    m_uiSocket->writeDatagram(compactJson(payload), QHostAddress::LocalHost, UiPortDown);
}

void XiaozhiWorker::setOverlayVisible(bool visible)
{
    if (m_overlayVisible == visible) {
        return;
    }
    m_overlayVisible = visible;
    emit overlayVisibleChanged(visible);
}

void XiaozhiWorker::setState(DeviceState state)
{
    const QString label = stateLabel(state);
    emit stateChanged(label);
    emit statusChanged(tr("Xiaozhi: %1").arg(label));
    if (state == StateListening || state == StateSpeaking || state == StateActivating) {
        setOverlayVisible(true);
    } else if (state == StateIdle && m_overlayHideTimer != nullptr) {
        m_overlayHideTimer->start(8000);
    }
}

QString XiaozhiWorker::stateLabel(DeviceState state) const
{
    switch (state) {
    case StateStarting:
        return tr("初始化");
    case StateWifiConfiguring:
        return tr("网络配置");
    case StateIdle:
        return tr("待命");
    case StateConnecting:
        return tr("连接中");
    case StateListening:
        return tr("聆听");
    case StateSpeaking:
        return tr("回答");
    case StateUpgrading:
        return tr("升级中");
    case StateActivating:
        return tr("激活中");
    case StateFatalError:
        return tr("错误");
    case StateUnknown:
    default:
        return tr("未知");
    }
}

bool XiaozhiWorker::prepareGpio()
{
    if (m_config.wakeGpioLine < 0) {
        return false;
    }

    const QString gpioNumber = QString::number(m_config.wakeGpioLine);
    const QString gpioDir = QStringLiteral("/sys/class/gpio/gpio%1").arg(gpioNumber);
    if (!QFileInfo::exists(gpioDir)) {
        writeTextFile(QStringLiteral("/sys/class/gpio/export"), gpioNumber);
    }
    writeTextFile(gpioDir + QStringLiteral("/direction"), QStringLiteral("in"));

    m_lastGpioValue = -1;
    handleGpioPoll();
    if (m_gpioPollTimer != nullptr) {
        m_gpioPollTimer->start(m_config.wakeGpioDebounceMs);
    }
    return true;
}

void XiaozhiWorker::releaseGpio()
{
    m_lastGpioValue = -1;
}

void XiaozhiWorker::handleGpioPoll()
{
    if (m_config.wakeGpioLine < 0) {
        return;
    }

    QFile file(QStringLiteral("/sys/class/gpio/gpio%1/value").arg(m_config.wakeGpioLine));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const int rawValue = QString::fromLatin1(file.readAll()).trimmed().toInt();
    const int activeValue = m_config.wakeGpioActiveLow ? 0 : 1;
    if (m_lastGpioValue >= 0 && rawValue == activeValue && m_lastGpioValue != rawValue) {
        wake();
    }
    m_lastGpioValue = rawValue;
}

bool XiaozhiWorker::writeTextFile(const QString &path, const QString &value) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(value.toLatin1()) == value.toLatin1().size();
}

XiaozhiModule::XiaozhiModule(GlobalState *globalState, QObject *parent)
    : IModule(globalState, parent)
    , m_worker(new XiaozhiWorker)
{
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &XiaozhiWorker::availableChanged, globalState, &GlobalState::setXiaozhiAvailable);
    connect(m_worker, &XiaozhiWorker::overlayVisibleChanged, globalState, &GlobalState::setXiaozhiOverlayVisible);
    connect(m_worker, &XiaozhiWorker::stateChanged, globalState, &GlobalState::setXiaozhiState);
    connect(m_worker, &XiaozhiWorker::statusChanged, globalState, &GlobalState::setXiaozhiStatus);
    connect(m_worker, &XiaozhiWorker::textChanged, globalState, &GlobalState::setXiaozhiText);
    m_workerThread.start();
}

XiaozhiModule::~XiaozhiModule()
{
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

QString XiaozhiModule::name() const
{
    return QStringLiteral("xiaozhi");
}

void XiaozhiModule::applyConfig(const AppConfig &config)
{
    m_config = config.xiaozhi;
}

void XiaozhiModule::start()
{
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection,
                              Q_ARG(XiaozhiConfig, m_config));
}

void XiaozhiModule::stop()
{
    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::QueuedConnection);
}

void XiaozhiModule::wake()
{
    QMetaObject::invokeMethod(m_worker, "wake", Qt::QueuedConnection);
}

void XiaozhiModule::hideOverlay()
{
    QMetaObject::invokeMethod(m_worker, "hideOverlay", Qt::QueuedConnection);
}
