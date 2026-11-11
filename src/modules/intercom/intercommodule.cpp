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
    m_networkManager.setCookieJar(new QNetworkCookieJar(this));

    m_voipBackend = VoipBackendFactory::createBackend(this);
    if (m_voipBackend) {
        connect(m_voipBackend, &IVoipBackend::stateChanged, this, &IntercomModule::onVoipStateChanged);
        connect(m_voipBackend, &IVoipBackend::durationChanged, this, &IntercomModule::onVoipDuration);
        connect(m_voipBackend, &IVoipBackend::audioRouteChanged, this, &IntercomModule::onVoipAudioRoute);
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
    ensureDeviceId();

    if (globalState()) {
        globalState()->setIntercomAvailable(m_config.enabled);
        globalState()->setIntercomUserId(m_config.userId);
        globalState()->setIntercomDeviceId(m_config.deviceId);
        globalState()->setIntercomServiceToken(m_config.serviceToken);
        globalState()->setIntercomHomeId(m_config.homeId);
        globalState()->setMicoapiServiceToken(m_config.micoapiServiceToken);
        globalState()->setMimcId(m_config.mimcId);

        if (!m_config.serviceToken.isEmpty() || !m_config.micoapiServiceToken.isEmpty()) {
            globalState()->setIntercomStatus(tr("已登录账号: %1").arg(m_config.userId));
        } else {
            globalState()->setIntercomStatus(tr("未登录"));
        }
    }
}

void IntercomModule::start()
{
    initDefaultRooms();

#if defined(Q_OS_ANDROID)
    if (m_config.enabled && (m_config.serviceToken.trimmed().isEmpty() || m_config.serviceToken.trimmed().length() < 30 || m_config.micoapiServiceToken.trimmed().isEmpty())) {
        if (importLocalXiaoAiToken()) {
            return;
        }
    }
#endif

    if (m_config.enabled) {
        if (!m_config.serviceToken.isEmpty()) {
            fetchRooms();
        }
        if (!m_config.micoapiServiceToken.isEmpty()) {
            initVoipRuntime();
            fetchVoipContacts();
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
#if defined(Q_OS_ANDROID)
        // 在 Android 上尝试从系统小爱读取真实的 device_id
        QFile didFile(QStringLiteral("/data/data/com.miui.voiceassist/shared_prefs/aivs_did.xml.xml"));
        if (didFile.open(QIODevice::ReadOnly)) {
            const QString content = QString::fromUtf8(didFile.readAll());
            QRegularExpression didRe(QStringLiteral("<string name=\"device_id\">([^<]+)</string>"));
            auto match = didRe.match(content);
            if (match.hasMatch()) {
                m_config.deviceId = match.captured(1).trimmed();
                return;
            }
        }
#endif
        // 生成 16 位的标准小爱/Android deviceId 伪随机十六进制
        const QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_config.deviceId = QCryptographicHash::hash(uuidStr.toUtf8(), QCryptographicHash::Md5).toHex().left(16).toLower();
    }
}

QString IntercomModule::buildAuthHeader() const
{
    if (m_config.serviceToken.trimmed().isEmpty()) {
        return QString();
    }
    if (m_config.serviceToken.startsWith(QStringLiteral("DO-TOKEN-V1 "))) {
        return m_config.serviceToken.trimmed();
    }

    const QString devId = m_config.deviceId.trimmed().isEmpty()
        ? QStringLiteral("837a4f95cd5a32ed")
        : m_config.deviceId.trimmed();

    const QByteArray scopeJson = QStringLiteral("{\"d\":\"%1\"}").arg(devId).toUtf8();
    const QByteArray scopeData = scopeJson.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    return QStringLiteral("DO-TOKEN-V1 app_id:%1,scope_data:%2,access_token:%3")
        .arg(QStringLiteral("326813440150602752"), QString::fromUtf8(scopeData), m_config.serviceToken.trimmed());
}

bool IntercomModule::importLocalXiaoAiToken()
{
    qInfo() << "[IntercomModule] Attempting to import token from system XiaoAi & Mico...";
    QString devId;
    QString accessToken;
    QString refreshToken;
    QString micoapiToken;
    QString mimcId;
    QString userId;

#if defined(Q_OS_ANDROID) || defined(Q_OS_LINUX)
    // 优先尝试使用 su 读取小爱内部存储配置与小爱音箱 App 配置
    QString suBinary = QStringLiteral("su");
    for (const QString &cand : {QStringLiteral("/system/bin/su"), QStringLiteral("/sbin/su"), QStringLiteral("/system/xbin/su"), QStringLiteral("su")}) {
        if (QFile::exists(cand)) {
            suBinary = cand;
            break;
        }
    }

    QProcess process;
    process.start(suBinary, QStringList() << QStringLiteral("-c")
        << QStringLiteral("cat /data/data/com.miui.voiceassist/shared_prefs/aivs_did.xml.xml 2>/dev/null; "
                          "echo '---DIVIDER---'; "
                          "cat /data/data/com.miui.voiceassist/shared_prefs/aivs_user_data.xml.xml 2>/dev/null; "
                          "echo '---DIVIDER---'; "
                          "cat /data/data/com.xiaomi.mico/shared_prefs/Account.xml 2>/dev/null; "
                          "echo '---DIVIDER---'; "
                          "cat /data/data/com.xiaomi.mico/shared_prefs/com.xiaomi.mico_preferences.xml 2>/dev/null"));
    if (process.waitForFinished(4000)) {
        const QString output = QString::fromUtf8(process.readAllStandardOutput());
        const QStringList parts = output.split(QStringLiteral("---DIVIDER---"));

        if (!parts.isEmpty()) {
            QRegularExpression didRe(QStringLiteral("<string name=\"device_id\">([^<]+)</string>"));
            auto matchDid = didRe.match(parts.first());
            if (matchDid.hasMatch()) {
                devId = matchDid.captured(1).trimmed();
            }
        }

        if (parts.size() > 1) {
            const QString userData = parts.at(1);
            QRegularExpression tokenRe(QStringLiteral("<string name=\"production_DO-TOKEN-V1_access_token\">([^<]+)</string>"));
            auto matchToken = tokenRe.match(userData);
            if (matchToken.hasMatch()) {
                accessToken = matchToken.captured(1).trimmed();
            }

            QRegularExpression refreshRe(QStringLiteral("<string name=\"production_DO-TOKEN-V1_refresh_token\">([^<]+)</string>"));
            auto matchRefresh = refreshRe.match(userData);
            if (matchRefresh.hasMatch()) {
                refreshToken = matchRefresh.captured(1).trimmed();
            }
        }

        if (parts.size() > 2) {
            const QString accountXml = parts.at(2);
            QRegularExpression hexRe(QStringLiteral("<string name=\"encoded_account\">(.*?)</string>"));
            auto matchHex = hexRe.match(accountXml);
            if (matchHex.hasMatch()) {
                const QByteArray raw = QByteArray::fromHex(matchHex.captured(1).toLatin1());
                const int idx = raw.indexOf("micoapi");
                if (idx != -1) {
                    const QByteArray after = raw.mid(idx);
                    int pos = 0;
                    while ((pos = after.indexOf('\x74', pos)) != -1) {
                        if (pos + 3 <= after.size()) {
                            const quint16 slen = (static_cast<quint8>(after.at(pos + 1)) << 8) | static_cast<quint8>(after.at(pos + 2));
                            if (slen > 50 && pos + 3 + slen <= after.size()) {
                                micoapiToken = QString::fromUtf8(after.mid(pos + 3, slen)).trimmed();
                                break;
                            }
                        }
                        pos++;
                    }
                }

                // 从序列化数据提取 userId
                QRegularExpression uidRe(QStringLiteral("userIdt\\x00\\x07(\\d+)"));
                auto matchUid = uidRe.match(QString::fromLatin1(raw));
                if (matchUid.hasMatch()) {
                    userId = matchUid.captured(1).trimmed();
                }
            }
        }

        if (parts.size() > 3) {
            const QString prefXml = parts.at(3);
            QRegularExpression mimcRe(QStringLiteral("<string name=\"mimc_id\\d*\">(\\d+)</string>"));
            auto matchMimc = mimcRe.match(prefXml);
            if (matchMimc.hasMatch()) {
                mimcId = matchMimc.captured(1).trimmed();
            }
        }
    }

    // 若通过 root 读取未成功，尝试直接 QFile 读取
    if (accessToken.isEmpty()) {
        QFile didFile(QStringLiteral("/data/data/com.miui.voiceassist/shared_prefs/aivs_did.xml.xml"));
        if (didFile.open(QIODevice::ReadOnly)) {
            const QString content = QString::fromUtf8(didFile.readAll());
            QRegularExpression didRe(QStringLiteral("<string name=\"device_id\">([^<]+)</string>"));
            auto match = didRe.match(content);
            if (match.hasMatch()) {
                devId = match.captured(1).trimmed();
            }
        }

        QFile userFile(QStringLiteral("/data/data/com.miui.voiceassist/shared_prefs/aivs_user_data.xml.xml"));
        if (userFile.open(QIODevice::ReadOnly)) {
            const QString content = QString::fromUtf8(userFile.readAll());
            QRegularExpression tokenRe(QStringLiteral("<string name=\"production_DO-TOKEN-V1_access_token\">([^<]+)</string>"));
            auto matchToken = tokenRe.match(content);
            if (matchToken.hasMatch()) {
                accessToken = matchToken.captured(1).trimmed();
            }
            QRegularExpression refreshRe(QStringLiteral("<string name=\"production_DO-TOKEN-V1_refresh_token\">([^<]+)</string>"));
            auto matchRefresh = refreshRe.match(content);
            if (matchRefresh.hasMatch()) {
                refreshToken = matchRefresh.captured(1).trimmed();
            }
        }
    }
#endif

    const bool importedAny = (!accessToken.isEmpty() || !micoapiToken.isEmpty());
    if (importedAny) {
        qInfo() << "[IntercomModule] Successfully imported tokens! userId:" << userId
                << "micoapiToken length:" << micoapiToken.length()
                << "mimcId:" << mimcId
                << "accessToken length:" << accessToken.length();

        if (!devId.isEmpty()) {
            m_config.deviceId = devId;
        }
        if (!userId.isEmpty()) {
            m_config.userId = userId;
        }
        if (!accessToken.isEmpty()) {
            m_config.serviceToken = accessToken;
        }
        if (!refreshToken.isEmpty()) {
            m_config.refreshToken = refreshToken;
        }
        if (!micoapiToken.isEmpty()) {
            m_config.micoapiServiceToken = micoapiToken;
        }
        if (!mimcId.isEmpty()) {
            m_config.mimcId = mimcId;
        }

        if (m_configManager) {
            m_configManager->updateIntercomConfig(
                m_config.enabled,
                m_config.userId,
                m_config.serviceToken,
                m_config.deviceId,
                m_config.homeId,
                m_config.refreshToken,
                m_config.micoapiServiceToken,
                m_config.mimcId
            );
        }
        if (globalState()) {
            globalState()->setIntercomDeviceId(m_config.deviceId);
            globalState()->setIntercomUserId(m_config.userId);
            if (!m_config.serviceToken.isEmpty()) {
                globalState()->setIntercomServiceToken(m_config.serviceToken);
            }
            globalState()->setMicoapiServiceToken(m_config.micoapiServiceToken);
            globalState()->setMimcId(m_config.mimcId);
            globalState()->setIntercomStatus(tr("已从系统小爱导入凭证"));
        }

        if (!m_config.serviceToken.isEmpty()) {
            fetchRooms();
        }
        if (!m_config.micoapiServiceToken.isEmpty()) {
            initVoipRuntime();
            fetchVoipContacts();
        }

        emit loginResult(true, tr("已成功从系统小爱及音箱导入凭证！"));
        return true;
    }

    qWarning() << "[IntercomModule] Could not import XiaoAi token from system.";
    if (globalState()) {
        globalState()->setIntercomStatus(tr("未检测到系统小爱凭证或权限不足"));
    }
    emit loginResult(false, tr("未检测到系统小爱凭证或权限不足，请使用扫码/浏览器登录"));
    return false;
}

void IntercomModule::refreshOAuthToken()
{
    if (m_config.refreshToken.trimmed().isEmpty()) {
        qWarning() << "[IntercomModule] No refreshToken available for refresh.";
        return;
    }

    qInfo() << "[IntercomModule] Refreshing XiaoAi OAuth token...";
    if (globalState()) {
        globalState()->setIntercomStatus(tr("正在刷新小爱授权凭证..."));
    }

    const QUrl url(QStringLiteral("https://account.xiaomi.com/oauth2/auth/token?pt=1&client_id=326813440150602752&client_secret=dWbMUXRKF6IACwDtvaXO-cOJQlnyQSc34chxCKudAdANCWrCtXdR5FYQjObaIZvXfUfDoYNm3-6gpAKL0aaxQg&redirect_uri=https%3A%2F%2Fi.xiaomixiaoai.com&grant_type=refresh_token&refresh_token=%1")
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
        qWarning() << "[IntercomModule] Refresh token network error:" << reply->errorString();
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

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

        if (m_configManager) {
            m_configManager->updateIntercomConfig(m_config.enabled, m_config.userId, m_config.serviceToken, m_config.deviceId, m_config.homeId, m_config.refreshToken);
        }
        if (globalState()) {
            globalState()->setIntercomServiceToken(m_config.serviceToken);
            globalState()->setIntercomDeviceId(m_config.deviceId);
            globalState()->setIntercomStatus(tr("凭证已自动刷新"));
        }

        fetchRooms();
    }
}

void IntercomModule::initDefaultRooms()
{
    QVariantList defaultList;
    QVariantMap wholeHouse;
    wholeHouse.insert(QStringLiteral("roomId"), QStringLiteral("WHOLE_0"));
    wholeHouse.insert(QStringLiteral("roomName"), tr("全屋广播"));
    wholeHouse.insert(QStringLiteral("icon"), QStringLiteral("📢"));
    wholeHouse.insert(QStringLiteral("isWholeHouse"), true);
    defaultList.append(wholeHouse);

    if (globalState() && globalState()->intercomRooms().isEmpty()) {
        globalState()->setIntercomRooms(defaultList);
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

    if (globalState()) {
        globalState()->setIntercomRecording(false);
    }

    if (durationMs < 500) {
        emit sendAudioResult(false, tr("说话时间太短，未发送"));
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
        emit sendAudioResult(false, tr("无有效音频数据"));
        return;
    }

    if (m_config.serviceToken.trimmed().isEmpty()) {
        emit sendAudioResult(false, tr("未配置小爱 serviceToken，无法发送"));
        return;
    }

    const QString targetRoomId = roomId.isEmpty() ? QStringLiteral("WHOLE_0") : roomId;
    const QString targetRoomName = roomName.isEmpty() ? tr("全屋广播") : roomName;
    const QString targetHomeId = m_config.homeId.trimmed();
    const QString deviceId = m_config.deviceId.trimmed();

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
        msgObj.insert(QStringLiteral("caller_home_id"), targetHomeId);
        msgObj.insert(QStringLiteral("to_home_id"), targetHomeId);
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

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = tr("发送失败: %1").arg(reply->errorString());
        qWarning() << "[IntercomModule] Upload audio network error:" << err;
        emit sendAudioResult(false, err);
        return;
    }

    const QByteArray responseData = reply->readAll();
    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        emit sendAudioResult(false, tr("服务器响应异常"));
        return;
    }

    const QJsonObject root = doc.object();
    const int code = root.value(QStringLiteral("code")).toInt(-1);
    if (code == 200 || code == 0) {
        emit sendAudioResult(true, tr("对讲语音发送成功"));
    } else {
        const QString msg = root.value(QStringLiteral("message")).toString(tr("错误码: %1").arg(code));
        emit sendAudioResult(false, tr("发送未成功: %1").arg(msg));
    }
}

void IntercomModule::saveCredentials(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &refreshToken)
{
    m_config.userId = userId.trimmed();
    m_config.serviceToken = serviceToken.trimmed();
    if (!deviceId.trimmed().isEmpty()) {
        m_config.deviceId = deviceId.trimmed();
    }
    if (!homeId.trimmed().isEmpty()) {
        m_config.homeId = homeId.trimmed();
    }
    if (!refreshToken.trimmed().isEmpty()) {
        m_config.refreshToken = refreshToken.trimmed();
    }

    if (m_configManager) {
        m_configManager->updateIntercomConfig(m_config.enabled, m_config.userId, m_config.serviceToken, m_config.deviceId, m_config.homeId, m_config.refreshToken);
    }

    if (globalState()) {
        globalState()->setIntercomUserId(m_config.userId);
        globalState()->setIntercomServiceToken(m_config.serviceToken);
        globalState()->setIntercomDeviceId(m_config.deviceId);
        globalState()->setIntercomHomeId(m_config.homeId);
        globalState()->setIntercomStatus(tr("已更新 Token，账号: %1").arg(m_config.userId));
    }

    fetchRooms();
}

void IntercomModule::login(const QString &username, const QString &password)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        emit loginResult(false, tr("用户名和密码不能为空"));
        return;
    }

    m_loginUser = username.trimmed();
    m_loginPwdHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex().toUpper();

    // 步骤 1: 请求 serviceLogin 获得 _sign, qs, callback
    const QUrl url(QStringLiteral("https://account.xiaomi.com/pass/serviceLogin?sid=ai-service&_json=true"));
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
    query.addQueryItem(QStringLiteral("sid"), QStringLiteral("ai-service"));
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
    qInfo() << "[IntercomModule] QR poll reply code:" << code;

    if (code == 70016) {
        emit loginResult(false, tr("账号或密码错误"));
        return;
    }
    if (code == 87001 || obj.contains(QStringLiteral("notificationUrl"))) {
        emit loginResult(false, tr("该账号开启了双重验证或需要图形验证，请直接在设置面板中填入 serviceToken"));
        return;
    }

    const QString userId = QString::number(obj.value(QStringLiteral("userId")).toVariant().toLongLong());
    const QString serviceToken = obj.value(QStringLiteral("serviceToken")).toString();
    const QString location = obj.value(QStringLiteral("location")).toString();

    if (!serviceToken.isEmpty() && !userId.isEmpty()) {
        saveCredentials(userId, serviceToken, m_config.deviceId, m_config.homeId);
        emit loginResult(true, tr("登录成功！已获取对讲凭证"));
        return;
    }

    if (!location.isEmpty()) {
        // 请求重定向 location 获取最终 Set-Cookie
        const QUrl locUrl(location);
        QNetworkRequest locReq(locUrl);
        locReq.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");
        m_currentReply = m_networkManager.get(locReq);
        m_config.userId = userId;
        connect(m_currentReply, &QNetworkReply::finished, this, &IntercomModule::handleLoginLocationReply);
        return;
    }

    emit loginResult(false, tr("未能获取有效 Token: %1").arg(obj.value(QStringLiteral("description")).toString()));
}

QString IntercomModule::extractServiceToken(QNetworkReply *reply)
{
    if (!reply) return QString();

    // 1. 从 Set-Cookie 头提取
    const QVariant cookiesVariant = reply->header(QNetworkRequest::SetCookieHeader);
    const QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie>>(cookiesVariant);
    for (const QNetworkCookie &c : cookies) {
        if (c.name() == "serviceToken") {
            return QString::fromUtf8(c.value());
        }
    }

    // 2. 从 RawHeaderPairs 提取
    for (const auto &pair : reply->rawHeaderPairs()) {
        if (pair.first.compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
            const auto parsedCookies = QNetworkCookie::parseCookies(pair.second);
            for (const auto &c : parsedCookies) {
                if (c.name() == "serviceToken") {
                    return QString::fromUtf8(c.value());
                }
            }
        }
    }

    // 3. 从 CookieJar 提取
    if (m_networkManager.cookieJar()) {
        const QList<QUrl> checkUrls = {
            reply->url(),
            QUrl(QStringLiteral("https://api.mina.mi.com")),
            QUrl(QStringLiteral("https://account.xiaomi.com")),
            QUrl(QStringLiteral("https://mi.com"))
        };
        for (const auto &u : checkUrls) {
            const auto jarCookies = m_networkManager.cookieJar()->cookiesForUrl(u);
            for (const auto &c : jarCookies) {
                if (c.name() == "serviceToken") {
                    return QString::fromUtf8(c.value());
                }
            }
        }
    }

    // 4. 从 URL 查询参数提取（若存在重定向附带）
    const QUrlQuery query(reply->url());
    if (query.hasQueryItem(QStringLiteral("serviceToken"))) {
        return query.queryItemValue(QStringLiteral("serviceToken"));
    }

    return QString();
}

void IntercomModule::handleLoginLocationReply()
{
    if (!m_currentReply) return;
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    const QString token = extractServiceToken(reply);
    if (!token.isEmpty()) {
        saveCredentials(m_config.userId, token, m_config.deviceId, m_config.homeId);
        emit loginResult(true, tr("登录成功！已获取 Token 并保存"));
    } else {
        emit loginResult(false, tr("获取 serviceToken 失败，请检查账号状态或直接手动填写"));
    }
}

void IntercomModule::startQrLogin()
{
    cancelQrLogin();

    qInfo() << "[IntercomModule] Starting QR and web login flow...";
    m_qrLoggingIn = true;
    m_qrImageUrl.clear();
    m_qrWebUrl.clear();
    m_qrLpUrl.clear();
    m_qrCandidateUserId.clear();
    m_qrStartTimeSec = QDateTime::currentSecsSinceEpoch();
    m_qrTimeoutSec = 300;

    if (globalState()) {
        globalState()->setIntercomQrLoggingIn(true);
        globalState()->setIntercomQrImageUrl(QString());
        globalState()->setIntercomQrWebUrl(QString());
        globalState()->setIntercomQrLoginStatus(tr("正在获取登录二维码与网页地址..."));
    }

    const QUrl url(QStringLiteral("https://account.xiaomi.com/longPolling/loginUrl?_qrsize=480&sid=ai-service&_locale=zh_CN"));
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

    if (code == 0 && (!userId.isEmpty() || !location.isEmpty() || !directToken.isEmpty())) {
        m_qrCandidateUserId = userId;
        if (!directToken.isEmpty()) {
            saveCredentials(m_qrCandidateUserId, directToken, m_config.deviceId, m_config.homeId);
            m_qrLoggingIn = false;
            if (globalState()) {
                globalState()->setIntercomQrLoggingIn(false);
                globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
            }
            emit loginResult(true, tr("登录成功！已获取对讲凭证"));
            return;
        }

        if (!location.isEmpty()) {
            if (globalState()) {
                globalState()->setIntercomQrLoginStatus(tr("已授权，正在获取通行凭证..."));
            }
            const QUrl locUrl(location);
            QNetworkRequest locReq(locUrl);
            locReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            locReq.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

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

    const QString token = extractServiceToken(reply);
    m_qrLoggingIn = false;
    qInfo() << "[IntercomModule] QR location reply, token extracted:" << !token.isEmpty();

    if (!token.isEmpty()) {
        const QString finalUserId = !m_qrCandidateUserId.isEmpty() ? m_qrCandidateUserId : m_config.userId;
        saveCredentials(finalUserId, token, m_config.deviceId, m_config.homeId);
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("登录成功！"));
        }
        emit loginResult(true, tr("登录成功！已获取对讲凭证并保存"));
    } else {
        if (globalState()) {
            globalState()->setIntercomQrLoggingIn(false);
            globalState()->setIntercomQrLoginStatus(tr("未能提取到 serviceToken，请重试或手动输入"));
        }
        emit loginResult(false, tr("未能提取到 serviceToken，请重试或手动输入"));
    }
}

void IntercomModule::fetchRooms()
{
    qInfo() << "[IntercomModule::fetchRooms] Called. serviceToken length:" << m_config.serviceToken.trimmed().length()
            << "homeId:" << m_config.homeId << "deviceId:" << m_config.deviceId;

    if (m_config.serviceToken.trimmed().isEmpty()) {
#if defined(Q_OS_ANDROID)
        if (importLocalXiaoAiToken()) {
            return;
        }
#endif
        initDefaultRooms();
        return;
    }

    ensureDeviceId();

    const QString authHeader = buildAuthHeader();

    // 如果还没有 homeId，先拉取家庭成员获取默认 homeId
    if (m_config.homeId.trimmed().isEmpty()) {
        const QUrl memberUrl(QStringLiteral("https://i.xiaomixiaoai.com/miot/intercom/get_home_members?device_id=%1&reqId=%2")
            .arg(m_config.deviceId, QUuid::createUuid().toString(QUuid::WithoutBraces)));
        qInfo() << "[IntercomModule::fetchRooms] Querying home members from:" << memberUrl.toString();
        QNetworkRequest req(memberUrl);
        if (!authHeader.isEmpty()) {
            req.setRawHeader("Authorization", authHeader.toUtf8());
        }
        const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(m_config.serviceToken, m_config.userId);
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
    const QString cookieHeader = QStringLiteral("serviceToken=%1; userId=%2").arg(m_config.serviceToken, m_config.userId);
    req.setRawHeader("Cookie", cookieHeader.toUtf8());
    req.setRawHeader("User-Agent", "MIAI/7.13.34 Android/14");

    QNetworkReply *reply = m_networkManager.get(req);
    connect(reply, &QNetworkReply::finished, this, &IntercomModule::handleFetchRoomsReply);
}

void IntercomModule::handleFetchHomeMembersReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qInfo() << "[IntercomModule::handleFetchHomeMembersReply] statusCode:" << statusCode << "error:" << reply->errorString();
    if (statusCode == 401) {
        qWarning() << "[IntercomModule] Home members request unauthorized (401), attempting token refresh...";
        if (!m_config.refreshToken.isEmpty()) {
            refreshOAuthToken();
            return;
        }
#if defined(Q_OS_ANDROID)
        if (importLocalXiaoAiToken()) {
            return;
        }
#endif
    }

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray payload = reply->readAll();
        qInfo() << "[IntercomModule::handleFetchHomeMembersReply] payload:" << payload;
        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject()) {
            const QJsonObject root = doc.object();
            const QJsonArray homes = root.value(QStringLiteral("data")).toArray();
            if (!homes.isEmpty()) {
                const QJsonObject firstHome = homes.first().toObject();
                QString fetchedHomeId;
                const QJsonValue homeIdVal = firstHome.contains(QStringLiteral("homeId"))
                    ? firstHome.value(QStringLiteral("homeId"))
                    : firstHome.value(QStringLiteral("home_id"));

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
        qWarning() << "[IntercomModule] Fetch rooms unauthorized (401), attempting token refresh...";
        if (!m_config.refreshToken.isEmpty()) {
            refreshOAuthToken();
            return;
        }
#if defined(Q_OS_ANDROID)
        if (importLocalXiaoAiToken()) {
            return;
        }
#endif
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
        const QJsonObject data = root.value(QStringLiteral("data")).toObject();
        const QJsonArray rooms = data.value(QStringLiteral("rooms")).toArray();

        for (const auto &item : rooms) {
            const QJsonObject roomObj = item.toObject();
            const QString roomId = roomObj.value(QStringLiteral("room_id")).toString();
            const QString roomName = roomObj.value(QStringLiteral("room_name")).toString();

            if (!roomId.isEmpty() && roomId != QStringLiteral("WHOLE_0")) {
                QVariantMap rm;
                rm.insert(QStringLiteral("roomId"), roomId);
                rm.insert(QStringLiteral("roomName"), roomName);
                rm.insert(QStringLiteral("isWholeHouse"), false);

                // 根据房间名称匹配贴合的 Emoji 图标
                if (roomName.contains(tr("卧"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🛏️"));
                } else if (roomName.contains(tr("门")) || roomName.contains(tr("玄关"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🚪"));
                } else if (roomName.contains(tr("厅"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🛋️"));
                } else if (roomName.contains(tr("厨"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🍳"));
                } else if (roomName.contains(tr("阳台"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🪴"));
                } else if (roomName.contains(tr("卫")) || roomName.contains(tr("洗"))) {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🚿"));
                } else {
                    rm.insert(QStringLiteral("icon"), QStringLiteral("🏠"));
                }
                list.append(rm);
            }
        }
    }

    if (globalState()) {
        globalState()->setIntercomRooms(list);
        globalState()->setIntercomStatus(tr("获取房间成功，共 %1 个房间/设备").arg(list.count()));
    }
}

void IntercomModule::initVoipRuntime()
{
    if (m_voipBackend) {
        const QString token = !m_config.micoapiServiceToken.isEmpty() ? m_config.micoapiServiceToken : m_config.serviceToken;
        m_voipBackend->init(m_config.userId, token, m_config.mimcId);
    }
}

void IntercomModule::fetchVoipContacts()
{
    if (m_config.micoapiServiceToken.isEmpty()) {
        qWarning() << "[IntercomModule] Cannot fetch VoIP contacts: micoapiServiceToken is empty.";
        return;
    }

    qInfo() << "[IntercomModule] Fetching VoIP contacts...";
    QUrl url(QStringLiteral("https://voip.mina.xiaoaisound.com/voip/proxy"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json;charset=UTF-8"));
    request.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");

    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2")
                               .arg(m_config.userId)
                               .arg(m_config.micoapiServiceToken);
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
        qWarning() << "[IntercomModule] Fetch VoIP contacts network error:" << reply->errorString();
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
    }
}

void IntercomModule::notifyMiLinkEnable(const QString &uid)
{
    if (m_config.micoapiServiceToken.isEmpty() || uid.trimmed().isEmpty()) {
        return;
    }
    qInfo() << "[IntercomModule] Sending notifyMiLinkEnable for uid:" << uid;

    const QList<QUrl> urls = {
        QUrl(QStringLiteral("https://voip.mina.xiaoaisound.com/remote/voip/milink_enable")),
        QUrl(QStringLiteral("https://api2.mina.xiaoaisound.com/remote/voip/milink_enable"))
    };

    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2")
                               .arg(m_config.userId)
                               .arg(m_config.micoapiServiceToken);

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
                                   const QString &uid, const QString &deviceModel, int deviceType)
{
    qInfo() << "[IntercomModule] Starting VoIP call to" << targetDeviceName << targetDeviceId << "uid=" << uid << "model=" << deviceModel << "type=" << deviceType;

    if (globalState()) {
        globalState()->setVoipTarget(targetDeviceId, targetDeviceName, avatar);
        globalState()->setVoipState(1, tr("呼叫中..."));
        globalState()->setVoipDurationSeconds(0);
        globalState()->setVoipMuted(false);
        globalState()->setVoipSpeakerphone(true);
        globalState()->setVoipActive(true);
    }

    notifyMiLinkEnable(uid);

    if (m_voipBackend) {
        m_voipBackend->startCall(targetDeviceId, targetDeviceName, uid, deviceModel, deviceType);
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
