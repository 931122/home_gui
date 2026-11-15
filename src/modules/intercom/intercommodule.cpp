#include "intercommodule.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <QUrlQuery>
#include <QUuid>

#include "core/configmanager.h"
#include "core/globalstate.h"
#include "backends/embeddedvoipbackend.h"
#include "backends/ivoipbackend.h"
#include "backends/voipbackendfactory.h"

IntercomModule *IntercomModule::s_instance = nullptr;

IntercomModule *IntercomModule::instance()
{
    return s_instance;
}

IntercomModule::IntercomModule(ConfigManager *configManager, GlobalState *globalState, QObject *parent)
    : IModule(globalState, parent)
    , m_configManager(configManager)
{
    s_instance = this;
    m_cookieJar = new AccessibleCookieJar(this);
    m_networkManager.setCookieJar(m_cookieJar);

    const QString initialBackend = m_configManager
        ? m_configManager->config().intercom.backend
        : QStringLiteral("auto");
    ensureVoipBackend(initialBackend);

    if (m_configManager) {
        connect(m_configManager, &ConfigManager::configReloaded, this, [this](const AppConfig &cfg) {
            if (!(this->globalState() && this->globalState()->voipActive())) {
                if (ensureVoipBackend(cfg.intercom.backend)) {
                    initVoipRuntime();
                }
            }
        });
    }

    connect(&m_audioRecorder, &AudioRecorder::recordingStarted, this, [this]() {
        if (this->globalState()) {
            this->globalState()->setIntercomRecording(true);
            this->globalState()->setIntercomRecordDuration(0);
        }
    });

    connect(&m_audioRecorder, &AudioRecorder::recordingStopped, this, [this](qint64) {
        if (this->globalState()) {
            this->globalState()->setIntercomRecording(false);
            this->globalState()->setIntercomAudioLevel(0.0);
        }
    });

    connect(&m_audioRecorder, &AudioRecorder::recordingCanceled, this, [this]() {
        if (this->globalState()) {
            this->globalState()->setIntercomRecording(false);
            this->globalState()->setIntercomAudioLevel(0.0);
        }
    });

    connect(&m_audioRecorder, &AudioRecorder::durationUpdated, this, [this](int sec) {
        if (this->globalState()) {
            this->globalState()->setIntercomRecordDuration(sec);
        }
    });

    connect(&m_audioRecorder, &AudioRecorder::audioLevelChanged, this, [this](qreal lvl) {
        if (this->globalState()) {
            this->globalState()->setIntercomAudioLevel(lvl);
        }
    });

    connect(&m_audioRecorder, &AudioRecorder::errorOccurred, this, [this](const QString &err) {
        if (this->globalState()) {
            this->globalState()->setIntercomRecording(false);
            this->globalState()->setIntercomStatus(err);
        }
    });
}

IntercomModule::~IntercomModule()
{
    stop();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

QString IntercomModule::name() const
{
    return QStringLiteral("IntercomModule");
}

void IntercomModule::applyConfig(const AppConfig &config)
{
    m_config = config.intercom;
    m_retriedVoip401 = false;
    m_retriedMicoapi5003 = false;
    ensureDeviceId();

    // 持久化兜底：如果配置文件中未读取到账号凭证，从 iOS/系统持久化 QSettings 恢复各个字段
    QSettings settings(QStringLiteral("home_gui"), QStringLiteral("intercom"));
    if (m_config.userId.isEmpty() && settings.contains(QStringLiteral("userId"))) {
        m_config.userId = settings.value(QStringLiteral("userId")).toString().trimmed();
    }
    if (m_config.serviceToken.isEmpty() && settings.contains(QStringLiteral("serviceToken"))) {
        m_config.serviceToken = settings.value(QStringLiteral("serviceToken")).toString().trimmed();
    }
    if (m_config.micoapiServiceToken.isEmpty() && settings.contains(QStringLiteral("micoapiServiceToken"))) {
        m_config.micoapiServiceToken = settings.value(QStringLiteral("micoapiServiceToken")).toString().trimmed();
    }
    if (m_config.refreshToken.isEmpty() && settings.contains(QStringLiteral("refreshToken"))) {
        m_config.refreshToken = settings.value(QStringLiteral("refreshToken")).toString().trimmed();
    }
    if (settings.contains(QStringLiteral("passToken")) && !settings.value(QStringLiteral("passToken")).toString().trimmed().isEmpty()) {
        m_config.passToken = settings.value(QStringLiteral("passToken")).toString().trimmed();
    }
    if (m_config.deviceId.isEmpty() && settings.contains(QStringLiteral("deviceId"))) {
        m_config.deviceId = settings.value(QStringLiteral("deviceId")).toString().trimmed();
    }
    if (m_config.homeId.isEmpty() && settings.contains(QStringLiteral("homeId"))) {
        m_config.homeId = settings.value(QStringLiteral("homeId")).toString().trimmed();
    }
    if (m_config.mimcId.isEmpty() && settings.contains(QStringLiteral("mimcId"))) {
        m_config.mimcId = settings.value(QStringLiteral("mimcId")).toString().trimmed();
    }
    if (settings.contains(QStringLiteral("enabled"))) {
        m_config.enabled = settings.value(QStringLiteral("enabled"), true).toBool();
    }
    qInfo() << "[IntercomModule] Initialized credentials: userId=" << m_config.userId
            << "enabled=" << m_config.enabled
            << "hasMicoapi=" << !m_config.micoapiServiceToken.isEmpty()
            << "hasV3ServiceToken=" << m_config.serviceToken.startsWith(QStringLiteral("V3_"))
            << "hasRefreshToken=" << !m_config.refreshToken.isEmpty()
            << "hasPassToken=" << !m_config.passToken.isEmpty();

    if (!m_config.userId.isEmpty() && (!m_config.serviceToken.isEmpty() || !m_config.micoapiServiceToken.isEmpty() || !m_config.passToken.isEmpty())) {
        m_config.enabled = true;
    }

    if (globalState()) {
        globalState()->setIntercomAvailable(m_config.enabled);
        globalState()->setIntercomUserId(m_config.userId);
        globalState()->setIntercomDeviceId(m_config.deviceId);
        globalState()->setIntercomServiceToken(m_config.serviceToken);
        globalState()->setIntercomHomeId(m_config.homeId);
        globalState()->setMicoapiServiceToken(m_config.micoapiServiceToken);
        globalState()->setMimcId(m_config.mimcId);

        if (!m_config.serviceToken.isEmpty() || !m_config.micoapiServiceToken.isEmpty() || !m_config.passToken.isEmpty()) {
            globalState()->setIntercomStatus(tr("已登录账号: %1").arg(m_config.userId));
        } else {
            globalState()->setIntercomStatus(tr("未登录"));
        }
    }
}

void IntercomModule::start()
{
    initDefaultRooms();

    if (!m_config.userId.isEmpty() && (!m_config.serviceToken.isEmpty() || !m_config.micoapiServiceToken.isEmpty() || !m_config.passToken.isEmpty())) {
        m_config.enabled = true;
        if (globalState()) {
            globalState()->setIntercomAvailable(true);
        }
    }

    if (m_config.enabled) {
        // 1. 同步音箱列表 (通过 micoapiToken 拉取 Mina 设备列表)
        fetchRooms();

        // 2. VoIP 通话初始化
        if (!m_config.micoapiServiceToken.isEmpty()) {
            initVoipRuntime();
            fetchVoipContacts();
        } else if (!m_config.userId.isEmpty() && !m_config.passToken.isEmpty()) {
            initVoipRuntime();
            exchangeTokenWithPassToken(QStringLiteral("micoapi"));
        }

        // 3. 自动检查并刷新/换取小爱 OAuth 凭证 (用于传声与语音上传)
        if (!m_config.serviceToken.startsWith(QStringLiteral("V3_"))) {
            if (!m_config.userId.isEmpty() && !m_config.passToken.isEmpty()) {
                qInfo() << "[IntercomModule] Initial serviceToken is not V3 OAuth token, exchanging via passToken...";
                exchangeOAuthTokenWithPassToken();
            } else if (!m_config.refreshToken.isEmpty()) {
                qInfo() << "[IntercomModule] Initial serviceToken is not V3 OAuth token, refreshing via refreshToken...";
                refreshOAuthToken();
            }
        }
    }
}

void IntercomModule::stop()
{
    cancelQrLogin();
    m_audioRecorder.cancelRecording();
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void IntercomModule::ensureDeviceId()
{
    if (m_config.deviceId.trimmed().isEmpty()) {
        // 生成 16 位的标准小爱/Android deviceId 伪随机十六进制
        const QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_config.deviceId = QCryptographicHash::hash(uuidStr.toUtf8(), QCryptographicHash::Md5).toHex().left(16).toLower();
    }
}

QString IntercomModule::buildAuthHeader() const
{
    // Authorization 必须使用独立的 XiaoAi OAuth access_token (V3_ 开头)
    const QString token = m_config.serviceToken.trimmed();
    if (token.isEmpty() || !token.startsWith(QStringLiteral("V3_"))) {
        return QString();
    }

    const QString devId = m_config.deviceId.trimmed().isEmpty()
        ? QStringLiteral("837a4f95cd5a32ed")
        : m_config.deviceId.trimmed();

    const QByteArray scopeJson = QStringLiteral("{\"d\":\"%1\"}").arg(devId).toUtf8();
    const QByteArray scopeData = scopeJson.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    return QStringLiteral("DO-TOKEN-V1 app_id:%1,scope_data:%2,access_token:%3")
        .arg(QStringLiteral("326813440150602752"), QString::fromUtf8(scopeData), token);
}

bool IntercomModule::importLocalXiaoAiToken()
{
    // 遵照要求，完全基于应用内官方 OAuth 与二维码流程，不使用任何 root (su) 提权
    return false;
}

void IntercomModule::refreshOAuthToken()
{
    if (m_config.refreshToken.trimmed().isEmpty()) {
        qWarning() << "[IntercomModule] No refreshToken available for refresh, falling back to passToken exchange...";
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
        }
        return;
    }

    qInfo() << "[IntercomModule] Refreshing XiaoAi OAuth token...";
    if (globalState()) {
        globalState()->setIntercomStatus(tr("正在刷新小爱授权凭证..."));
    }

    const QUrl url(QStringLiteral("https://account.xiaomi.com/oauth2/auth/token?pt=1&client_id=326813440150602752&client_secret=dWbMUXRKF6IACwDtvaXO-cOJQlnyQSc34chxCKudAdANCWrCtXdR5FYQjObaIZvXfUfDoYNm3-6gpAKL0aaxQg&redirect_uri=https%3A%2F%2Fi.ai.mi.com&grant_type=refresh_token&refresh_token=%1")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_config.refreshToken))));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleRefreshTokenReply);
}

void IntercomModule::handleRefreshTokenReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[IntercomModule] Refresh token network error:" << reply->errorString() << ", trying passToken exchange...";
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
        }
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
        }
        return;
    }

    const QJsonObject obj = doc.object();
    const QString newAccessToken = obj.value(QStringLiteral("access_token")).toString();
    const QString newRefreshToken = obj.value(QStringLiteral("refresh_token")).toString();
    const QString newDeviceId = obj.value(QStringLiteral("device_id")).toString();

    if (!newAccessToken.isEmpty()) {
        qInfo() << "[IntercomModule] OAuth token refreshed successfully!";
        m_config.serviceToken = newAccessToken;
        if (!newRefreshToken.isEmpty()) {
            m_config.refreshToken = newRefreshToken;
        }
        if (!newDeviceId.isEmpty()) {
            m_config.deviceId = newDeviceId;
        }

        // 同步备份至 iOS/系统持久化 QSettings (NSUserDefaults)，防冷启动丢失
        QSettings settings(QStringLiteral("home_gui"), QStringLiteral("intercom"));
        settings.setValue(QStringLiteral("enabled"), true);
        settings.setValue(QStringLiteral("userId"), m_config.userId);
        settings.setValue(QStringLiteral("serviceToken"), m_config.serviceToken);
        settings.setValue(QStringLiteral("micoapiServiceToken"), m_config.micoapiServiceToken);
        settings.setValue(QStringLiteral("deviceId"), m_config.deviceId);
        settings.setValue(QStringLiteral("homeId"), m_config.homeId);
        settings.setValue(QStringLiteral("refreshToken"), m_config.refreshToken);
        settings.setValue(QStringLiteral("passToken"), m_config.passToken);
        settings.setValue(QStringLiteral("mimcId"), m_config.mimcId);
        settings.sync();

        if (m_configManager) {
            m_configManager->updateIntercomConfig(
                m_config.enabled,
                m_config.userId,
                m_config.serviceToken,
                m_config.deviceId,
                m_config.homeId,
                m_config.refreshToken,
                m_config.micoapiServiceToken,
                m_config.mimcId,
                m_config.passToken
            );
        }
        if (globalState()) {
            globalState()->setIntercomServiceToken(m_config.serviceToken);
            globalState()->setIntercomDeviceId(m_config.deviceId);
            globalState()->setIntercomStatus(tr("凭证已自动刷新"));
        }

        fetchRooms();

        // 若之前有由于 401 触发刷新而暂存的录音，自动立即重发
        if (!m_pendingPcmData.isEmpty() && !m_pendingTargetRoomId.isEmpty()) {
            const QString targetRoomId = m_pendingTargetRoomId;
            const QString targetRoomName = m_pendingTargetRoomName;
            m_pendingTargetRoomId.clear();
            m_pendingTargetRoomName.clear();
            qInfo() << "[IntercomModule] Automatically resending pending voice after token refresh to:" << targetRoomName;
            sendPendingAudio(targetRoomId, targetRoomName);
        }
    } else {
        qWarning() << "[IntercomModule] Refresh token response missing access_token, falling back to passToken exchange:" << data;
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
        }
    }
}

static QString defaultIconForRoom(const QString &roomName)
{
    if (roomName.contains(QStringLiteral("卧"))) {
        return QStringLiteral("🛏️");
    } else if (roomName.contains(QStringLiteral("门")) || roomName.contains(QStringLiteral("玄关"))) {
        return QStringLiteral("🚪");
    } else if (roomName.contains(QStringLiteral("厅"))) {
        return QStringLiteral("🛋️");
    } else if (roomName.contains(QStringLiteral("厨"))) {
        return QStringLiteral("🍳");
    } else if (roomName.contains(QStringLiteral("餐"))) {
        return QStringLiteral("🍽️");
    } else if (roomName.contains(QStringLiteral("书"))) {
        return QStringLiteral("📚");
    } else if (roomName.contains(QStringLiteral("阳台"))) {
        return QStringLiteral("🪴");
    } else if (roomName.contains(QStringLiteral("卫")) || roomName.contains(QStringLiteral("洗"))) {
        return QStringLiteral("🚿");
    } else if (roomName.contains(QStringLiteral("音箱")) || roomName.contains(QStringLiteral("音响")) || roomName.contains(QStringLiteral("小爱"))) {
        return QStringLiteral("🔊");
    }
    return QStringLiteral("🏠");
}

void IntercomModule::initDefaultRooms()
{
    // 初始化已知房间映射
    m_roomNameToId.insert(tr("全屋广播"), QStringLiteral("WHOLE_0"));
    m_roomNameToId.insert(QStringLiteral("WHOLE_0"), QStringLiteral("WHOLE_0"));
    m_roomNameToId.insert(tr("北卧室"), QStringLiteral("552001369698"));
    m_roomNameToId.insert(QStringLiteral("北卧室"), QStringLiteral("552001369698"));

    m_deviceToRoomMap.insert(QStringLiteral("1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa"), QStringLiteral("552001369698"));
    m_deviceToRoomMap.insert(QStringLiteral("Xiaomi 智能音箱 Pro"), QStringLiteral("552001369698"));
    m_deviceToRoomMap.insert(QStringLiteral("866485066"), QStringLiteral("552001369698"));

    QVariantList defaultList;

    // 1. 全屋广播始终排在首位
    QVariantMap wholeHouse;
    wholeHouse.insert(QStringLiteral("roomId"), QStringLiteral("WHOLE_0"));
    wholeHouse.insert(QStringLiteral("roomName"), tr("全屋广播"));
    wholeHouse.insert(QStringLiteral("icon"), QStringLiteral("📢"));
    wholeHouse.insert(QStringLiteral("isWholeHouse"), true);
    defaultList.append(wholeHouse);

    // 2. 如果配置中指定了房间列表，优先使用配置文件中的房间
    if (!m_config.rooms.isEmpty()) {
        for (const auto &rc : m_config.rooms) {
            if (rc.roomName.trimmed().isEmpty()) continue;
            QVariantMap rm;
            rm.insert(QStringLiteral("roomId"), rc.roomId.isEmpty() ? rc.roomName : rc.roomId);
            rm.insert(QStringLiteral("roomName"), rc.roomName);
            rm.insert(QStringLiteral("icon"), rc.icon.isEmpty() ? defaultIconForRoom(rc.roomName) : rc.icon);
            rm.insert(QStringLiteral("isWholeHouse"), false);
            defaultList.append(rm);
        }
    } else if (globalState() && !globalState()->voipSpeakers().isEmpty()) {
        // 3. 如果已有音箱设备列表，展示实际存在小爱音箱的目标
        const QVariantList speakers = globalState()->voipSpeakers();
        QSet<QString> added;
        for (const auto &spVal : speakers) {
            const QVariantMap sp = spVal.toMap();
            const QString devName = sp.value(QStringLiteral("deviceName")).toString();
            const QString devId = sp.value(QStringLiteral("deviceId")).toString();
            if (devName.isEmpty() || added.contains(devName)) continue;
            added.insert(devName);

            // 建立设备到房间的映射
            if (devName.contains(QStringLiteral("Pro")) || devName.contains(tr("北卧")) || devId == QStringLiteral("1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa")) {
                m_deviceToRoomMap.insert(devId, QStringLiteral("552001369698"));
                m_deviceToRoomMap.insert(devName, QStringLiteral("552001369698"));
            }

            QVariantMap rm;
            rm.insert(QStringLiteral("roomId"), devId.isEmpty() ? devName : devId);
            rm.insert(QStringLiteral("roomName"), devName);
            rm.insert(QStringLiteral("icon"), QStringLiteral("🔊"));
            rm.insert(QStringLiteral("isWholeHouse"), false);
            defaultList.append(rm);
        }
    }

    if (defaultList.size() <= 1) {
        // 默认内置北卧室音箱房间
        QVariantMap northBed;
        northBed.insert(QStringLiteral("roomId"), QStringLiteral("552001369698"));
        northBed.insert(QStringLiteral("roomName"), tr("北卧室"));
        northBed.insert(QStringLiteral("icon"), QStringLiteral("🛏️"));
        northBed.insert(QStringLiteral("isWholeHouse"), false);
        defaultList.append(northBed);
    }

    if (globalState()) {
        if (globalState()->intercomRooms().isEmpty()) {
            globalState()->setIntercomRooms(defaultList);
        }
    }
}

bool IntercomModule::startRecording()
{
    if (!m_audioRecorder.startRecording()) {
        return false;
    }

    if (globalState()) {
        globalState()->setIntercomRecording(true);
        globalState()->setIntercomRecordDuration(0);
    }
    return true;
}

void IntercomModule::stopRecording()
{
    qint64 durationMs = 0;
    m_pendingPcmData = m_audioRecorder.stopRecording(&durationMs);
    m_pendingDurationMs = durationMs;

    qInfo() << "[IntercomModule::stopRecording] Recorded duration:" << durationMs
            << "ms, pcm bytes:" << m_pendingPcmData.size();

    if (globalState()) {
        globalState()->setIntercomRecording(false);
    }

    if (durationMs < 500) {
        emit sendAudioResult(false, tr("说话时间太短 (仅 %1ms)，未发送").arg(durationMs));
        return;
    }

    emit recordingReadyForRoomSelect(durationMs);
}

void IntercomModule::cancelRecording()
{
    m_audioRecorder.cancelRecording();
    m_pendingPcmData.clear();
    m_pendingDurationMs = 0;

    if (globalState()) {
        globalState()->setIntercomRecording(false);
    }
}

void IntercomModule::sendPendingAudio(const QString &roomId, const QString &roomName)
{
    if (m_pendingPcmData.isEmpty() || m_pendingDurationMs < 500) {
        emit sendAudioResult(false, tr("无有效音频数据 (录制: %1ms, %2字节)").arg(m_pendingDurationMs).arg(m_pendingPcmData.size()));
        return;
    }

    QString targetRoomId = roomId.trimmed().isEmpty() ? QStringLiteral("WHOLE_0") : roomId.trimmed();
    QString targetRoomName = roomName.trimmed().isEmpty() ? tr("全屋广播") : roomName.trimmed();

    // 智能解析：如果 targetRoomId 是设备 UUID（包含 '-'）或者音箱名称，自动解析为真实的 MiOT 房间 ID
    if (targetRoomId.contains(QLatin1Char('-')) || targetRoomId.contains(QStringLiteral("音箱")) || targetRoomName.contains(QStringLiteral("音箱"))) {
        if (m_deviceToRoomMap.contains(targetRoomId)) {
            const QString resolved = m_deviceToRoomMap.value(targetRoomId);
            qInfo() << "[IntercomModule::sendPendingAudio] Resolved device UUID" << targetRoomId << "to MiOT roomId:" << resolved;
            targetRoomId = resolved;
        } else if (m_deviceToRoomMap.contains(targetRoomName)) {
            targetRoomId = m_deviceToRoomMap.value(targetRoomName);
        } else if (m_roomNameToId.contains(targetRoomName)) {
            targetRoomId = m_roomNameToId.value(targetRoomName);
        } else if (targetRoomName.contains(tr("北卧")) || targetRoomName.contains(QStringLiteral("OH2P")) || targetRoomName.contains(QStringLiteral("Pro")) || targetRoomId == QStringLiteral("1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa")) {
            targetRoomId = m_roomNameToId.value(tr("北卧室"), QStringLiteral("552001369698"));
            targetRoomName = tr("北卧室");
        }
    } else if (m_roomNameToId.contains(targetRoomName)) {
        targetRoomId = m_roomNameToId.value(targetRoomName);
    } else if (targetRoomName.contains(tr("北卧"))) {
        targetRoomId = QStringLiteral("552001369698");
        targetRoomName = tr("北卧室");
    }

    // 必须持有有效的 XiaoAi OAuth access_token (以 V3_ 开头)
    if (!m_config.serviceToken.startsWith(QStringLiteral("V3_"))) {
        m_pendingTargetRoomId = targetRoomId;
        m_pendingTargetRoomName = targetRoomName;
        if (!m_config.passToken.isEmpty()) {
            qInfo() << "[IntercomModule::sendPendingAudio] V3 OAuth token not ready, exchanging via passToken...";
            exchangeOAuthTokenWithPassToken();
            emit sendAudioResult(false, tr("正在获取小爱授权凭证，请稍候..."));
            return;
        } else if (!m_config.refreshToken.isEmpty()) {
            qInfo() << "[IntercomModule::sendPendingAudio] V3 OAuth token not ready, refreshing via refreshToken...";
            refreshOAuthToken();
            emit sendAudioResult(false, tr("正在刷新小爱授权凭证，请稍候..."));
            return;
        } else {
            emit sendAudioResult(false, tr("未配置小爱语音授权凭证，请在设置中扫码登录"));
            return;
        }
    }

    ensureDeviceId();
    const QString targetHomeId = m_config.homeId.trimmed();
    const QString deviceId = m_config.deviceId.trimmed().isEmpty()
        ? QStringLiteral("837a4f95cd5a32ed")
        : m_config.deviceId.trimmed();

    qInfo() << "[IntercomModule::sendPendingAudio] Sending voice to room:" << targetRoomName
            << "(" << targetRoomId << ") pcmBytes=" << m_pendingPcmData.size()
            << "durationMs=" << m_pendingDurationMs
            << "deviceId=" << deviceId;

    m_pendingTargetRoomId = targetRoomId;
    m_pendingTargetRoomName = targetRoomName;

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 1. audio_file
    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"audio_file\"; filename=\"speech.pcm\"")));
    audioPart.setBody(m_pendingPcmData);
    multiPart->append(audioPart);

    auto addTextPart = [multiPart](const QString &name, const QString &value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
            QVariant(QStringLiteral("form-data; name=\"%1\"").arg(name)));
        part.setBody(value.toUtf8());
        multiPart->append(part);
    };

    // 2. requestId
    addTextPart(QStringLiteral("requestId"), QUuid::createUuid().toString(QUuid::WithoutBraces));

    // 3. to_room_id
    addTextPart(QStringLiteral("to_room_id"), targetRoomId);

    // 4. caller_device_id
    addTextPart(QStringLiteral("caller_device_id"), deviceId);

    // 5. to_home_id & caller_home_id
    if (!targetHomeId.isEmpty()) {
        addTextPart(QStringLiteral("to_home_id"), targetHomeId);
        addTextPart(QStringLiteral("caller_home_id"), targetHomeId);
    }

    // 6. duration_in_ms
    addTextPart(QStringLiteral("duration_in_ms"), QString::number(m_pendingDurationMs));

    // 7. codec
    addTextPart(QStringLiteral("codec"), QStringLiteral("pcm"));

    // 8. intercom_message json
    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("send_type"), targetRoomId == QStringLiteral("WHOLE_0") ? QStringLiteral("ALL") : QStringLiteral("SINGLE"));
    msgObj.insert(QStringLiteral("send_device_type"), QStringLiteral("PERSONAL"));
    msgObj.insert(QStringLiteral("send_device_category"), QStringLiteral("PHONE"));
    msgObj.insert(QStringLiteral("device_name"), tr("智能中控"));
    if (!targetHomeId.isEmpty()) {
        msgObj.insert(QStringLiteral("home_id"), targetHomeId);
        msgObj.insert(QStringLiteral("home_name"), tr("我的家"));
    }
    msgObj.insert(QStringLiteral("room_id"), targetRoomId);
    msgObj.insert(QStringLiteral("room_name"), targetRoomName);
    msgObj.insert(QStringLiteral("sender_device_id"), deviceId);

    addTextPart(QStringLiteral("intercom_message"), QString::fromUtf8(QJsonDocument(msgObj).toJson(QJsonDocument::Compact)));

    // Request
    const QUrl url(QStringLiteral("https://i.xiaomixiaoai.com/miot/intercom/upload_intercom_origin_audio"));
    QNetworkRequest request(url);
    const QString authHeader = buildAuthHeader();
    if (!authHeader.isEmpty()) {
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }
    const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(m_config.serviceToken, m_config.userId);
    request.setRawHeader("Cookie", cookieHeader.toUtf8());
    request.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14 Device/HomeGui");

    QNetworkReply *reply = m_networkManager.post(request, multiPart);
    multiPart->setParent(reply); // reply 会在析构时销毁 multiPart

    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleUploadAudioReply);
}

void IntercomModule::handleUploadAudioReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }
    reply->deleteLater();

    const QByteArray responseData = reply->readAll();
    qInfo() << "[IntercomModule] Upload audio response:" << responseData;

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 401 || reply->error() == QNetworkReply::AuthenticationRequiredError) {
        qWarning() << "[IntercomModule] Upload audio unauthorized (401), clearing token and attempting refresh / re-exchange...";
        m_config.serviceToken.clear();
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
        } else if (!m_config.refreshToken.isEmpty()) {
            refreshOAuthToken();
        }
        emit sendAudioResult(false, tr("小爱语音授权已过期，正在自动更新，请稍候重试"));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        const QJsonDocument errDoc = QJsonDocument::fromJson(responseData);
        if (!errDoc.isNull() && errDoc.isObject()) {
            const QString serverMsg = errDoc.object().value(QStringLiteral("message")).toString();
            if (!serverMsg.isEmpty()) {
                errMsg = serverMsg;
            }
        }
        const QString err = tr("发送失败: %1").arg(errMsg);
        qWarning() << "[IntercomModule] Upload audio network error:" << reply->error() << err;
        emit sendAudioResult(false, err);
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        emit sendAudioResult(false, tr("服务器响应异常"));
        return;
    }

    const QJsonObject root = doc.object();
    const int code = root.value(QStringLiteral("code")).toInt(-1);
    if (code == 200 || code == 0) {
        m_pendingTargetRoomId.clear();
        m_pendingTargetRoomName.clear();
        m_pendingPcmData.clear();
        m_pendingDurationMs = 0;
        emit sendAudioResult(true, tr("对讲语音发送成功"));
    } else {
        const QString msg = root.value(QStringLiteral("message")).toString(tr("错误码: %1").arg(code));
        emit sendAudioResult(false, tr("发送未成功: %1").arg(msg));
    }
}

void IntercomModule::saveCredentials(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &refreshToken, const QString &micoapiToken)
{
    const QString prevUserId = m_config.userId.trimmed();
    const QString trimmedUserId = userId.trimmed();
    const bool userChanged = !prevUserId.isEmpty() && !trimmedUserId.isEmpty() && prevUserId != trimmedUserId;

    m_config.userId = trimmedUserId;
    const QString tokenInput = serviceToken.trimmed();
    const QString micoapiInput = micoapiToken.trimmed();

    if (userChanged) {
        m_config.micoapiServiceToken.clear();
        m_config.passToken.clear();
        m_config.refreshToken.clear();
        m_config.mimcId.clear();
    }

    if (!tokenInput.isEmpty()) {
        if (tokenInput.startsWith(QStringLiteral("sXEJM"))) {
            // sXEJM 开头明确为 micoapi serviceToken
            m_config.micoapiServiceToken = tokenInput;
            if (m_config.serviceToken.isEmpty() || m_config.serviceToken.startsWith(QStringLiteral("sXEJM"))) {
                m_config.serviceToken = tokenInput;
            }
        } else {
            m_config.serviceToken = tokenInput;
        }
    }

    if (!micoapiInput.isEmpty()) {
        m_config.micoapiServiceToken = micoapiInput;
    }

    if (!deviceId.trimmed().isEmpty()) {
        m_config.deviceId = deviceId.trimmed();
    }
    if (!homeId.trimmed().isEmpty()) {
        m_config.homeId = homeId.trimmed();
    }
    if (!refreshToken.trimmed().isEmpty()) {
        m_config.refreshToken = refreshToken.trimmed();
    }
    m_retriedVoip401 = false;
    m_retriedMicoapi5003 = false;
    m_config.enabled = true;

    if (m_configManager) {
        m_configManager->updateIntercomConfig(
            m_config.enabled,
            m_config.userId,
            m_config.serviceToken,
            m_config.deviceId,
            m_config.homeId,
            m_config.refreshToken,
            m_config.micoapiServiceToken,
            m_config.mimcId,
            m_config.passToken
        );
    }

    // 同步备份至 iOS/系统持久化 QSettings (NSUserDefaults)，防冷启动丢失
    QSettings settings(QStringLiteral("home_gui"), QStringLiteral("intercom"));
    settings.setValue(QStringLiteral("enabled"), true);
    settings.setValue(QStringLiteral("userId"), m_config.userId);
    settings.setValue(QStringLiteral("serviceToken"), m_config.serviceToken);
    settings.setValue(QStringLiteral("micoapiServiceToken"), m_config.micoapiServiceToken);
    settings.setValue(QStringLiteral("deviceId"), m_config.deviceId);
    settings.setValue(QStringLiteral("homeId"), m_config.homeId);
    settings.setValue(QStringLiteral("refreshToken"), m_config.refreshToken);
    settings.setValue(QStringLiteral("passToken"), m_config.passToken);
    settings.setValue(QStringLiteral("mimcId"), m_config.mimcId);
    settings.sync();

    if (globalState()) {
        globalState()->setIntercomUserId(m_config.userId);
        globalState()->setIntercomServiceToken(m_config.serviceToken);
        globalState()->setMicoapiServiceToken(m_config.micoapiServiceToken);
        globalState()->setIntercomDeviceId(m_config.deviceId);
        globalState()->setIntercomHomeId(m_config.homeId);
        globalState()->setMimcId(m_config.mimcId);
        globalState()->setIntercomStatus(tr("已更新凭证，账号: %1").arg(m_config.userId));
    }

    if (!m_config.serviceToken.isEmpty()) {
        fetchRooms();
    }
    if (!m_config.micoapiServiceToken.isEmpty() || !m_config.serviceToken.isEmpty()) {
        initVoipRuntime();
        fetchVoipContacts();
    }
}

void IntercomModule::applyLoginTokens(const QString &userId, const QString &sid, const QString &token, const QString &passToken, const QString &statusMsg)
{
    const QString prevUserId = m_config.userId.trimmed();
    const QString trimmedUserId = userId.trimmed();
    if (!trimmedUserId.isEmpty() && !prevUserId.isEmpty() && prevUserId != trimmedUserId) {
        m_config.serviceToken.clear();
        m_config.micoapiServiceToken.clear();
        m_config.passToken.clear();
        m_config.refreshToken.clear();
        m_config.mimcId.clear();
    }

    if (!trimmedUserId.isEmpty()) {
        m_config.userId = trimmedUserId;
    }
    if (!passToken.trimmed().isEmpty()) {
        m_config.passToken = passToken.trimmed();
    }

    const QString cleanSid = sid.trimmed();
    const QString cleanToken = token.trimmed();
    if (!cleanToken.isEmpty()) {
        if (cleanSid == QStringLiteral("micoapi")) {
            m_config.micoapiServiceToken = cleanToken;
            m_retriedVoip401 = false;
            m_retriedMicoapi5003 = false;
        } else if (cleanSid == QStringLiteral("ai-service") || cleanToken.startsWith(QStringLiteral("V3_"))) {
            m_config.serviceToken = cleanToken;
        }
    }

    m_config.enabled = true;

    if (m_configManager) {
        m_configManager->updateIntercomConfig(
            m_config.enabled,
            m_config.userId,
            m_config.serviceToken,
            m_config.deviceId,
            m_config.homeId,
            m_config.refreshToken,
            m_config.micoapiServiceToken,
            m_config.mimcId,
            m_config.passToken
        );
    }

    // 同步备份至 iOS/系统持久化 QSettings (NSUserDefaults)，防冷启动丢失
    QSettings settings(QStringLiteral("home_gui"), QStringLiteral("intercom"));
    settings.setValue(QStringLiteral("enabled"), true);
    settings.setValue(QStringLiteral("userId"), m_config.userId);
    settings.setValue(QStringLiteral("serviceToken"), m_config.serviceToken);
    settings.setValue(QStringLiteral("micoapiServiceToken"), m_config.micoapiServiceToken);
    settings.setValue(QStringLiteral("deviceId"), m_config.deviceId);
    settings.setValue(QStringLiteral("homeId"), m_config.homeId);
    settings.setValue(QStringLiteral("refreshToken"), m_config.refreshToken);
    settings.setValue(QStringLiteral("passToken"), m_config.passToken);
    settings.setValue(QStringLiteral("mimcId"), m_config.mimcId);
    settings.sync();

    if (globalState()) {
        globalState()->setIntercomAvailable(true);
        globalState()->setIntercomUserId(m_config.userId);
        globalState()->setIntercomServiceToken(m_config.serviceToken);
        globalState()->setMicoapiServiceToken(m_config.micoapiServiceToken);
        globalState()->setIntercomDeviceId(m_config.deviceId);
        globalState()->setIntercomHomeId(m_config.homeId);
        globalState()->setMimcId(m_config.mimcId);
        globalState()->setIntercomStatus(
            !statusMsg.isEmpty() ? statusMsg : tr("已登录账号: %1").arg(m_config.userId));
    }

    qInfo() << "[IntercomModule] Saved login tokens for userId=" << m_config.userId
            << "sid=" << cleanSid
            << "hasMicoapi=" << !m_config.micoapiServiceToken.isEmpty()
            << "hasAiService=" << !m_config.serviceToken.isEmpty()
            << "hasPassToken=" << !m_config.passToken.isEmpty();

    fetchRooms();

    if (!m_config.micoapiServiceToken.isEmpty() || !m_config.serviceToken.isEmpty()) {
        initVoipRuntime();
        fetchVoipContacts();
    }

    if (cleanSid == QStringLiteral("micoapi")) {
        if (m_config.serviceToken.isEmpty() && !m_config.passToken.isEmpty() && !m_config.userId.isEmpty()) {
            exchangeTokenWithPassToken(QStringLiteral("ai-service"));
        }
    } else {
        if (m_config.micoapiServiceToken.isEmpty() && !m_config.passToken.isEmpty() && !m_config.userId.isEmpty()) {
            exchangeTokenWithPassToken(QStringLiteral("micoapi"));
        }
    }

    if (!m_config.passToken.isEmpty() && !m_config.serviceToken.startsWith(QStringLiteral("V3_"))) {
        qInfo() << "[IntercomModule] Automatically starting XiaoAi OAuth exchange via passToken...";
        exchangeOAuthTokenWithPassToken();
    }
}

void IntercomModule::login(const QString &username, const QString &password)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        emit loginResult(false, tr("用户名和密码不能为空"));
        return;
    }

    m_loginSid = QStringLiteral("micoapi");
    m_loginUser = username.trimmed();
    m_loginPwdHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex().toUpper();
    m_loginCandidateUserId.clear();
    m_loginCandidatePassToken.clear();
    clearAuthCookiesForSid(m_loginSid, true);

    // 步骤 1: 请求 micoapi 的 serviceLogin 获得 _sign, qs, callback
    const QUrl url(QStringLiteral("https://account.xiaomi.com/pass/serviceLogin?sid=micoapi&_json=true"));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    m_currentReply = m_networkManager.get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &IntercomModule::handleLoginStep1Reply);
}

void IntercomModule::handleLoginStep1Reply()
{
    if (!m_currentReply) return;
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginResult(false, tr("登录预检请求失败: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    if (data.startsWith("&&&START&&&")) {
        data.remove(0, 11);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit loginResult(false, tr("解析登录参数异常"));
        return;
    }

    const QJsonObject obj = doc.object();
    m_loginSign = obj.value(QStringLiteral("_sign")).toString();
    m_loginQs = obj.value(QStringLiteral("qs")).toString();
    m_loginCallback = obj.value(QStringLiteral("callback")).toString();

    // 步骤 2: 发送 serviceLoginAuth2
    const QUrl authUrl(QStringLiteral("https://account.xiaomi.com/pass/serviceLoginAuth2"));
    QNetworkRequest authRequest(authUrl);
    authRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    authRequest.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("user"), m_loginUser);
    query.addQueryItem(QStringLiteral("hash"), m_loginPwdHash);
    query.addQueryItem(QStringLiteral("sid"), m_loginSid);
    query.addQueryItem(QStringLiteral("_sign"), m_loginSign);
    query.addQueryItem(QStringLiteral("qs"), m_loginQs);
    query.addQueryItem(QStringLiteral("callback"), m_loginCallback);
    query.addQueryItem(QStringLiteral("_json"), QStringLiteral("true"));

    m_currentReply = m_networkManager.post(authRequest, query.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_currentReply, &QNetworkReply::finished, this, &IntercomModule::handleLoginStep2Reply);
}

void IntercomModule::handleLoginStep2Reply()
{
    if (!m_currentReply) return;
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginResult(false, tr("验证登录失败: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    if (data.startsWith("&&&START&&&")) {
        data.remove(0, 11);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit loginResult(false, tr("登录返回数据格式不正确"));
        return;
    }

    const QJsonObject obj = doc.object();
    const int code = obj.value(QStringLiteral("code")).toInt(-1);
    qInfo() << "[IntercomModule] Password login reply code:" << code << "sid:" << m_loginSid;

    if (code == 70016) {
        emit loginResult(false, tr("账号或密码错误"));
        return;
    }
    if (code == 87001 || obj.contains(QStringLiteral("notificationUrl"))) {
        emit loginResult(false, tr("该账号开启了双重验证或需要图形验证，请使用扫码或浏览器登录"));
        return;
    }

    const qint64 uidNum = obj.value(QStringLiteral("userId")).toVariant().toLongLong();
    const QString userId = uidNum > 0 ? QString::number(uidNum) : QString();
    const QString serviceToken = obj.value(QStringLiteral("serviceToken")).toString();
    const QString passToken = extractPassToken(reply, obj);
    const QString location = obj.value(QStringLiteral("location")).toString();
    const QString ssecurity = obj.value(QStringLiteral("ssecurity")).toString();
    const QString nonce = obj.value(QStringLiteral("nonce")).toVariant().toString();

    if (!serviceToken.isEmpty() && !userId.isEmpty()) {
        applyLoginTokens(userId, m_loginSid, serviceToken, passToken, tr("登录成功，账号: %1").arg(userId));
        emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
        return;
    }

    if (!location.isEmpty()) {
        // 请求重定向 location 获取最终 Set-Cookie (需要带 clientSign 并允许跨子域重定向)
        const QString stsUrl = buildStsUrl(location, nonce, ssecurity);
        const QUrl locUrl(stsUrl);
        QNetworkRequest locReq(locUrl);
        locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
        m_loginCandidateUserId = userId;
        m_loginCandidatePassToken = passToken;
        m_loginCandidateSsecurity = ssecurity;
        m_loginCandidateNonce = nonce;
        m_currentReply = m_networkManager.get(locReq);
        connect(m_currentReply, &QNetworkReply::finished, this, &IntercomModule::handleLoginLocationReply);
        return;
    }

    emit loginResult(false, tr("未能获取有效 Token: %1").arg(obj.value(QStringLiteral("description")).toString()));
}

QString IntercomModule::buildStsUrl(const QString &locationUrl, const QString &nonce, const QString &ssecurity)
{
    if (locationUrl.isEmpty()) {
        return locationUrl;
    }
    const QString cleanNonce = nonce.trimmed();
    const QString cleanSsec = ssecurity.trimmed();
    if (cleanNonce.isEmpty() || cleanSsec.isEmpty()) {
        return locationUrl;
    }
    // STS formula: clientSign = base64(sha1("nonce=" + nonce + "&" + ssecurity))
    const QString nsec = QStringLiteral("nonce=%1&%2").arg(cleanNonce, cleanSsec);
    const QByteArray sha = QCryptographicHash::hash(nsec.toUtf8(), QCryptographicHash::Sha1);
    const QString clientSign = QString::fromLatin1(sha.toBase64());
    const QString encSign = QString::fromUtf8(QUrl::toPercentEncoding(clientSign));

    if (locationUrl.contains(QLatin1Char('?'))) {
        return locationUrl + QStringLiteral("&clientSign=") + encSign;
    } else {
        return locationUrl + QStringLiteral("?clientSign=") + encSign;
    }
}

void IntercomModule::clearAuthCookiesForSid(const QString &sid, bool includePassToken)
{
    if (m_cookieJar) {
        QList<QNetworkCookie> remaining;
        for (const QNetworkCookie &c : m_cookieJar->allCookiesList()) {
            const QString name = QString::fromUtf8(c.name());
            const QString domain = c.domain().toLower();
            bool remove = false;

            if (name == QStringLiteral("serviceToken")) {
                if (sid.isEmpty()) {
                    remove = true;
                } else if (sid == QStringLiteral("micoapi") && (domain.contains(QStringLiteral("mina")) || domain.contains(QStringLiteral("xiaoaisound")))) {
                    remove = true;
                } else if (sid == QStringLiteral("ai-service") && domain.contains(QStringLiteral("ai.xiaomi"))) {
                    remove = true;
                }
            } else if (includePassToken && (name == QStringLiteral("passToken") || name == QStringLiteral("userId"))) {
                remove = true;
            }

            if (!remove) {
                remaining.append(c);
            }
        }
        m_cookieJar->setAllCookiesList(remaining);
        return;
    }

    auto *jar = m_networkManager.cookieJar();
    if (!jar) return;

    QList<QUrl> urls = {
        QUrl(QStringLiteral("https://account.xiaomi.com")),
        QUrl(QStringLiteral("https://mi.com"))
    };
    if (sid == QStringLiteral("micoapi") || sid.isEmpty()) {
        urls << QUrl(QStringLiteral("https://api2.mina.mi.com"))
             << QUrl(QStringLiteral("https://api.mina.mi.com"))
             << QUrl(QStringLiteral("https://voip.mina.xiaoaisound.com"));
    }
    if (sid == QStringLiteral("ai-service") || sid.isEmpty()) {
        urls << QUrl(QStringLiteral("https://account.ai.xiaomi.com"))
             << QUrl(QStringLiteral("https://i.xiaomixiaoai.com"));
    }

    for (const QUrl &u : urls) {
        const auto cookies = jar->cookiesForUrl(u);
        for (const QNetworkCookie &c : cookies) {
            if (c.name() == "serviceToken" ||
                (includePassToken && (c.name() == "passToken" || c.name() == "userId"))) {
                jar->deleteCookie(c);
            }
        }
    }
}

QString IntercomModule::extractCookieOrHeaderValue(QNetworkReply *reply, const QString &cookieName, const QList<QUrl> &preferredUrls) const
{
    if (!reply) return QString();

    const QByteArray nameBytes = cookieName.toUtf8();

    // 1. 从 Set-Cookie 头提取
    const QVariant cookiesVariant = reply->header(QNetworkRequest::SetCookieHeader);
    const QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie>>(cookiesVariant);
    for (const QNetworkCookie &c : cookies) {
        if (c.name() == nameBytes && !c.value().isEmpty()) {
            return QString::fromUtf8(c.value());
        }
    }

    // 2. 从 RawHeaderPairs 提取
    for (const auto &pair : reply->rawHeaderPairs()) {
        if (pair.first.compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
            const auto parsedCookies = QNetworkCookie::parseCookies(pair.second);
            for (const auto &c : parsedCookies) {
                if (c.name() == nameBytes && !c.value().isEmpty()) {
                    return QString::fromUtf8(c.value());
                }
            }
        }
    }

    // 3. 从 AccessibleCookieJar 提取所有 Cookie 搜索
    if (m_cookieJar) {
        for (const QNetworkCookie &c : m_cookieJar->allCookiesList()) {
            if (c.name() == nameBytes && !c.value().isEmpty()) {
                return QString::fromUtf8(c.value());
            }
        }
    }

    // 4. 从 preferredUrls 检查
    if (m_networkManager.cookieJar()) {
        for (const QUrl &u : preferredUrls) {
            if (!u.isValid() || u.isEmpty()) continue;
            const auto jarCookies = m_networkManager.cookieJar()->cookiesForUrl(u);
            for (const QNetworkCookie &c : jarCookies) {
                if (c.name() == nameBytes && !c.value().isEmpty()) {
                    return QString::fromUtf8(c.value());
                }
            }
        }
    }

    // 5. 从 URL 查询参数提取（若存在重定向附带）
    const QUrlQuery query(reply->url());
    if (query.hasQueryItem(cookieName)) {
        return query.queryItemValue(cookieName);
    }

    return QString();
}

QString IntercomModule::extractServiceToken(QNetworkReply *reply, const QString &sid)
{
    const QByteArray nameBytes = "serviceToken";
    if (!reply) return QString();

    // 1. Check raw reply header first (Set-Cookie)
    for (const auto &pair : reply->rawHeaderPairs()) {
        if (pair.first.compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
            const auto cookies = QNetworkCookie::parseCookies(pair.second);
            for (const auto &c : cookies) {
                if (c.name() == nameBytes && !c.value().isEmpty()) {
                    const QString val = QString::fromUtf8(c.value()).trimmed();
                    qInfo() << "[IntercomModule] Extracted serviceToken from raw header, domain=" << c.domain()
                            << "len=" << val.length();
                    return val;
                }
            }
        }
    }

    // 1.1 Check QNetworkRequest::SetCookieHeader
    const QVariant cookieHeaderVar = reply->header(QNetworkRequest::SetCookieHeader);
    if (cookieHeaderVar.isValid()) {
        const auto cookies = qvariant_cast<QList<QNetworkCookie>>(cookieHeaderVar);
        for (const auto &c : cookies) {
            if (c.name() == nameBytes && !c.value().isEmpty()) {
                const QString val = QString::fromUtf8(c.value()).trimmed();
                qInfo() << "[IntercomModule] Extracted serviceToken from SetCookieHeader, domain=" << c.domain()
                        << "len=" << val.length();
                return val;
            }
        }
    }

    // 1.2 Check Location redirect header query parameter
    const QString locHeader = QString::fromUtf8(reply->rawHeader("Location"));
    if (!locHeader.isEmpty() && locHeader.contains(QStringLiteral("serviceToken="))) {
        const QUrl locUrl(locHeader);
        const QUrlQuery q(locUrl.query());
        const QString t = q.queryItemValue(QStringLiteral("serviceToken")).trimmed();
        if (!t.isEmpty()) {
            qInfo() << "[IntercomModule] Extracted serviceToken from Location header, len=" << t.length();
            return t;
        }
    }

    // 1.3 Check response JSON body
    const QByteArray body = reply->peek(4096);
    if (!body.isEmpty()) {
        QByteArray cleanBody = body;
        if (cleanBody.startsWith("&&&START&&&")) {
            cleanBody.remove(0, 11);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(cleanBody);
        if (doc.isObject()) {
            const QString t = doc.object().value(QStringLiteral("serviceToken")).toString().trimmed();
            if (!t.isEmpty()) {
                qInfo() << "[IntercomModule] Extracted serviceToken from JSON body, len=" << t.length();
                return t;
            }
        }
    }

    // 2. Check CookieJar with domain filtering
    if (m_cookieJar) {
        const auto all = m_cookieJar->allCookiesList();
        QString domainMatchVal;
        QString anyMatchVal;

        for (const QNetworkCookie &c : all) {
            if (c.name() != nameBytes || c.value().isEmpty()) continue;
            const QString domain = c.domain().toLower();
            const QString val = QString::fromUtf8(c.value());

            if (sid == QStringLiteral("micoapi")) {
                if (domain.contains(QStringLiteral("mina")) || domain.contains(QStringLiteral("xiaoaisound"))) {
                    qInfo() << "[IntercomModule] Extracted micoapi serviceToken from cookieJar domain=" << c.domain() << "path=" << c.path();
                    return val;
                } else if (domain.contains(QStringLiteral("mi.com"))) {
                    domainMatchVal = val;
                }
            } else if (sid == QStringLiteral("ai-service")) {
                if (domain.contains(QStringLiteral("ai.xiaomi"))) {
                    qInfo() << "[IntercomModule] Extracted ai-service serviceToken from cookieJar domain=" << c.domain() << "path=" << c.path();
                    return val;
                } else if (domain.contains(QStringLiteral("xiaomi.com"))) {
                    domainMatchVal = val;
                }
            }
            if (anyMatchVal.isEmpty()) {
                anyMatchVal = val;
            }
        }

        if (!domainMatchVal.isEmpty()) {
            qInfo() << "[IntercomModule] Extracted serviceToken from cookieJar (domainMatch):" << domainMatchVal.left(10) << "...";
            return domainMatchVal;
        }
        if (!anyMatchVal.isEmpty()) {
            qInfo() << "[IntercomModule] Extracted serviceToken from cookieJar (anyMatch):" << anyMatchVal.left(10) << "...";
            return anyMatchVal;
        }
    }

    // 3. Fallback to checkUrls
    QList<QUrl> checkUrls;
    if (reply && reply->url().isValid()) {
        checkUrls << reply->url();
    }
    if (sid == QStringLiteral("micoapi")) {
        checkUrls << QUrl(QStringLiteral("https://api2.mina.mi.com/sts"))
                  << QUrl(QStringLiteral("https://api2.mina.mi.com"))
                  << QUrl(QStringLiteral("https://api.mina.mi.com"))
                  << QUrl(QStringLiteral("https://voip.mina.xiaoaisound.com"));
    } else if (sid == QStringLiteral("ai-service")) {
        checkUrls << QUrl(QStringLiteral("https://account.ai.xiaomi.com/sts"))
                  << QUrl(QStringLiteral("https://account.ai.xiaomi.com"))
                  << QUrl(QStringLiteral("https://i.xiaomixiaoai.com"));
    }
    checkUrls << QUrl(QStringLiteral("https://account.xiaomi.com"))
              << QUrl(QStringLiteral("https://mi.com"));

    return extractCookieOrHeaderValue(reply, QStringLiteral("serviceToken"), checkUrls);
}

QString IntercomModule::extractPassToken(QNetworkReply *reply, const QJsonObject &jsonObj) const
{
    const QString fromJson = jsonObj.value(QStringLiteral("passToken")).toString().trimmed();
    if (!fromJson.isEmpty()) {
        return fromJson;
    }

    QList<QUrl> urls = {
        QUrl(QStringLiteral("https://account.xiaomi.com")),
        QUrl(QStringLiteral("https://mi.com"))
    };
    if (reply && reply->url().isValid()) {
        urls.prepend(reply->url());
    }
    return extractCookieOrHeaderValue(reply, QStringLiteral("passToken"), urls).trimmed();
}

void IntercomModule::handleLoginLocationReply()
{
    if (!m_currentReply) return;
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    const QString token = extractServiceToken(reply, m_loginSid);
    const QString replyPassToken = extractPassToken(reply);
    const QString finalPassToken = !replyPassToken.isEmpty() ? replyPassToken : m_loginCandidatePassToken;
    const QString finalUserId = !m_loginCandidateUserId.isEmpty() ? m_loginCandidateUserId : m_config.userId;

    if (!token.isEmpty() && !finalUserId.isEmpty()) {
        applyLoginTokens(finalUserId, m_loginSid, token, finalPassToken, tr("登录成功，账号: %1").arg(finalUserId));
        emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
    } else if (!finalPassToken.isEmpty() && !finalUserId.isEmpty()) {
        qInfo() << "[IntercomModule] Login location did not yield serviceToken directly. Exchanging via passToken for sid=" << m_loginSid;
        m_config.userId = finalUserId;
        m_config.passToken = finalPassToken;
        exchangeTokenWithPassToken(m_loginSid);
    } else {
        emit loginResult(false, tr("获取 serviceToken 失败，请尝试扫码/浏览器登录或检查账号状态"));
    }
}

void IntercomModule::startQrLogin()
{
    cancelQrLogin();

    qInfo() << "[IntercomModule] Starting QR and web login flow (sid=micoapi)...";
    m_qrLoggingIn = true;
    m_qrLoginSid = QStringLiteral("micoapi");
    m_qrImageUrl.clear();
    m_qrWebUrl.clear();
    m_qrLpUrl.clear();
    m_qrCandidateUserId.clear();
    m_qrCandidatePassToken.clear();
    m_qrStartTimeSec = QDateTime::currentSecsSinceEpoch();
    m_qrTimeoutSec = 300;
    clearAuthCookiesForSid(m_qrLoginSid, true);

    if (globalState()) {
        globalState()->setIntercomQrLoggingIn(true);
        globalState()->setIntercomQrImageUrl(QString());
        globalState()->setIntercomQrWebUrl(QString());
        globalState()->setIntercomQrLoginStatus(tr("正在获取登录二维码与网页地址..."));
    }

    const QUrl url(QStringLiteral("https://account.xiaomi.com/longPolling/loginUrl?_qrsize=480&sid=micoapi&_locale=zh_CN"));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    m_currentReply = m_networkManager.get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &IntercomModule::handleQrLoginUrlReply);
}

void IntercomModule::cancelQrLogin()
{
    m_qrLoggingIn = false;
    if (m_qrPollReply) {
        m_qrPollReply->abort();
        m_qrPollReply->deleteLater();
        m_qrPollReply = nullptr;
    }
    if (m_qrLocationReply) {
        m_qrLocationReply->abort();
        m_qrLocationReply->deleteLater();
        m_qrLocationReply = nullptr;
    }
    if (globalState()) {
        globalState()->setIntercomQrLoggingIn(false);
        globalState()->setIntercomQrLoginStatus(QString());
    }
}

void IntercomModule::handleQrLoginUrlReply()
{
    if (!m_currentReply) return;
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (!m_qrLoggingIn) return;

    if (reply->error() != QNetworkReply::NoError) {
        m_qrLoggingIn = false;
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("获取登录地址失败: %1").arg(reply->errorString()));
        }
        emit loginResult(false, tr("获取登录地址失败: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    if (data.startsWith("&&&START&&&")) {
        data.remove(0, 11);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_qrLoggingIn = false;
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("解析登录参数异常"));
        }
        emit loginResult(false, tr("解析登录参数异常"));
        return;
    }

    const QJsonObject obj = doc.object();
    m_qrWebUrl = obj.value(QStringLiteral("loginUrl")).toString();
    m_qrImageUrl = obj.value(QStringLiteral("qr")).toString();
    m_qrLpUrl = obj.value(QStringLiteral("lp")).toString();
    m_qrTimeoutSec = obj.value(QStringLiteral("timeout")).toInt(300);
    m_qrStartTimeSec = QDateTime::currentSecsSinceEpoch();

    qInfo() << "[IntercomModule] QR login parameters received: qr=" << m_qrImageUrl << "loginUrl=" << m_qrWebUrl << "lp=" << m_qrLpUrl;

    if (globalState()) {
        globalState()->setIntercomQrImageUrl(m_qrImageUrl);
        globalState()->setIntercomQrWebUrl(m_qrWebUrl);
        globalState()->setIntercomQrLoginStatus(tr("请使用小米手机扫码，或点击在浏览器中打开登录"));
    }

    pollQrLogin();
}

void IntercomModule::pollQrLogin()
{
    if (!m_qrLoggingIn) return;
    if (m_qrLpUrl.isEmpty()) return;

    const qint64 elapsed = QDateTime::currentSecsSinceEpoch() - m_qrStartTimeSec;
    if (elapsed >= m_qrTimeoutSec) {
        m_qrLoggingIn = false;
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("登录超时，请重新获取二维码"));
        }
        emit loginResult(false, tr("登录超时，请重新获取二维码"));
        return;
    }

    if (m_qrPollReply) {
        m_qrPollReply->abort();
        m_qrPollReply->deleteLater();
        m_qrPollReply = nullptr;
    }

    const QUrl lpUrl(m_qrLpUrl);
    QNetworkRequest request(lpUrl);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    request.setTransferTimeout(35000);

    m_qrPollReply = m_networkManager.get(request);
    connect(m_qrPollReply, &QNetworkReply::finished, this, &IntercomModule::handleQrPollReply);
}

void IntercomModule::handleQrPollReply()
{
    if (!m_qrPollReply) return;
    QNetworkReply *reply = m_qrPollReply;
    m_qrPollReply = nullptr;
    reply->deleteLater();

    if (!m_qrLoggingIn) return;

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            const qint64 elapsed = QDateTime::currentSecsSinceEpoch() - m_qrStartTimeSec;
            if (elapsed < m_qrTimeoutSec) {
                QTimer::singleShot(600, this, &IntercomModule::pollQrLogin);
                return;
            }
        }
        m_qrLoggingIn = false;
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("登录已取消或已超时"));
        }
        return;
    }

    QByteArray data = reply->readAll();
    if (data.startsWith("&&&START&&&")) {
        data.remove(0, 11);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QTimer::singleShot(1000, this, &IntercomModule::pollQrLogin);
        return;
    }

    const QJsonObject obj = doc.object();
    const int code = obj.value(QStringLiteral("code")).toInt(-1);
    qInfo() << "[IntercomModule] QR poll reply code:" << code;

    const QString userId = obj.value(QStringLiteral("userId")).toVariant().toString();
    const QString location = obj.value(QStringLiteral("location")).toString();
    const QString directToken = obj.value(QStringLiteral("serviceToken")).toString();
    const QString passToken = extractPassToken(reply, obj);
    const QString ssecurity = obj.value(QStringLiteral("ssecurity")).toString();
    const QString nonce = obj.value(QStringLiteral("nonce")).toVariant().toString();

    if (code == 0 && (!userId.isEmpty() || !location.isEmpty() || !directToken.isEmpty())) {
        m_qrCandidateUserId = userId;
        if (!passToken.isEmpty()) {
            m_qrCandidatePassToken = passToken;
        }
        m_qrCandidateSsecurity = ssecurity;
        m_qrCandidateNonce = nonce;
        if (!directToken.isEmpty()) {
            applyLoginTokens(m_qrCandidateUserId, m_qrLoginSid, directToken, m_qrCandidatePassToken, tr("登录成功，账号: %1").arg(m_qrCandidateUserId));
            m_qrLoggingIn = false;
            if (globalState()) {
                globalState()->setIntercomQrLoggingIn(false);
                globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
            }
            emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
            return;
        }

        if (!location.isEmpty()) {
            if (globalState()) {
                globalState()->setIntercomQrLoginStatus(tr("已授权，正在获取通行凭证..."));
            }
            const QString stsUrl = buildStsUrl(location, nonce, ssecurity);
            qInfo() << "[IntercomModule] QR poll location URL:" << (stsUrl != location ? "signed with clientSign" : "unsigned")
                    << "host=" << QUrl(stsUrl).host();
            const QUrl locUrl(stsUrl);
            QNetworkRequest locReq(locUrl);
            locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
            locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

            if (m_qrLocationReply) {
                m_qrLocationReply->abort();
                m_qrLocationReply->deleteLater();
            }
            m_qrLocationReply = m_networkManager.get(locReq);
            connect(m_qrLocationReply, &QNetworkReply::finished, this, &IntercomModule::handleQrLocationReply);
            return;
        }
    }

    if (code > 0 && code != 70000) {
        if (obj.value(QStringLiteral("description")).toString().contains(QStringLiteral("expire"), Qt::CaseInsensitive)) {
            m_qrLoggingIn = false;
            if (globalState()) {
                globalState()->setIntercomQrLoggingIn(false);
                globalState()->setIntercomQrLoginStatus(tr("二维码已失效，请重新生成"));
            }
            emit loginResult(false, tr("二维码已失效，请重新生成"));
            return;
        }
    }

    QTimer::singleShot(800, this, &IntercomModule::pollQrLogin);
}

void IntercomModule::handleQrLocationReply()
{
    if (!m_qrLocationReply) return;
    QNetworkReply *reply = m_qrLocationReply;
    m_qrLocationReply = nullptr;
    reply->deleteLater();

    if (!m_qrLoggingIn) return;

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString token = extractServiceToken(reply, m_qrLoginSid);
    const QString replyPassToken = extractPassToken(reply);
    const QString finalPassToken = !replyPassToken.isEmpty() ? replyPassToken : m_qrCandidatePassToken;
    const QString finalUserId = !m_qrCandidateUserId.isEmpty() ? m_qrCandidateUserId : m_config.userId;
    qInfo() << "[IntercomModule] QR location reply sid=" << m_qrLoginSid
            << "http=" << httpStatus
            << "tokenExtracted=" << !token.isEmpty()
            << "passTokenExtracted=" << !finalPassToken.isEmpty()
            << "userId=" << finalUserId
            << "finalUrl=" << reply->url().host();

    if (!token.isEmpty()) {
        m_qrLoggingIn = false;
        applyLoginTokens(finalUserId, m_qrLoginSid, token, finalPassToken, tr("登录成功，账号: %1").arg(finalUserId));
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
        }
        emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
    } else if (!finalPassToken.isEmpty() && !finalUserId.isEmpty()) {
        qInfo() << "[IntercomModule] QR location did not yield serviceToken directly. Exchanging via passToken for sid=" << m_qrLoginSid;
        if (globalState()) {
            globalState()->setIntercomQrLoginStatus(tr("已获取授权，正在自动换取通话凭证..."));
        }
        m_config.userId = finalUserId;
        m_config.passToken = finalPassToken;
        exchangeTokenWithPassToken(m_qrLoginSid);
    } else {
        m_qrLoggingIn = false;
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("未能提取到 micoapi serviceToken，请重试或手动输入"));
        }
        emit loginResult(false, tr("未能提取到 micoapi serviceToken，请重试或手动输入"));
    }
}

void IntercomModule::exchangeTokenWithPassToken(const QString &sid)
{
    const QString cleanSid = sid.trimmed();
    const QString userId = m_config.userId.trimmed();
    const QString passToken = m_config.passToken.trimmed();
    if (cleanSid.isEmpty() || userId.isEmpty() || passToken.isEmpty()) {
        if (m_qrLoggingIn) {
            m_qrLoggingIn = false;
            if (globalState()) {
                globalState()->setIntercomQrLoggingIn(false);
                globalState()->setIntercomQrLoginStatus(tr("登录失败：缺少 passToken 或 userId"));
            }
            emit loginResult(false, tr("登录失败：缺少 passToken 或 userId"));
        }
        return;
    }
    if (m_pendingTokenExchanges.contains(cleanSid)) {
        return;
    }
    m_pendingTokenExchanges.insert(cleanSid);
    clearAuthCookiesForSid(cleanSid, false);

    if (auto *jar = m_cookieJar) {
        QList<QNetworkCookie> authCookies;
        QNetworkCookie cUid(QByteArrayLiteral("userId"), userId.toUtf8());
        cUid.setDomain(QStringLiteral(".account.xiaomi.com"));
        cUid.setPath(QStringLiteral("/"));
        authCookies.append(cUid);

        QNetworkCookie cPass(QByteArrayLiteral("passToken"), passToken.toUtf8());
        cPass.setDomain(QStringLiteral(".account.xiaomi.com"));
        cPass.setPath(QStringLiteral("/"));
        authCookies.append(cPass);

        jar->setCookiesFromUrl(authCookies, QUrl(QStringLiteral("https://account.xiaomi.com")));
    }

    const QUrl url(QStringLiteral("https://account.xiaomi.com/pass/serviceLogin?sid=%1&_json=true")
                       .arg(QString::fromUtf8(QUrl::toPercentEncoding(cleanSid))));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
    request.setRawHeader("Cookie", QStringLiteral("userId=%1; passToken=%2").arg(userId, passToken).toUtf8());

    qInfo() << "[IntercomModule] Exchanging token via passToken for sid=" << cleanSid;
    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cleanSid, userId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_pendingTokenExchanges.remove(cleanSid);
            qWarning() << "[IntercomModule] passToken exchange step1 error sid=" << cleanSid << reply->errorString();
            if (m_qrLoggingIn) {
                m_qrLoggingIn = false;
                if (globalState()) {
                    globalState()->setIntercomQrLoggingIn(false);
                    globalState()->setIntercomQrLoginStatus(tr("换取通话凭证网络错误: %1").arg(reply->errorString()));
                }
                emit loginResult(false, tr("换取通话凭证网络错误: %1").arg(reply->errorString()));
            }
            return;
        }

        QByteArray data = reply->readAll();
        if (data.startsWith("&&&START&&&")) {
            data.remove(0, 11);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            m_pendingTokenExchanges.remove(cleanSid);
            qWarning() << "[IntercomModule] passToken exchange invalid JSON sid=" << cleanSid;
            if (m_qrLoggingIn) {
                m_qrLoggingIn = false;
                if (globalState()) {
                    globalState()->setIntercomQrLoggingIn(false);
                    globalState()->setIntercomQrLoginStatus(tr("解析换取凭证响应失败"));
                }
                emit loginResult(false, tr("解析换取凭证响应失败"));
            }
            return;
        }

        const QJsonObject obj = doc.object();
        const int code = obj.value(QStringLiteral("code")).toInt(-1);
        const QString uidStr = obj.value(QStringLiteral("userId")).toVariant().toString();
        const QString resolvedUserId = (!uidStr.isEmpty() && uidStr != QStringLiteral("0")) ? uidStr : userId;
        const QString freshPassToken = extractPassToken(reply, obj);
        const QString directToken = obj.value(QStringLiteral("serviceToken")).toString();
        const QString location = obj.value(QStringLiteral("location")).toString();
        const QString ssecurity = obj.value(QStringLiteral("ssecurity")).toString();
        const QString nonce = obj.value(QStringLiteral("nonce")).toVariant().toString();

        if (code == 0 && !directToken.isEmpty()) {
            m_pendingTokenExchanges.remove(cleanSid);
            applyLoginTokens(resolvedUserId, cleanSid, directToken, freshPassToken);
            if (m_qrLoggingIn) {
                m_qrLoggingIn = false;
                if (globalState()) {
                    globalState()->setIntercomQrLoggingIn(false);
                    globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
                }
                emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
            }
            return;
        }

        if (code == 0 && !location.isEmpty()) {
            const QString stsUrl = buildStsUrl(location, nonce, ssecurity);
            qInfo() << "[IntercomModule] passToken exchange following location:" << (stsUrl != location ? "signed with clientSign" : "unsigned")
                    << "host=" << QUrl(stsUrl).host();
            QNetworkRequest locReq{QUrl(stsUrl)};
            locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
            locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
            QNetworkReply *locReply = m_networkManager.get(locReq);
            connect(locReply, &QNetworkReply::finished, this, [this, locReply, cleanSid, resolvedUserId, freshPassToken]() {
                locReply->deleteLater();
                m_pendingTokenExchanges.remove(cleanSid);
                const QString token = extractServiceToken(locReply, cleanSid);
                const QString locPass = extractPassToken(locReply);
                const QString finalPass = !locPass.isEmpty() ? locPass : freshPassToken;
                if (!token.isEmpty()) {
                    applyLoginTokens(resolvedUserId, cleanSid, token, finalPass);
                    if (m_qrLoggingIn) {
                        m_qrLoggingIn = false;
                        if (globalState()) {
                            globalState()->setIntercomQrLoggingIn(false);
                            globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
                        }
                        emit loginResult(true, tr("登录成功！已获取 VoIP 通话凭证并保存"));
                    }
                } else {
                    qWarning() << "[IntercomModule] passToken exchange location yielded empty token sid=" << cleanSid
                               << "http=" << locReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    if (m_qrLoggingIn) {
                        m_qrLoggingIn = false;
                        if (globalState()) {
                            globalState()->setIntercomQrLoggingIn(false);
                            globalState()->setIntercomQrLoginStatus(tr("未能提取到有效通话凭证，请重试"));
                        }
                        emit loginResult(false, tr("未能提取到 %1 serviceToken，请重试").arg(cleanSid));
                    }
                }
            });
            return;
        }

        m_pendingTokenExchanges.remove(cleanSid);
        qWarning() << "[IntercomModule] passToken exchange rejected sid=" << cleanSid << "code=" << code;
        if (m_qrLoggingIn) {
            m_qrLoggingIn = false;
            if (globalState()) {
                globalState()->setIntercomQrLoggingIn(false);
                globalState()->setIntercomQrLoginStatus(tr("换取通话凭证失败 (code: %1)").arg(code));
            }
            emit loginResult(false, tr("换取通话凭证失败 (code: %1)").arg(code));
        }
    });
}

void IntercomModule::exchangeOAuthTokenWithPassToken()
{
    const QString userId = m_config.userId.trimmed();
    const QString passToken = m_config.passToken.trimmed();
    if (userId.isEmpty() || passToken.isEmpty()) {
        qWarning() << "[IntercomModule] exchangeOAuthTokenWithPassToken skipped: missing userId or passToken";
        return;
    }
    if (m_oauthExchanging) {
        qInfo() << "[IntercomModule] OAuth token exchange already in progress, skipping.";
        return;
    }
    m_oauthExchanging = true;
    qInfo() << "[IntercomModule] Starting XiaoAi OAuth token exchange flow for userId=" << userId;

    if (auto *jar = m_cookieJar) {
        QList<QNetworkCookie> authCookies;
        QNetworkCookie cUid(QByteArrayLiteral("userId"), userId.toUtf8());
        cUid.setDomain(QStringLiteral(".account.xiaomi.com"));
        cUid.setPath(QStringLiteral("/"));
        authCookies.append(cUid);

        QNetworkCookie cPass(QByteArrayLiteral("passToken"), passToken.toUtf8());
        cPass.setDomain(QStringLiteral(".account.xiaomi.com"));
        cPass.setPath(QStringLiteral("/"));
        authCookies.append(cPass);

        jar->setCookiesFromUrl(authCookies, QUrl(QStringLiteral("https://account.xiaomi.com")));
    }

    requestOAuthAuthorizeCode(userId, passToken);
}

void IntercomModule::handleOAuthServiceLoginReply(QNetworkReply *reply, const QString &userId, const QString &passToken)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[IntercomModule] OAuth serviceLogin step error:" << reply->errorString();
        m_oauthExchanging = false;
        return;
    }

    QByteArray data = reply->readAll();
    if (data.startsWith("&&&START&&&")) {
        data.remove(0, 11);
    }
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "[IntercomModule] OAuth serviceLogin invalid JSON";
        m_oauthExchanging = false;
        return;
    }

    const QJsonObject obj = doc.object();
    const int code = obj.value(QStringLiteral("code")).toInt(-1);
    const QString location = obj.value(QStringLiteral("location")).toString();
    const QString ssecurity = obj.value(QStringLiteral("ssecurity")).toString();
    const QString nonce = obj.value(QStringLiteral("nonce")).toVariant().toString();

    if (code == 0 && !location.isEmpty()) {
        const QString stsUrl = buildStsUrl(location, nonce, ssecurity);
        qInfo() << "[IntercomModule] OAuth serviceLogin following STS:" << (stsUrl != location ? "signed with clientSign" : "unsigned") << QUrl(stsUrl).host();
        QNetworkRequest locReq{QUrl(stsUrl)};
        locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

        QNetworkReply *locReply = m_networkManager.get(locReq);
        connect(locReply, &QNetworkReply::finished, this, [this, locReply, userId, passToken]() {
            locReply->deleteLater();
            handleOAuthAuthorizeRedirect(locReply, userId, passToken, 1);
        });
        return;
    }

    qWarning() << "[IntercomModule] OAuth serviceLogin returned code=" << code << "desc=" << obj.value(QStringLiteral("desc")).toString();
    m_oauthExchanging = false;
}

void IntercomModule::requestOAuthAuthorizeCode(const QString &userId, const QString &passToken)
{
    qInfo() << "[IntercomModule] Starting OAuth code request for userId=" << userId << "hasPassToken=" << !passToken.isEmpty();
    const QUrl authUrl(QStringLiteral("https://account.xiaomi.com/oauth2/authorize?pt=1&client_id=326813440150602752&response_type=code&redirect_uri=https%3A%2F%2Fi.ai.mi.com&skip_confirm=true&_json=true"));
    QNetworkRequest req(authUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    // 组合 Cookie：包含 CookieJar 中所有针对 account.xiaomi.com 的 cookie (含有 oauth2.0 的 serviceToken)，并确保 userId 与 passToken
    QList<QNetworkCookie> jarCookies = m_cookieJar ? m_cookieJar->cookiesForUrl(QUrl(QStringLiteral("https://account.xiaomi.com"))) : QList<QNetworkCookie>();
    QStringList cookieParts;
    bool hasUid = false;
    bool hasPass = false;
    for (const auto &c : jarCookies) {
        cookieParts.append(QStringLiteral("%1=%2").arg(QString::fromUtf8(c.name()), QString::fromUtf8(c.value())));
        if (c.name() == "userId") hasUid = true;
        if (c.name() == "passToken") hasPass = true;
    }
    if (!hasUid && !userId.isEmpty()) {
        cookieParts.append(QStringLiteral("userId=%1").arg(userId));
    }
    if (!hasPass && !passToken.isEmpty()) {
        cookieParts.append(QStringLiteral("passToken=%1").arg(passToken));
    }
    req.setRawHeader("Cookie", cookieParts.join(QStringLiteral("; ")).toUtf8());

    qInfo() << "[IntercomModule] Requesting OAuth authorize code (_json=true), cookies count:" << cookieParts.size();
    QNetworkReply *reply = m_networkManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, userId, passToken]() {
        reply->deleteLater();
        handleOAuthAuthorizeRedirect(reply, userId, passToken, 0);
    });
}

void IntercomModule::handleOAuthAuthorizeRedirect(QNetworkReply *reply, const QString &userId, const QString &passToken, int hopCount)
{
    if (hopCount > 8) {
        qWarning() << "[IntercomModule] OAuth authorize redirect loop exceeded limit (8)";
        m_oauthExchanging = false;
        return;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    const QString locHeader = QString::fromUtf8(reply->rawHeader("Location"));
    const QUrl finalTargetUrl = redirectUrl.isValid() ? redirectUrl : QUrl(locHeader);

    qInfo() << "[IntercomModule] OAuth authorize reply http=" << statusCode
            << "target=" << finalTargetUrl.toString();

    // 提取并保存响应中携带的所有 Set-Cookie（包括 STS 签发的 serviceToken）
    QList<QNetworkCookie> replyCookies = qvariant_cast<QList<QNetworkCookie>>(reply->header(QNetworkRequest::SetCookieHeader));
    for (const auto &pair : reply->rawHeaderPairs()) {
        if (pair.first.compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
            replyCookies.append(QNetworkCookie::parseCookies(pair.second));
        }
    }
    if (m_cookieJar && !replyCookies.isEmpty()) {
        m_cookieJar->setCookiesFromUrl(replyCookies, reply->url());
        for (auto c : replyCookies) {
            c.setDomain(QStringLiteral(".account.xiaomi.com"));
            m_cookieJar->insertCookie(c);
            c.setDomain(QStringLiteral(".xiaomi.com"));
            m_cookieJar->insertCookie(c);
            qInfo() << "[IntercomModule] Preserved redirect Set-Cookie:" << c.name() << "valLen=" << c.value().length();
        }
    }

    // 1. 检查是否已经在目标 URL 中拿到授权码 code
    QString authCode;
    if (finalTargetUrl.isValid()) {
        QUrlQuery query(finalTargetUrl.query());
        authCode = query.queryItemValue(QStringLiteral("code"));
    }

    if (!authCode.isEmpty()) {
        qInfo() << "[IntercomModule] Successfully extracted OAuth code! Requesting access_token...";
        requestOAuthTokenByCode(authCode);
        return;
    }

    // 2. 若 301/302/303/307 重定向，继续跟进
    if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307) && finalTargetUrl.isValid()) {
        QUrl nextUrl = finalTargetUrl;
        if (nextUrl.isRelative()) {
            nextUrl = reply->url().resolved(nextUrl);
        }

        // 如果重定向到了 serviceLogin 登录入口，确保带上 _json=true
        if (nextUrl.path().contains(QStringLiteral("/pass/serviceLogin"))) {
            QUrlQuery q(nextUrl.query());
            if (!q.hasQueryItem(QStringLiteral("_json"))) {
                q.addQueryItem(QStringLiteral("_json"), QStringLiteral("true"));
            }
            nextUrl.setQuery(q);

            qInfo() << "[IntercomModule] Following OAuth serviceLogin with _json=true and passToken:" << nextUrl.host();
            QNetworkRequest nextReq(nextUrl);
            nextReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
            nextReq.setRawHeader("Cookie", QStringLiteral("userId=%1; passToken=%2").arg(userId, passToken).toUtf8());

            QNetworkReply *nextReply = m_networkManager.get(nextReq);
            connect(nextReply, &QNetworkReply::finished, this, [this, nextReply, userId, passToken]() {
                nextReply->deleteLater();
                handleOAuthServiceLoginReply(nextReply, userId, passToken);
            });
            return;
        }

        // 确保对 /oauth2/authorize 在非最终确认阶段附带 _json=true，在最终确认跳转阶段不附带 _json=true
        if (nextUrl.path().contains(QStringLiteral("/oauth2/authorize"))) {
            QUrlQuery q(nextUrl.query());
            if (q.queryItemValue(QStringLiteral("confirmed")) == QStringLiteral("true")) {
                q.removeAllQueryItems(QStringLiteral("_json"));
            } else if (!q.hasQueryItem(QStringLiteral("_json"))) {
                q.addQueryItem(QStringLiteral("_json"), QStringLiteral("true"));
            }
            nextUrl.setQuery(q);
        }

        qInfo() << "[IntercomModule] Following OAuth redirect (hop" << hopCount + 1 << ") to:" << nextUrl.toString();
        QNetworkRequest nextReq(nextUrl);
        nextReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        nextReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

        // 组合 Cookie：CookieJar 中的所有 cookie + userId & passToken，消除重复并同步 oauth2.0_serviceToken
        QMap<QString, QString> cookieMap;
        if (m_cookieJar) {
            for (const auto &c : m_cookieJar->allCookiesList()) {
                cookieMap.insert(QString::fromUtf8(c.name()), QString::fromUtf8(c.value()));
            }
        }
        if (cookieMap.contains(QStringLiteral("oauth2.0_serviceToken"))) {
            cookieMap.insert(QStringLiteral("serviceToken"), cookieMap.value(QStringLiteral("oauth2.0_serviceToken")));
        }
        if (cookieMap.contains(QStringLiteral("oauth2.0_slh"))) {
            cookieMap.insert(QStringLiteral("slh"), cookieMap.value(QStringLiteral("oauth2.0_slh")));
        }
        if (cookieMap.contains(QStringLiteral("oauth2.0_ph"))) {
            cookieMap.insert(QStringLiteral("ph"), cookieMap.value(QStringLiteral("oauth2.0_ph")));
        }
        if (!userId.isEmpty()) {
            cookieMap.insert(QStringLiteral("userId"), userId);
            cookieMap.insert(QStringLiteral("cUserId"), userId);
        }
        if (!passToken.isEmpty()) cookieMap.insert(QStringLiteral("passToken"), passToken);

        QStringList cookieParts;
        for (auto it = cookieMap.constBegin(); it != cookieMap.constEnd(); ++it) {
            cookieParts.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
        }
        nextReq.setRawHeader("Cookie", cookieParts.join(QStringLiteral("; ")).toUtf8());

        QNetworkReply *nextReply = m_networkManager.get(nextReq);
        connect(nextReply, &QNetworkReply::finished, this, [this, nextReply, userId, passToken, hopCount]() {
            nextReply->deleteLater();
            handleOAuthAuthorizeRedirect(nextReply, userId, passToken, hopCount + 1);
        });
        return;
    }

    // 3. 检查 200 响应中是否返回了 JSON（如包含 oauthLoginUrl 或 STS location）
    if (statusCode == 200) {
        QByteArray data = reply->readAll();
        qInfo() << "[IntercomModule] OAuth authorize 200 full body:" << data;
        if (data.startsWith("&&&START&&&")) {
            data.remove(0, 11);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const QJsonObject root = doc.object();
            const QJsonObject dataObj = root.value(QStringLiteral("data")).toObject();
            const QString oauthLoginUrl = dataObj.value(QStringLiteral("oauthLoginUrl")).toString();

            if (!oauthLoginUrl.isEmpty()) {
                qInfo() << "[IntercomModule] Authorize returned oauthLoginUrl with clientSign, proceeding to login...";
                QNetworkRequest loginReq{QUrl(oauthLoginUrl)};
                loginReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
                loginReq.setRawHeader("Cookie", QStringLiteral("userId=%1; passToken=%2").arg(userId, passToken).toUtf8());

                QNetworkReply *loginReply = m_networkManager.get(loginReq);
                connect(loginReply, &QNetworkReply::finished, this, [this, loginReply, userId, passToken]() {
                    loginReply->deleteLater();
                    handleOAuthServiceLoginReply(loginReply, userId, passToken);
                });
                return;
            }

            // 检查响应中是否直接携带重定向目标（如 {"data": {"code": 302, "redirectUrl": "https://i.ai.mi.com?code=C3_..."}}）
            QString redirectUrlStr = dataObj.value(QStringLiteral("redirectUrl")).toString();
            if (redirectUrlStr.isEmpty()) {
                redirectUrlStr = root.value(QStringLiteral("redirectUrl")).toString();
            }
            if (!redirectUrlStr.isEmpty()) {
                QUrl redUrl(redirectUrlStr);
                QString codeInRed = QUrlQuery(redUrl.query()).queryItemValue(QStringLiteral("code"));
                if (!codeInRed.isEmpty()) {
                    qInfo() << "[IntercomModule] Successfully extracted OAuth code from redirectUrl:" << codeInRed;
                    requestOAuthTokenByCode(codeInRed);
                    return;
                }
            }

            // 如果返回了 _ssign 和 scope_id，调用 POST /oauth2/userAuthorization 提交用户确认
            const QString ssign = dataObj.value(QStringLiteral("_ssign")).toString();
            if (!ssign.isEmpty()) {
                qInfo() << "[IntercomModule] Authorize returned _ssign, calling POST /oauth2/userAuthorization...";
                const QUrl postUrl(QStringLiteral("https://account.xiaomi.com/oauth2/userAuthorization"));
                QNetworkRequest postReq(postUrl);
                postReq.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
                postReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

                QMap<QString, QString> cookieMap;
                if (m_cookieJar) {
                    for (const auto &c : m_cookieJar->allCookiesList()) {
                        cookieMap.insert(QString::fromUtf8(c.name()), QString::fromUtf8(c.value()));
                    }
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_serviceToken"))) {
                    cookieMap.insert(QStringLiteral("serviceToken"), cookieMap.value(QStringLiteral("oauth2.0_serviceToken")));
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_slh"))) {
                    cookieMap.insert(QStringLiteral("slh"), cookieMap.value(QStringLiteral("oauth2.0_slh")));
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_ph"))) {
                    cookieMap.insert(QStringLiteral("ph"), cookieMap.value(QStringLiteral("oauth2.0_ph")));
                }
                if (!userId.isEmpty()) {
                    cookieMap.insert(QStringLiteral("userId"), userId);
                    cookieMap.insert(QStringLiteral("cUserId"), userId);
                }
                if (!passToken.isEmpty()) cookieMap.insert(QStringLiteral("passToken"), passToken);

                QStringList cookieParts;
                for (auto it = cookieMap.constBegin(); it != cookieMap.constEnd(); ++it) {
                    cookieParts.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
                }
                postReq.setRawHeader("Cookie", cookieParts.join(QStringLiteral("; ")).toUtf8());

                QUrlQuery postData;
                postData.addQueryItem(QStringLiteral("pt"), QString::number(dataObj.value(QStringLiteral("pt")).toInt(1)));
                postData.addQueryItem(QStringLiteral("device_id"), dataObj.value(QStringLiteral("device_id")).toString());
                postData.addQueryItem(QStringLiteral("followup"), dataObj.value(QStringLiteral("followup")).toString());
                postData.addQueryItem(QStringLiteral("scope_id"), dataObj.value(QStringLiteral("scope_id")).toString());
                postData.addQueryItem(QStringLiteral("redirect_uri"), dataObj.value(QStringLiteral("redirect_uri")).toString());
                postData.addQueryItem(QStringLiteral("client_id"), QStringLiteral("326813440150602752"));
                postData.addQueryItem(QStringLiteral("_ssign"), ssign);
                postData.addQueryItem(QStringLiteral("_json"), QStringLiteral("true"));

                const QByteArray postBody = postData.toString(QUrl::FullyEncoded).toUtf8();
                qInfo() << "[IntercomModule] POST body:" << postBody;
                qInfo() << "[IntercomModule] POST cookies:" << cookieParts.join(QStringLiteral("; "));

                QNetworkReply *postReply = m_networkManager.post(postReq, postBody);
                connect(postReply, &QNetworkReply::finished, this, [this, postReply, userId, passToken, hopCount]() {
                    postReply->deleteLater();
                    handleOAuthAuthorizeRedirect(postReply, userId, passToken, hopCount + 1);
                });
                return;
            }

            // 处理授权确认阶段返回的 followup 链接（已带有确认参数 sign, nonce, confirmed=true, scope 等）
            const QString followup = dataObj.value(QStringLiteral("followup")).toString();
            if (!followup.isEmpty()) {
                QUrl followUrl(followup);
                QUrlQuery followQ(followUrl.query());
                followQ.removeAllQueryItems(QStringLiteral("_json"));
                followUrl.setQuery(followQ);

                qInfo() << "[IntercomModule] Authorize returned confirmation followup, following (without _json) to:" << followUrl.toString();
                QNetworkRequest followReq{followUrl};
                followReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
                followReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

                QMap<QString, QString> cookieMap;
                if (m_cookieJar) {
                    for (const auto &c : m_cookieJar->allCookiesList()) {
                        cookieMap.insert(QString::fromUtf8(c.name()), QString::fromUtf8(c.value()));
                    }
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_serviceToken"))) {
                    cookieMap.insert(QStringLiteral("serviceToken"), cookieMap.value(QStringLiteral("oauth2.0_serviceToken")));
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_slh"))) {
                    cookieMap.insert(QStringLiteral("slh"), cookieMap.value(QStringLiteral("oauth2.0_slh")));
                }
                if (cookieMap.contains(QStringLiteral("oauth2.0_ph"))) {
                    cookieMap.insert(QStringLiteral("ph"), cookieMap.value(QStringLiteral("oauth2.0_ph")));
                }
                if (!userId.isEmpty()) {
                    cookieMap.insert(QStringLiteral("userId"), userId);
                    cookieMap.insert(QStringLiteral("cUserId"), userId);
                }
                if (!passToken.isEmpty()) cookieMap.insert(QStringLiteral("passToken"), passToken);

                QStringList cookieParts;
                for (auto it = cookieMap.constBegin(); it != cookieMap.constEnd(); ++it) {
                    cookieParts.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
                }
                followReq.setRawHeader("Cookie", cookieParts.join(QStringLiteral("; ")).toUtf8());

                QNetworkReply *followReply = m_networkManager.get(followReq);
                connect(followReply, &QNetworkReply::finished, this, [this, followReply, userId, passToken, hopCount]() {
                    followReply->deleteLater();
                    handleOAuthAuthorizeRedirect(followReply, userId, passToken, hopCount + 1);
                });
                return;
            }

            const QString codeInObj = dataObj.value(QStringLiteral("code")).toString();
            if (!codeInObj.isEmpty()) {
                qInfo() << "[IntercomModule] Found OAuth code directly in JSON data:" << codeInObj;
                requestOAuthTokenByCode(codeInObj);
                return;
            }

            QString location = root.value(QStringLiteral("location")).toString();
            if (location.isEmpty()) {
                location = dataObj.value(QStringLiteral("location")).toString();
            }
            if (!location.isEmpty()) {
                QUrl locUrl(location);
                QUrlQuery locQ(locUrl.query());
                QString codeInLoc = locQ.queryItemValue(QStringLiteral("code"));
                if (!codeInLoc.isEmpty()) {
                    qInfo() << "[IntercomModule] Extracted OAuth code from location:" << codeInLoc;
                    requestOAuthTokenByCode(codeInLoc);
                    return;
                }

                const QString nonce = root.value(QStringLiteral("nonce")).toVariant().toString();
                const QString ssec = root.value(QStringLiteral("ssecurity")).toString();
                const QString stsUrl = buildStsUrl(location, nonce, ssec);
                qInfo() << "[IntercomModule] Authorize returned STS location, following to:" << QUrl(stsUrl).host();
                QNetworkRequest locReq{QUrl(stsUrl)};
                locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
                locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

                QNetworkReply *locReply = m_networkManager.get(locReq);
                connect(locReply, &QNetworkReply::finished, this, [this, locReply, userId, passToken, hopCount]() {
                    locReply->deleteLater();
                    handleOAuthAuthorizeRedirect(locReply, userId, passToken, hopCount + 1);
                });
                return;
            }
        }
    }

    qWarning() << "[IntercomModule] Failed to obtain OAuth code from authorize flow, statusCode=" << statusCode;
    m_oauthExchanging = false;
}

void IntercomModule::requestOAuthTokenByCode(const QString &authCode)
{
    const QUrl tokenUrl(QStringLiteral("https://account.xiaomi.com/oauth2/auth/token?pt=1&client_id=326813440150602752&client_secret=dWbMUXRKF6IACwDtvaXO-cOJQlnyQSc34chxCKudAdANCWrCtXdR5FYQjObaIZvXfUfDoYNm3-6gpAKL0aaxQg&redirect_uri=https%3A%2F%2Fi.ai.mi.com&grant_type=authorization_code&code=%1")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(authCode))));

    QNetworkRequest req(tokenUrl);
    req.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    qInfo() << "[IntercomModule] Exchanging authCode for access_token and refresh_token...";
    QNetworkReply *reply = m_networkManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_oauthExchanging = false;

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[IntercomModule] Exchange OAuth code network error:" << reply->errorString();
            return;
        }

        const QByteArray data = reply->readAll();
        qInfo() << "[IntercomModule] OAuth code exchange reply:" << data;

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qWarning() << "[IntercomModule] OAuth code exchange invalid JSON";
            return;
        }

        const QJsonObject obj = doc.object();
        const QString accessToken = obj.value(QStringLiteral("access_token")).toString();
        const QString refreshToken = obj.value(QStringLiteral("refresh_token")).toString();
        const QString deviceId = obj.value(QStringLiteral("device_id")).toString();

        if (!accessToken.isEmpty()) {
            qInfo() << "[IntercomModule] XiaoAi OAuth access_token successfully acquired! length=" << accessToken.length();
            m_config.serviceToken = accessToken;
            if (!refreshToken.isEmpty()) {
                m_config.refreshToken = refreshToken;
            }
            if (!deviceId.isEmpty()) {
                m_config.deviceId = deviceId;
            }

            // 持久化到 QSettings 备份
            QSettings settings(QStringLiteral("home_gui"), QStringLiteral("intercom"));
            settings.setValue(QStringLiteral("serviceToken"), m_config.serviceToken);
            settings.setValue(QStringLiteral("refreshToken"), m_config.refreshToken);
            settings.setValue(QStringLiteral("deviceId"), m_config.deviceId);
            settings.sync();

            if (m_configManager) {
                m_configManager->updateIntercomConfig(
                    m_config.enabled,
                    m_config.userId,
                    m_config.serviceToken,
                    m_config.deviceId,
                    m_config.homeId,
                    m_config.refreshToken,
                    m_config.micoapiServiceToken,
                    m_config.mimcId,
                    m_config.passToken
                );
            }

            if (globalState()) {
                globalState()->setIntercomServiceToken(m_config.serviceToken);
                globalState()->setIntercomDeviceId(m_config.deviceId);
                globalState()->setIntercomStatus(tr("小爱语音授权就绪"));
            }

            // 重新拉取房间列表以更新音箱状态
            fetchRooms();

            // 若有之前处于暂存状态的待发录音，立即自动发送
            if (!m_pendingPcmData.isEmpty() && !m_pendingTargetRoomId.isEmpty()) {
                const QString targetRoomId = m_pendingTargetRoomId;
                const QString targetRoomName = m_pendingTargetRoomName;
                m_pendingTargetRoomId.clear();
                m_pendingTargetRoomName.clear();
                qInfo() << "[IntercomModule] Auto-sending pending audio with new OAuth token to:" << targetRoomName;
                sendPendingAudio(targetRoomId, targetRoomName);
            }
        } else {
            qWarning() << "[IntercomModule] OAuth token exchange response did not contain access_token:" << data;
        }
    });
}

void IntercomModule::fetchRooms()
{
    // 1. 同步拉取 Mina 设备列表（含每个音箱所在的房间 roomName / roomId）
    // Mina API 必须使用 micoapiServiceToken
    const QString minaToken = !m_config.micoapiServiceToken.trimmed().isEmpty()
        ? m_config.micoapiServiceToken.trimmed()
        : m_config.serviceToken.trimmed();

    if (!minaToken.isEmpty()) {
        fetchMinaDeviceRooms(minaToken);
    }

    // 2. 拉取 MiOT 小爱家庭传声房间列表 (需要 V3_ OAuth token)
    if (!m_config.serviceToken.startsWith(QStringLiteral("V3_"))) {
        qInfo() << "[IntercomModule::fetchRooms] No V3 OAuth token available yet for MiOT intercom rooms API";
        return;
    }

    ensureDeviceId();
    const QString authHeader = buildAuthHeader();
    const QString effectiveToken = m_config.serviceToken.trimmed();

    // 如果还没有 homeId，先拉取家庭成员获取默认 homeId
    if (m_config.homeId.trimmed().isEmpty()) {
        const QUrl memberUrl(QStringLiteral("https://i.xiaomixiaoai.com/miot/intercom/get_home_members?device_id=%1&reqId=%2")
            .arg(m_config.deviceId, QUuid::createUuid().toString(QUuid::WithoutBraces)));
        qInfo() << "[IntercomModule::fetchRooms] Querying home members from:" << memberUrl.toString();
        QNetworkRequest req(memberUrl);
        if (!authHeader.isEmpty()) {
            req.setRawHeader("Authorization", authHeader.toUtf8());
        }
        const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(effectiveToken, m_config.userId);
        req.setRawHeader("Cookie", cookieHeader.toUtf8());
        req.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

        QNetworkReply *reply = m_networkManager.get(req);
        connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleFetchHomeMembersReply);
        return;
    }

    const QUrl roomUrl(QStringLiteral("https://i.xiaomixiaoai.com/miot/intercom/get_home_rooms_and_messages?device_id=%1&home_id=%2")
        .arg(m_config.deviceId, m_config.homeId));
    qInfo() << "[IntercomModule::fetchRooms] Querying rooms from:" << roomUrl.toString();
    QNetworkRequest req(roomUrl);
    if (!authHeader.isEmpty()) {
        req.setRawHeader("Authorization", authHeader.toUtf8());
    }
    const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(effectiveToken, m_config.userId);
    req.setRawHeader("Cookie", cookieHeader.toUtf8());
    req.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    QNetworkReply *reply = m_networkManager.get(req);
    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleFetchRoomsReply);
}

void IntercomModule::fetchMinaDeviceRooms(const QString &token)
{
    const QString userId = m_config.userId.trimmed();
    if (token.isEmpty() || userId.isEmpty()) {
        return;
    }

    qInfo() << "[IntercomModule] Fetching Mina device list for speaker rooms...";
    const QUrl url(QStringLiteral("https://api2.mina.mi.com/admin/v2/device_list?master=1"));
    QNetworkRequest req(url);
    const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(token, userId);
    req.setRawHeader("Cookie", cookieHeader.toUtf8());
    req.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");

    QNetworkReply *reply = m_networkManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[IntercomModule] Mina device list error:" << reply->errorString();
            return;
        }
        const QByteArray data = reply->readAll();
        parseMinaDeviceRooms(data);
    });
}

void IntercomModule::parseMinaDeviceRooms(const QByteArray &payload)
{
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;
    const QJsonObject root = doc.object();
    const QJsonArray devices = root.value(QStringLiteral("data")).toArray();
    if (devices.isEmpty()) return;

    qInfo() << "[IntercomModule] Parsing Mina devices, count:" << devices.size();

    QVariantList list;
    QVariantMap wholeHouse;
    wholeHouse.insert(QStringLiteral("roomId"), QStringLiteral("WHOLE_0"));
    wholeHouse.insert(QStringLiteral("roomName"), tr("全屋广播"));
    wholeHouse.insert(QStringLiteral("icon"), QStringLiteral("📢"));
    wholeHouse.insert(QStringLiteral("isWholeHouse"), true);
    list.append(wholeHouse);

    QSet<QString> seenRoomNames;

    for (const auto &devVal : devices) {
        const QJsonObject devObj = devVal.toObject();
        QString roomName = devObj.value(QStringLiteral("roomName")).toString().trimmed();
        QString roomId;
        const QJsonValue roomIdVal = devObj.value(QStringLiteral("roomId"));
        if (roomIdVal.isDouble()) {
            roomId = QString::number(static_cast<qint64>(roomIdVal.toDouble()));
        } else if (roomIdVal.isString()) {
            roomId = roomIdVal.toString().trimmed();
        }

        const QString devName = devObj.value(QStringLiteral("name")).toString().trimmed();
        const QString devId = devObj.value(QStringLiteral("deviceID")).toString().trimmed();
        const QString did = devObj.value(QStringLiteral("did")).toString().trimmed();

        // 建立 Mina 设备 UUID / 名称到 MiOT 房间的映射
        if (!roomName.isEmpty()) {
            const QString rId = !roomId.isEmpty() ? roomId : m_roomNameToId.value(roomName, roomName);
            if (!devId.isEmpty()) m_deviceToRoomMap.insert(devId, rId);
            if (!devName.isEmpty()) m_deviceToRoomMap.insert(devName, rId);
            if (!did.isEmpty()) m_deviceToRoomMap.insert(did, rId);
        } else if (devName.contains(QStringLiteral("Pro")) || devName.contains(tr("北卧")) || devId == QStringLiteral("1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa")) {
            const QString rId = m_roomNameToId.value(tr("北卧室"), QStringLiteral("552001369698"));
            if (!devId.isEmpty()) m_deviceToRoomMap.insert(devId, rId);
            if (!devName.isEmpty()) m_deviceToRoomMap.insert(devName, rId);
            if (!did.isEmpty()) m_deviceToRoomMap.insert(did, rId);
        }

        QString devHomeId = devObj.value(QStringLiteral("homeId")).toString().trimmed();
        if (devHomeId.isEmpty() && devObj.value(QStringLiteral("home_id")).isString()) {
            devHomeId = devObj.value(QStringLiteral("home_id")).toString().trimmed();
        }
        if (!devHomeId.isEmpty() && m_config.homeId.trimmed().isEmpty()) {
            m_config.homeId = devHomeId;
            if (m_configManager) {
                m_configManager->updateIntercomConfig(m_config.enabled, m_config.userId, m_config.serviceToken, m_config.deviceId, m_config.homeId, m_config.refreshToken);
            }
            if (globalState()) {
                globalState()->setIntercomHomeId(m_config.homeId);
            }
        }

        if (!roomName.isEmpty()) {
            if (!seenRoomNames.contains(roomName)) {
                seenRoomNames.insert(roomName);
                QVariantMap rm;
                rm.insert(QStringLiteral("roomId"), !roomId.isEmpty() ? roomId : (!devId.isEmpty() ? devId : roomName));
                rm.insert(QStringLiteral("roomName"), roomName);
                rm.insert(QStringLiteral("icon"), defaultIconForRoom(roomName));
                rm.insert(QStringLiteral("isWholeHouse"), false);
                list.append(rm);
            }
        } else if (!devName.isEmpty()) {
            if (!seenRoomNames.contains(devName)) {
                seenRoomNames.insert(devName);
                QVariantMap rm;
                rm.insert(QStringLiteral("roomId"), !devId.isEmpty() ? devId : devName);
                rm.insert(QStringLiteral("roomName"), devName);
                rm.insert(QStringLiteral("icon"), QStringLiteral("🔊"));
                rm.insert(QStringLiteral("isWholeHouse"), false);
                list.append(rm);
            }
        }
    }

    // 关键守卫：若当前已加载了真正的 MiOT 房间（非 UUID 且非仅全屋广播），绝不用 Mina 纯设备列表覆盖
    if (globalState() && globalState()->intercomRooms().size() > 1) {
        bool hasRealMiotRooms = false;
        for (const auto &item : globalState()->intercomRooms()) {
            const QString rId = item.toMap().value(QStringLiteral("roomId")).toString();
            if (!rId.isEmpty() && rId != QStringLiteral("WHOLE_0") && !rId.contains(QLatin1Char('-'))) {
                hasRealMiotRooms = true;
                break;
            }
        }
        if (hasRealMiotRooms) {
            qInfo() << "[IntercomModule::parseMinaDeviceRooms] Genuine MiOT rooms already active, skipping overwrite from Mina devices.";
            return;
        }
    }

    if (list.size() > 1 && globalState()) {
        globalState()->setIntercomRooms(list);
        globalState()->setIntercomStatus(tr("已同步 %1 个音箱房间").arg(list.size() - 1));
        qInfo() << "[IntercomModule] Successfully loaded" << (list.size() - 1) << "XiaoAi speaker rooms from Mina device list.";
    }
}

void IntercomModule::handleFetchHomeMembersReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qInfo() << "[IntercomModule::handleFetchHomeMembersReply] statusCode:" << statusCode << "error:" << reply->errorString();
    if (statusCode == 401) {
        qWarning() << "[IntercomModule] Home members request unauthorized (401), attempting token refresh / re-exchange...";
        m_config.serviceToken.clear();
        if (!m_config.refreshToken.isEmpty()) {
            refreshOAuthToken();
            return;
        } else if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
            return;
        }
    }

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray payload = reply->readAll();
        qInfo() << "[IntercomModule::handleFetchHomeMembersReply] payload:" << payload;
        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject()) {
            const QJsonObject root = doc.object();
            QJsonArray homes;
            if (root.value(QStringLiteral("data")).isArray()) {
                homes = root.value(QStringLiteral("data")).toArray();
            } else if (root.value(QStringLiteral("data")).isObject()) {
                const QJsonObject dataObj = root.value(QStringLiteral("data")).toObject();
                if (dataObj.value(QStringLiteral("homelist")).isArray()) {
                    homes = dataObj.value(QStringLiteral("homelist")).toArray();
                } else if (dataObj.value(QStringLiteral("homes")).isArray()) {
                    homes = dataObj.value(QStringLiteral("homes")).toArray();
                } else if (dataObj.value(QStringLiteral("home_list")).isArray()) {
                    homes = dataObj.value(QStringLiteral("home_list")).toArray();
                }
            } else if (root.value(QStringLiteral("result")).isObject()) {
                const QJsonObject resObj = root.value(QStringLiteral("result")).toObject();
                if (resObj.value(QStringLiteral("homelist")).isArray()) {
                    homes = resObj.value(QStringLiteral("homelist")).toArray();
                } else if (resObj.value(QStringLiteral("homes")).isArray()) {
                    homes = resObj.value(QStringLiteral("homes")).toArray();
                }
            } else if (root.value(QStringLiteral("homelist")).isArray()) {
                homes = root.value(QStringLiteral("homelist")).toArray();
            }

            if (!homes.isEmpty()) {
                const QJsonObject firstHome = homes.first().toObject();
                QString fetchedHomeId;
                const QJsonValue homeIdVal = firstHome.contains(QStringLiteral("homeId"))
                    ? firstHome.value(QStringLiteral("homeId"))
                    : (firstHome.contains(QStringLiteral("home_id"))
                       ? firstHome.value(QStringLiteral("home_id"))
                       : firstHome.value(QStringLiteral("id")));

                if (homeIdVal.isDouble()) {
                    fetchedHomeId = QString::number(static_cast<qint64>(homeIdVal.toDouble()));
                } else if (homeIdVal.isString()) {
                    fetchedHomeId = homeIdVal.toString().trimmed();
                }

                if (!fetchedHomeId.isEmpty()) {
                    qInfo() << "[IntercomModule] Discovered primary homeId:" << fetchedHomeId;
                    m_config.homeId = fetchedHomeId;
                    if (m_configManager) {
                        m_configManager->updateIntercomConfig(m_config.enabled, m_config.userId, m_config.serviceToken, m_config.deviceId, m_config.homeId, m_config.refreshToken);
                    }
                    if (globalState()) {
                        globalState()->setIntercomHomeId(m_config.homeId);
                    }
                    // 有了 homeId 后立即拉取房间
                    fetchRooms();
                    return;
                }
            }
        }
    }

    // 降级使用默认房间
    initDefaultRooms();
}

void IntercomModule::handleFetchRoomsReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qInfo() << "[IntercomModule::handleFetchRoomsReply] statusCode:" << statusCode << "error:" << reply->errorString();
    if (statusCode == 401) {
        qWarning() << "[IntercomModule] Fetch rooms unauthorized (401), attempting token refresh / re-exchange...";
        m_config.serviceToken.clear();
        if (!m_config.passToken.isEmpty()) {
            exchangeOAuthTokenWithPassToken();
            return;
        } else if (!m_config.refreshToken.isEmpty()) {
            refreshOAuthToken();
            return;
        }
    }

    if (statusCode == 500) {
        // 如果 home_id 错误导致 500 ("head of empty list")，清空 homeId 并重新通过 get_home_members 获取
        qWarning() << "[IntercomModule] Fetch rooms got 500 with homeId:" << m_config.homeId << ", clearing homeId to re-query home members...";
        m_config.homeId.clear();
        fetchRooms();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[IntercomModule] Fetch rooms error:" << reply->errorString();
        initDefaultRooms();
        return;
    }

    const QByteArray payload = reply->readAll();
    qInfo() << "[IntercomModule::handleFetchRoomsReply] payload length:" << payload.length();
    parseRoomsFromJson(payload);
}

void IntercomModule::parseRoomsFromJson(const QByteArray &payload)
{
    qInfo() << "[IntercomModule::parseRoomsFromJson] Parsing rooms...";
    QVariantList list;

    // 第一项始终是全屋广播
    QVariantMap wholeHouse;
    wholeHouse.insert(QStringLiteral("roomId"), QStringLiteral("WHOLE_0"));
    wholeHouse.insert(QStringLiteral("roomName"), tr("全屋广播"));
    wholeHouse.insert(QStringLiteral("icon"), QStringLiteral("📢"));
    wholeHouse.insert(QStringLiteral("isWholeHouse"), true);
    list.append(wholeHouse);

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        QJsonArray rooms;

        if (root.value(QStringLiteral("data")).isObject()) {
            const QJsonObject data = root.value(QStringLiteral("data")).toObject();
            if (data.value(QStringLiteral("rooms")).isArray()) {
                rooms = data.value(QStringLiteral("rooms")).toArray();
            } else if (data.value(QStringLiteral("room_list")).isArray()) {
                rooms = data.value(QStringLiteral("room_list")).toArray();
            } else if (data.value(QStringLiteral("roomList")).isArray()) {
                rooms = data.value(QStringLiteral("roomList")).toArray();
            }
        } else if (root.value(QStringLiteral("data")).isArray()) {
            rooms = root.value(QStringLiteral("data")).toArray();
        } else if (root.value(QStringLiteral("result")).isObject()) {
            const QJsonObject res = root.value(QStringLiteral("result")).toObject();
            if (res.value(QStringLiteral("rooms")).isArray()) {
                rooms = res.value(QStringLiteral("rooms")).toArray();
            } else if (res.value(QStringLiteral("room_list")).isArray()) {
                rooms = res.value(QStringLiteral("room_list")).toArray();
            }
        } else if (root.value(QStringLiteral("rooms")).isArray()) {
            rooms = root.value(QStringLiteral("rooms")).toArray();
        }

        for (const auto &item : rooms) {
            const QJsonObject roomObj = item.toObject();
            QString roomId = roomObj.value(QStringLiteral("room_id")).toString();
            if (roomId.isEmpty()) roomId = roomObj.value(QStringLiteral("roomId")).toString();
            if (roomId.isEmpty() && roomObj.value(QStringLiteral("id")).isString()) roomId = roomObj.value(QStringLiteral("id")).toString();
            if (roomId.isEmpty() && roomObj.value(QStringLiteral("id")).isDouble()) roomId = QString::number(static_cast<qint64>(roomObj.value(QStringLiteral("id")).toDouble()));

            QString roomName = roomObj.value(QStringLiteral("room_name")).toString();
            if (roomName.isEmpty()) roomName = roomObj.value(QStringLiteral("roomName")).toString();
            if (roomName.isEmpty()) roomName = roomObj.value(QStringLiteral("name")).toString();

            if (!roomId.isEmpty() && roomId != QStringLiteral("WHOLE_0") && !roomName.isEmpty()) {
                m_roomNameToId.insert(roomName, roomId);
                m_roomNameToId.insert(roomId, roomId);
                qInfo() << "[IntercomModule::parseRoomsFromJson] Room registered:" << roomName << "->" << roomId;

                QVariantMap rm;
                rm.insert(QStringLiteral("roomId"), roomId);
                rm.insert(QStringLiteral("roomName"), roomName);
                rm.insert(QStringLiteral("isWholeHouse"), false);
                rm.insert(QStringLiteral("icon"), defaultIconForRoom(roomName));
                list.append(rm);
            }
        }
    }

    if (list.size() > 1) {
        if (globalState()) {
            globalState()->setIntercomRooms(list);
            globalState()->setIntercomStatus(tr("获取房间成功，共 %1 个有小爱的房间").arg(list.count() - 1));
            qInfo() << "[IntercomModule::parseRoomsFromJson] Updated intercom rooms with XiaoAi speakers, count:" << list.count();
        }
    }
}

void IntercomModule::initVoipRuntime()
{
    // Provide default speaker list so VoIP calling is immediately ready and visible
    if (globalState() && globalState()->voipSpeakers().isEmpty()) {
        QVariantList defaultSpeakers;

        QVariantMap sp1;
        sp1.insert(QStringLiteral("deviceId"), QStringLiteral("6a16de70-77aa-4d08-8364-e180c98507df"));
        sp1.insert(QStringLiteral("deviceName"), QStringLiteral("小爱音箱Pro"));
        sp1.insert(QStringLiteral("deviceModel"), QStringLiteral("LX06"));
        sp1.insert(QStringLiteral("did"), QStringLiteral("266346631"));
        sp1.insert(QStringLiteral("uid"), QStringLiteral("595739636"));
        sp1.insert(QStringLiteral("avatar"), QStringLiteral("http://cdn.cnbj1.fds.api.mi-img.com/speaker-img/model-img/LX06.png"));
        sp1.insert(QStringLiteral("presence"), QStringLiteral("online"));
        sp1.insert(QStringLiteral("isOnline"), true);
        sp1.insert(QStringLiteral("deviceType"), 105);
        defaultSpeakers.append(sp1);

        QVariantMap sp2;
        sp2.insert(QStringLiteral("deviceId"), QStringLiteral("1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa"));
        sp2.insert(QStringLiteral("deviceName"), QStringLiteral("Xiaomi 智能音箱 Pro"));
        sp2.insert(QStringLiteral("deviceModel"), QStringLiteral("OH2P"));
        sp2.insert(QStringLiteral("did"), QStringLiteral("866485066"));
        sp2.insert(QStringLiteral("uid"), QStringLiteral("629654798"));
        sp2.insert(QStringLiteral("avatar"), QStringLiteral("https://cdn.cnbj1.fds.api.mi-img.com/speaker-img/model-img/OH2P.png"));
        sp2.insert(QStringLiteral("presence"), QStringLiteral("online"));
        sp2.insert(QStringLiteral("isOnline"), true);
        sp2.insert(QStringLiteral("deviceType"), 105);
        defaultSpeakers.append(sp2);

        QVariantMap sp3;
        sp3.insert(QStringLiteral("deviceId"), QStringLiteral("c77f1a7b-fccf-4a5d-8a7a-49a7c7434313"));
        sp3.insert(QStringLiteral("deviceName"), QStringLiteral("Redmi小爱触屏音箱Pro 8英寸"));
        sp3.insert(QStringLiteral("deviceModel"), QStringLiteral("X08E"));
        sp3.insert(QStringLiteral("did"), QStringLiteral("379843557"));
        sp3.insert(QStringLiteral("uid"), QStringLiteral("504573265"));
        sp3.insert(QStringLiteral("avatar"), QStringLiteral("http://cdn.cnbj1.fds.api.mi-img.com/speaker-img/model-img/X08E.png"));
        sp3.insert(QStringLiteral("presence"), QStringLiteral("online"));
        sp3.insert(QStringLiteral("isOnline"), true);
        sp3.insert(QStringLiteral("deviceType"), 103);
        defaultSpeakers.append(sp3);

        globalState()->setVoipSpeakers(defaultSpeakers);
        globalState()->setVoipAvailable(true);
    }

    if (m_voipBackend) {
        const QString token = !m_config.micoapiServiceToken.isEmpty() ? m_config.micoapiServiceToken : m_config.serviceToken;
        m_voipBackend->init(m_config.userId, token, m_config.mimcId);
        syncBackendExtraTokens();
    }
}

void IntercomModule::fetchVoipContacts()
{
    const QString token = !m_config.micoapiServiceToken.isEmpty() ? m_config.micoapiServiceToken : m_config.serviceToken;
    if (token.isEmpty()) {
        qWarning() << "[IntercomModule] Cannot fetch VoIP contacts: token is empty.";
        return;
    }

    qInfo() << "[IntercomModule] Fetching VoIP contacts...";
    QUrl url(QStringLiteral("https://voip.mina.xiaoaisound.com/voip/proxy"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json;charset=UTF-8"));
    request.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");

    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2")
                               .arg(m_config.userId)
                               .arg(token);
    request.setRawHeader("Cookie", cookie.toUtf8());

    QJsonObject rootObj;
    rootObj.insert(QStringLiteral("path"), QStringLiteral("/inner/speaker/api/v1/user/contacts"));
    rootObj.insert(QStringLiteral("method"), QStringLiteral("get"));
    rootObj.insert(QStringLiteral("params"), QJsonObject());
    rootObj.insert(QStringLiteral("paramsType"), QStringLiteral("form"));

    QNetworkReply *reply = m_networkManager.post(request, QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleVoipContactsReply);
}

void IntercomModule::handleVoipContactsReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qWarning() << "[IntercomModule] Fetch VoIP contacts network error:" << httpStatus << reply->errorString();
        if (httpStatus == 401 && !m_retriedVoip401 && !m_config.userId.trimmed().isEmpty() && !m_config.passToken.trimmed().isEmpty()) {
            m_retriedVoip401 = true;
            exchangeTokenWithPassToken(QStringLiteral("micoapi"));
        }
        return;
    }

    const QByteArray payload = reply->readAll();
    qInfo() << "[IntercomModule] VoIP contacts reply length:" << payload.length();

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonValue dataVal = root.value(QStringLiteral("data"));
    QJsonObject dataObj;
    if (dataVal.isString()) {
        dataObj = QJsonDocument::fromJson(dataVal.toString().toUtf8()).object();
    } else if (dataVal.isObject()) {
        dataObj = dataVal.toObject();
    }

    QJsonArray users = dataObj.value(QStringLiteral("users")).toArray();
    QVariantList speakerList;

    for (const QJsonValue &uVal : users) {
        QJsonObject uObj = uVal.toObject();
        QVariantMap sp;
        sp.insert(QStringLiteral("deviceId"), uObj.value(QStringLiteral("deviceId")).toString());
        sp.insert(QStringLiteral("deviceName"), uObj.value(QStringLiteral("deviceName")).toString());
        sp.insert(QStringLiteral("deviceModel"), uObj.value(QStringLiteral("deviceModel")).toString());
        sp.insert(QStringLiteral("avatar"), uObj.value(QStringLiteral("defaultAvatar")).toString());
        sp.insert(QStringLiteral("presence"), uObj.value(QStringLiteral("presence")).toString());
        sp.insert(QStringLiteral("isOnline"), uObj.value(QStringLiteral("presence")).toString() == QStringLiteral("online"));
        sp.insert(QStringLiteral("did"), uObj.value(QStringLiteral("did")).toString());
        sp.insert(QStringLiteral("uid"), uObj.value(QStringLiteral("uid")).toString());
        sp.insert(QStringLiteral("deviceType"), uObj.value(QStringLiteral("deviceType")).toInt());
        speakerList.append(sp);
    }

    qInfo() << "[IntercomModule] Found" << speakerList.size() << "VoIP speakers";
    if (globalState()) {
        globalState()->setVoipSpeakers(speakerList);
        globalState()->setVoipAvailable(!speakerList.isEmpty());
        if (globalState()->intercomRooms().size() <= 1) {
            fetchRooms();
        }
    }
}

void IntercomModule::fetchXiaoaiDialogue(const QString &deviceId, const QString &hardware, qint64 timestamp, bool append)
{
    const QString token = !m_config.micoapiServiceToken.isEmpty() ? m_config.micoapiServiceToken : m_config.serviceToken;
    if (token.isEmpty() || m_config.userId.isEmpty()) {
        qWarning() << "[IntercomModule] Cannot fetch XiaoAi dialogue: token or userId is empty.";
        if (globalState()) {
            globalState()->setXiaoaiDialogueLoading(false);
            globalState()->setXiaoaiDialogueError(tr("未登录小米账号或凭证为空"));
        }
        return;
    }

    if (globalState()) {
        globalState()->setXiaoaiDialogueLoading(true);
        globalState()->setXiaoaiDialogueError(QString());
    }
    m_dialogueAppend = append;

    QString hw = hardware.trimmed();
    QString devId = deviceId.trimmed();
    if (hw.isEmpty() && globalState()) {
        const QVariantList speakers = globalState()->voipSpeakers();
        for (const QVariant &spVar : speakers) {
            const QVariantMap sp = spVar.toMap();
            if (devId.isEmpty() || sp.value(QStringLiteral("deviceId")).toString() == devId) {
                hw = sp.value(QStringLiteral("deviceModel")).toString();
                if (devId.isEmpty()) {
                    devId = sp.value(QStringLiteral("deviceId")).toString();
                }
                break;
            }
        }
    }
    if (hw.isEmpty()) {
        hw = QStringLiteral("LX06");
    }
    if (devId.isEmpty()) {
        devId = m_config.deviceId.trimmed();
    }

    const qint64 queryTs = (timestamp > 0) ? timestamp : QDateTime::currentMSecsSinceEpoch();

    QUrl url(QStringLiteral("https://userprofile.mina.mi.com/device_profile/v2/conversation"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("source"), QStringLiteral("dialogu"));
    query.addQueryItem(QStringLiteral("hardware"), hw);
    query.addQueryItem(QStringLiteral("timestamp"), QString::number(queryTs));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("20"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");
    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2; deviceId=%3")
                               .arg(m_config.userId)
                               .arg(token)
                               .arg(devId);
    request.setRawHeader("Cookie", cookie.toUtf8());

    qInfo() << "[IntercomModule] Fetching XiaoAi dialogue for hardware:" << hw << "deviceId:" << devId << "timestamp:" << queryTs;

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleXiaoaiDialogueReply);
}

void IntercomModule::handleXiaoaiDialogueReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (globalState()) {
        globalState()->setXiaoaiDialogueLoading(false);
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[IntercomModule] Fetch XiaoAi dialogue network error:" << httpStatus << reply->errorString();
        if (globalState()) {
            globalState()->setXiaoaiDialogueError(tr("网络错误 (%1): %2").arg(httpStatus).arg(reply->errorString()));
        }
        if (httpStatus == 401 && !m_retriedVoip401 && !m_config.userId.trimmed().isEmpty() && !m_config.passToken.trimmed().isEmpty()) {
            m_retriedVoip401 = true;
            exchangeTokenWithPassToken(QStringLiteral("micoapi"));
        }
        return;
    }

    const QByteArray payload = reply->readAll();
    qInfo() << "[IntercomModule] XiaoAi dialogue reply length:" << payload.length();
    qInfo().noquote() << "[XiaoAiDump]" << payload;

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        if (globalState()) {
            globalState()->setXiaoaiDialogueError(tr("返回数据格式错误"));
        }
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject dataObj;
    QJsonValue dataVal = root.value(QStringLiteral("data"));
    if (dataVal.isString()) {
        dataObj = QJsonDocument::fromJson(dataVal.toString().toUtf8()).object();
    } else if (dataVal.isObject()) {
        dataObj = dataVal.toObject();
    } else if (root.contains(QStringLiteral("records"))) {
        dataObj = root;
    }

    QJsonArray recordsArr = dataObj.value(QStringLiteral("records")).toArray();
    QVariantList recordList;
    qint64 earliestTs = 0;
    if (dataObj.contains(QStringLiteral("nextTimestamp"))) {
        earliestTs = dataObj.value(QStringLiteral("nextTimestamp")).toVariant().toLongLong();
    }

    for (const QJsonValue &rVal : recordsArr) {
        QJsonObject rObj = rVal.toObject();
        QVariantMap item;

        // 1. 时间戳解析：优先 "time"，次选 "timestamp"
        qint64 timeMs = rObj.value(QStringLiteral("time")).toVariant().toLongLong();
        if (timeMs <= 0) {
            timeMs = rObj.value(QStringLiteral("timestamp")).toVariant().toLongLong();
        }
        if (earliestTs == 0 || (timeMs > 0 && timeMs < earliestTs)) {
            earliestTs = timeMs;
        }

        // 2. 用户问题 query 解析 (支持 recordGroup 嵌套格式)
        QString query = rObj.value(QStringLiteral("query")).toString();
        if (query.isEmpty()) {
            query = rObj.value(QStringLiteral("question")).toString();
        }

        QJsonObject recordGroupObj;
        if (rObj.contains(QStringLiteral("recordGroup"))) {
            const QJsonValue rgVal = rObj.value(QStringLiteral("recordGroup"));
            if (rgVal.isString()) {
                recordGroupObj = QJsonDocument::fromJson(rgVal.toString().toUtf8()).object();
            } else if (rgVal.isObject()) {
                recordGroupObj = rgVal.toObject();
            }
        }
        if (!recordGroupObj.isEmpty() && query.isEmpty()) {
            query = recordGroupObj.value(QStringLiteral("user")).toObject().value(QStringLiteral("content")).toString();
        }

        item.insert(QStringLiteral("query"), query);
        item.insert(QStringLiteral("timestamp"), timeMs);

        QDateTime dt = QDateTime::fromMSecsSinceEpoch(timeMs);
        item.insert(QStringLiteral("timeStr"), dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        item.insert(QStringLiteral("timeShort"), dt.toString(QStringLiteral("HH:mm")));

        // 3. 回答解析 (支持 answers 数组、recordGroup.xiaoai、以及顶层 toSpeak/content)
        QString replySummary;
        QVariantList answersList;

        // 3a. 先从 recordGroup.xiaoai 中提取
        if (!recordGroupObj.isEmpty()) {
            const QJsonObject xiaoaiObj = recordGroupObj.value(QStringLiteral("xiaoai")).toObject();
            const QString xContent = xiaoaiObj.value(QStringLiteral("content")).toString();
            if (!xContent.isEmpty()) {
                replySummary = xContent;
            }
            // 检查 instructions
            QJsonArray instArr = xiaoaiObj.value(QStringLiteral("instructionList")).toArray();
            if (instArr.isEmpty() && xiaoaiObj.value(QStringLiteral("instructions")).isString()) {
                instArr = QJsonDocument::fromJson(xiaoaiObj.value(QStringLiteral("instructions")).toString().toUtf8()).array();
            }
            for (const QJsonValue &iVal : instArr) {
                const QString payloadText = iVal.toObject().value(QStringLiteral("payload")).toObject().value(QStringLiteral("text")).toString();
                if (!payloadText.isEmpty()) {
                    if (replySummary.isEmpty()) replySummary = payloadText;
                    break;
                }
            }
            // 检查 widgets
            QJsonArray widgetArr = xiaoaiObj.value(QStringLiteral("widgetList")).toArray();
            if (widgetArr.isEmpty() && xiaoaiObj.value(QStringLiteral("widget")).isString()) {
                widgetArr = QJsonDocument::fromJson(xiaoaiObj.value(QStringLiteral("widget")).toString().toUtf8()).array();
            }
            for (const QJsonValue &wVal : widgetArr) {
                const QJsonObject wObj = wVal.toObject();
                const QString wText = wObj.value(QStringLiteral("text")).toString();
                const QString wTitle = wObj.value(QStringLiteral("title")).toString();
                if (!wText.isEmpty()) {
                    if (replySummary.isEmpty()) replySummary = wText;
                    break;
                } else if (!wTitle.isEmpty()) {
                    if (replySummary.isEmpty()) replySummary = wTitle;
                    break;
                }
            }
        }

        // 3b. 检查 rObj 顶层文本字段
        if (replySummary.isEmpty()) {
            if (rObj.contains(QStringLiteral("toSpeak")) && !rObj.value(QStringLiteral("toSpeak")).toString().isEmpty()) {
                replySummary = rObj.value(QStringLiteral("toSpeak")).toString();
            } else if (rObj.contains(QStringLiteral("content")) && !rObj.value(QStringLiteral("content")).toString().isEmpty()) {
                replySummary = rObj.value(QStringLiteral("content")).toString();
            } else if (rObj.contains(QStringLiteral("reply")) && !rObj.value(QStringLiteral("reply")).toString().isEmpty()) {
                replySummary = rObj.value(QStringLiteral("reply")).toString();
            } else if (rObj.contains(QStringLiteral("text")) && !rObj.value(QStringLiteral("text")).toString().isEmpty()) {
                replySummary = rObj.value(QStringLiteral("text")).toString();
            }
        }

        // 3c. 解析 answers 数组
        QJsonArray answersArr = rObj.value(QStringLiteral("answers")).toArray();
        for (const QJsonValue &aVal : answersArr) {
            if (aVal.isString()) {
                const QString sText = aVal.toString().trimmed();
                if (!sText.isEmpty()) {
                    QVariantMap ans;
                    ans.insert(QStringLiteral("type"), QStringLiteral("TEXT"));
                    ans.insert(QStringLiteral("text"), sText);
                    answersList.append(ans);
                    if (replySummary.isEmpty()) replySummary = sText;
                }
                continue;
            }

            if (!aVal.isObject()) continue;
            QJsonObject aObj = aVal.toObject();
            QVariantMap ans;
            const QString type = aObj.value(QStringLiteral("type")).toString();
            ans.insert(QStringLiteral("type"), type);

            QString ansText;
            // 1. 小爱大模型 (LLM) 回复，例如天气、问答、闲聊等
            if (aObj.contains(QStringLiteral("llm"))) {
                const QJsonValue llmVal = aObj.value(QStringLiteral("llm"));
                if (llmVal.isString()) {
                    ansText = llmVal.toString();
                } else if (llmVal.isObject()) {
                    const QJsonObject llmObj = llmVal.toObject();
                    ansText = llmObj.value(QStringLiteral("text")).toString();
                    if (ansText.isEmpty()) ansText = llmObj.value(QStringLiteral("content")).toString();
                    if (ansText.isEmpty()) ansText = llmObj.value(QStringLiteral("toSpeak")).toString();
                }
            }
            // 2. 传统 TTS 语音回复
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("tts"))) {
                const QJsonValue ttsVal = aObj.value(QStringLiteral("tts"));
                if (ttsVal.isString()) {
                    ansText = ttsVal.toString();
                } else if (ttsVal.isObject()) {
                    const QJsonObject ttsObj = ttsVal.toObject();
                    ansText = ttsObj.value(QStringLiteral("text")).toString();
                    if (ansText.isEmpty()) ansText = ttsObj.value(QStringLiteral("toSpeak")).toString();
                    if (ansText.isEmpty()) ansText = ttsObj.value(QStringLiteral("content")).toString();
                }
            }
            // 3. nlp 文本回复
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("nlp"))) {
                const QJsonValue nlpVal = aObj.value(QStringLiteral("nlp"));
                if (nlpVal.isString()) {
                    ansText = nlpVal.toString();
                } else if (nlpVal.isObject()) {
                    const QJsonObject nlpObj = nlpVal.toObject();
                    ansText = nlpObj.value(QStringLiteral("text")).toString();
                    if (ansText.isEmpty()) ansText = nlpObj.value(QStringLiteral("toSpeak")).toString();
                    if (ansText.isEmpty()) ansText = nlpObj.value(QStringLiteral("content")).toString();
                }
            }
            // 4. 直接字段检查
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("text"))) {
                ansText = aObj.value(QStringLiteral("text")).toString();
            }
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("toSpeak"))) {
                ansText = aObj.value(QStringLiteral("toSpeak")).toString();
            }
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("content"))) {
                ansText = aObj.value(QStringLiteral("content")).toString();
            }
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("answer"))) {
                ansText = aObj.value(QStringLiteral("answer")).toString();
            }
            if (ansText.isEmpty() && aObj.contains(QStringLiteral("payload"))) {
                ansText = aObj.value(QStringLiteral("payload")).toObject().value(QStringLiteral("text")).toString();
            }

            // 5. 通用子对象遍历兜底：如果 aObj 的任何子对象包含有效 text/content/toSpeak
            if (ansText.isEmpty()) {
                for (auto it = aObj.constBegin(); it != aObj.constEnd(); ++it) {
                    if (it.value().isObject()) {
                        const QJsonObject subObj = it.value().toObject();
                        if (subObj.contains(QStringLiteral("text")) && !subObj.value(QStringLiteral("text")).toString().trimmed().isEmpty()) {
                            ansText = subObj.value(QStringLiteral("text")).toString();
                            break;
                        }
                        if (subObj.contains(QStringLiteral("content")) && !subObj.value(QStringLiteral("content")).toString().trimmed().isEmpty()) {
                            ansText = subObj.value(QStringLiteral("content")).toString();
                            break;
                        }
                        if (subObj.contains(QStringLiteral("toSpeak")) && !subObj.value(QStringLiteral("toSpeak")).toString().trimmed().isEmpty()) {
                            ansText = subObj.value(QStringLiteral("toSpeak")).toString();
                            break;
                        }
                    }
                }
            }

            if (!ansText.isEmpty()) {
                ans.insert(QStringLiteral("text"), ansText);
                if (replySummary.isEmpty()) replySummary = ansText;
            }

            if (type == QStringLiteral("AUDIO") || aObj.contains(QStringLiteral("audio"))) {
                QJsonObject audioObj = aObj.value(QStringLiteral("audio")).toObject();
                QJsonArray infoList = audioObj.value(QStringLiteral("audioInfoList")).toArray();
                if (!infoList.isEmpty()) {
                    QJsonObject firstAudio = infoList.at(0).toObject();
                    const QString title = firstAudio.value(QStringLiteral("title")).toString();
                    const QString artist = firstAudio.value(QStringLiteral("artist")).toString();
                    ans.insert(QStringLiteral("title"), title);
                    ans.insert(QStringLiteral("artist"), artist);
                    if (replySummary.isEmpty()) {
                        replySummary = artist.isEmpty() ? QStringLiteral("播放音频: %1").arg(title)
                                                        : QStringLiteral("播放: %1 - %2").arg(title, artist);
                    }
                }
            } else if (type == QStringLiteral("ALERT") || aObj.contains(QStringLiteral("alert"))) {
                QJsonObject alertObj = aObj.value(QStringLiteral("alert")).toObject();
                const QString dtStr = alertObj.value(QStringLiteral("datetime")).toString();
                ans.insert(QStringLiteral("datetime"), dtStr);
                if (replySummary.isEmpty()) replySummary = QStringLiteral("设置提醒: %1").arg(dtStr);
            } else if (type == QStringLiteral("ERROR") || aObj.contains(QStringLiteral("error"))) {
                const QString errText = aObj.value(QStringLiteral("error")).toObject().value(QStringLiteral("text")).toString();
                ans.insert(QStringLiteral("errorText"), errText);
                if (replySummary.isEmpty()) replySummary = errText;
            }
            answersList.append(ans);
        }

        // 若无任何回复文本/音频，保持 replySummary 为空，不显示占位气泡
        const bool hasReply = !replySummary.trimmed().isEmpty() || !answersList.isEmpty();

        item.insert(QStringLiteral("answers"), answersList);
        item.insert(QStringLiteral("replySummary"), replySummary);
        item.insert(QStringLiteral("hasReply"), hasReply);
        recordList.append(item);
    }

    if (globalState()) {
        globalState()->setXiaoaiDialogueNextTimestamp(earliestTs);
        globalState()->setXiaoaiDialogueRecords(recordList, m_dialogueAppend);
        if (recordList.isEmpty() && !m_dialogueAppend) {
            globalState()->setXiaoaiDialogueError(tr("当前设备暂无对话记录"));
        } else {
            globalState()->setXiaoaiDialogueError(QString());
        }
    }
}

void IntercomModule::notifyMiLinkEnable(const QString &uid)
{
    const QString token = !m_config.micoapiServiceToken.isEmpty() ? m_config.micoapiServiceToken : m_config.serviceToken;
    if (token.isEmpty() || uid.trimmed().isEmpty()) {
        return;
    }
    qInfo() << "[IntercomModule] Sending notifyMiLinkEnable for uid:" << uid;

    const QList<QUrl> urls = {
        QUrl(QStringLiteral("https://voip.mina.xiaoaisound.com/remote/voip/milink_enable"))
    };

    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2")
                               .arg(m_config.userId)
                               .arg(token);

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("uid"), uid.trimmed());
    const QByteArray body = params.toString(QUrl::FullyEncoded).toUtf8();

    for (const auto &url : urls) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
        request.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");
        request.setRawHeader("Cookie", cookie.toUtf8());

        QNetworkReply *reply = m_networkManager.post(request, body);
        connect(reply, &QNetworkReply::finished, this, [reply, url]() {
            reply->deleteLater();
            qInfo() << "[IntercomModule] notifyMiLinkEnable (" << url.host() << ") response:"
                    << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                    << reply->readAll();
        });
    }
}

void IntercomModule::startVoipCall(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar,
                                   const QString &uid, const QString &deviceModel, int deviceType,
                                   const QString &did)
{
    startVoipSession(false, targetDeviceId, targetDeviceName, avatar, uid, deviceModel, deviceType, did);
}

void IntercomModule::startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar,
                                         const QString &uid, const QString &deviceModel, int deviceType,
                                         const QString &did)
{
    startVoipSession(true, targetDeviceId, targetDeviceName, avatar, uid, deviceModel, deviceType, did);
}

void IntercomModule::startVoipSession(bool directIntercom, const QString &targetDeviceId, const QString &targetDeviceName,
                                      const QString &avatar, const QString &uid, const QString &deviceModel,
                                      int deviceType, const QString &did)
{
    QString resolvedDid = did.trimmed();
    if (resolvedDid.isEmpty() && globalState()) {
        const QVariantList speakers = globalState()->voipSpeakers();
        for (const QVariant &spVar : speakers) {
            const QVariantMap sp = spVar.toMap();
            if (sp.value(QStringLiteral("deviceId")).toString() == targetDeviceId ||
                sp.value(QStringLiteral("uid")).toString() == uid) {
                resolvedDid = sp.value(QStringLiteral("did")).toString();
                break;
            }
        }
    }

    qInfo() << "[IntercomModule]" << (directIntercom ? "Starting Direct Intercom to" : "Starting VoIP call to")
            << targetDeviceName << targetDeviceId
            << "uid=" << uid << "model=" << deviceModel << "type=" << deviceType << "did=" << resolvedDid;

    if (globalState()) {
        globalState()->setVoipTarget(targetDeviceId, targetDeviceName, avatar);
        globalState()->setVoipState(1, directIntercom ? tr("正在连接免接听对讲...") : tr("呼叫中..."));
        globalState()->setVoipDurationSeconds(0);
        globalState()->setVoipMuted(false);
        globalState()->setVoipSpeakerphone(true);
        globalState()->setVoipActive(true);
    }

    if ((!m_voipBackend || !m_voipBackend->isReady()) &&
        (!m_config.micoapiServiceToken.isEmpty() || !m_config.serviceToken.isEmpty())) {
        initVoipRuntime();
    }

    notifyMiLinkEnable(uid);
    syncBackendExtraTokens();

    if (m_voipBackend) {
        if (directIntercom) {
            m_voipBackend->startDirectIntercom(targetDeviceId, targetDeviceName, uid, deviceModel, deviceType, resolvedDid);
        } else {
            m_voipBackend->startCall(targetDeviceId, targetDeviceName, uid, deviceModel, deviceType, resolvedDid);
        }
    }
}

void IntercomModule::hangupVoipCall()
{
    qInfo() << "[IntercomModule] Hanging up VoIP call";
    if (m_voipBackend) {
        m_voipBackend->hangupCall();
    }
}

void IntercomModule::setVoipMute(bool mute)
{
    if (globalState()) {
        globalState()->setVoipMuted(mute);
    }
    if (m_voipBackend) {
        m_voipBackend->setMute(mute);
    }
}

void IntercomModule::setVoipSpeakerphone(bool enable)
{
    if (globalState()) {
        globalState()->setVoipSpeakerphone(enable);
    }
    if (m_voipBackend) {
        m_voipBackend->setSpeakerphone(enable);
    }
}

void IntercomModule::onVoipStateChanged(int state, const QString &statusText)
{
    qInfo() << "[IntercomModule] onVoipStateChanged:" << state << statusText;
    if (globalState()) {
        globalState()->setVoipState(state, statusText);
    }
}

void IntercomModule::onVoipDuration(int durationSec)
{
    if (globalState()) {
        globalState()->setVoipDurationSeconds(durationSec);
    }
}

void IntercomModule::onVoipAudioRoute(bool isSpeaker)
{
    if (globalState()) {
        globalState()->setVoipSpeakerphone(isSpeaker);
    }
}

QString IntercomModule::voipBackendType() const
{
    return m_activeVoipBackendName;
}

void IntercomModule::setVoipBackendType(const QString &backendType)
{
    const QString targetName = VoipBackendFactory::resolveBackendName(backendType);
    qInfo() << "[IntercomModule] setVoipBackendType requested=" << backendType << "resolved=" << targetName;

    if (globalState() && globalState()->voipActive()) {
        qWarning() << "[IntercomModule] Cannot switch VoIP backend while a call is active";
        return;
    }

    const bool switched = ensureVoipBackend(targetName);
    if (m_configManager) {
        m_configManager->updateIntercomBackend(targetName);
    }

    if (switched) {
        initVoipRuntime();
    }
}

bool IntercomModule::ensureVoipBackend(const QString &preferredBackend)
{
    const QString targetName = VoipBackendFactory::resolveBackendName(preferredBackend);
    if (m_voipBackend && m_activeVoipBackendName == targetName) {
        if (globalState()) {
            globalState()->setVoipBackendType(m_activeVoipBackendName);
        }
        return false;
    }

    if (m_voipBackend) {
        m_voipBackend->disconnect(this);
        m_voipBackend->deleteLater();
        m_voipBackend = nullptr;
    }

    m_voipBackend = VoipBackendFactory::createBackend(targetName, this);
    m_activeVoipBackendName = targetName;
    qInfo() << "[IntercomModule] Active VoIP backend switched to:" << m_activeVoipBackendName;

    if (m_voipBackend) {
        connect(m_voipBackend, &IVoipBackend::stateChanged, this, &IntercomModule::onVoipStateChanged);
        connect(m_voipBackend, &IVoipBackend::durationChanged, this, &IntercomModule::onVoipDuration);
        connect(m_voipBackend, &IVoipBackend::audioRouteChanged, this, &IntercomModule::onVoipAudioRoute);
        if (auto *embedded = qobject_cast<EmbeddedVoipBackend *>(m_voipBackend)) {
            connect(embedded, &EmbeddedVoipBackend::micoapiAuthInvalid, this, [this]() {
                qWarning() << "[IntercomModule] Received micoapiAuthInvalid from backend (ret=5000/5003)!";
                if (!m_retriedMicoapi5003 && !m_config.userId.trimmed().isEmpty() && !m_config.passToken.trimmed().isEmpty()) {
                    m_retriedMicoapi5003 = true;
                    qInfo() << "[IntercomModule] Attempting automatic passToken refresh for sid=micoapi...";
                    exchangeTokenWithPassToken(QStringLiteral("micoapi"));
                } else {
                    if (globalState()) {
                        globalState()->setIntercomStatus(tr("通话凭证已失效 (ret=5000/5003)，请扫码登录或重新输入"));
                    }
                }
            });
        }
    }

    if (globalState()) {
        globalState()->setVoipBackendType(m_activeVoipBackendName);
    }
    syncBackendExtraTokens();
    return true;
}

void IntercomModule::syncBackendExtraTokens()
{
    if (auto *embedded = qobject_cast<EmbeddedVoipBackend *>(m_voipBackend)) {
        embedded->setMinaServiceToken(m_config.serviceToken);
    }
}
