#include "embeddedvoipbackend.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>
#include <QSocketNotifier>
#include <QDebug>
#include <iostream>
#include <csignal>

namespace {
    EmbeddedVoipBackend *g_backend = nullptr;

    void signalHandler(int sig)
    {
        if (g_backend) {
            std::cout << "\n[voip_cli] Interrupt received, hanging up call...\n";
            g_backend->hangupCall();
        }
        QTimer::singleShot(500, []() {
            QCoreApplication::quit();
        });
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("voip_cli"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Xiaomi Smart Speaker VoIP & Intercom CLI Test Interface Program\n"
        "Commands:\n"
        "  call      Initiate full-duplex VoIP call (triggers physical ring/prompt '来电话了')\n"
        "  intercom  Direct auto-answer two-way intercom (hands-free auto-answer '好的，已接听')\n"
        "  hangup    Terminate active call or intercom session on remote speaker\n"
        "  status    Check remote speaker status and volume\n"
        "  tts       Broadcast text to speech directly on speaker\n"
        "  wakeup    Send cloud notifyMiLinkEnable wake-up trigger\n"
    ));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption userIdOpt(QStringList() << "u" << "user-id", "Xiaomi Account MID", "mid", "8406660");
    QCommandLineOption tokenOpt(QStringList() << "t" << "service-token", "Mina serviceToken", "token",
        "sXEJMjSdm6MHAaBJzdchB5JFcJiUm2/wZLvBpdKzt8l7Y6qihmYIdwcfwOnjiffxL0ChS9HoQ/+XG/IOJm1uF5d7HqQdXmxIpWoKCTdcST4vLuCgO8FlNqpduxa9EsDN+Egr4TTTQW6AVBkR18Ercohyod7qkc7dIMT1sx2t+GoTXNXugdApYbAbD16MssEFdnW14609K/8cziIVqYTQdw==");
    QCommandLineOption callerVuidOpt(QStringList() << "c" << "caller-vuid", "Caller VUID", "vuid", "674966491");
    QCommandLineOption targetVuidOpt(QStringList() << "target-vuid", "Target Speaker VUID", "vuid", "629654798");
    QCommandLineOption targetDidOpt(QStringList() << "target-did", "Target Speaker DID", "did", "866485066");
    QCommandLineOption targetModelOpt(QStringList() << "target-model", "Target Speaker Model", "model", "OH2P");
    QCommandLineOption targetNameOpt(QStringList() << "target-name", "Target Speaker Display Name", "name", "北卧室");
    QCommandLineOption targetDeviceIdOpt(QStringList() << "target-device-id", "Target Cloud Device ID", "id", "1959e5ad-0809-4c3c-abdf-f64f9a6ca2fa");
    QCommandLineOption durationOpt(QStringList() << "d" << "duration", "Call duration in seconds before auto-hangup", "sec", "30");
    QCommandLineOption msgOpt(QStringList() << "m" << "message", "Text message for TTS or intercom", "text", "收到对讲呼叫测试");

    parser.addOption(userIdOpt);
    parser.addOption(tokenOpt);
    parser.addOption(callerVuidOpt);
    parser.addOption(targetVuidOpt);
    parser.addOption(targetDidOpt);
    parser.addOption(targetModelOpt);
    parser.addOption(targetNameOpt);
    parser.addOption(targetDeviceIdOpt);
    parser.addOption(durationOpt);
    parser.addOption(msgOpt);
    parser.addPositionalArgument("command", "Subcommand: call, wakeup, intercom, tts, status");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    const QString cmd = args.isEmpty() ? QStringLiteral("call") : args.first();

    const QString userId = parser.value(userIdOpt);
    const QString serviceToken = parser.value(tokenOpt);
    const quint64 callerVuid = parser.value(callerVuidOpt).toULongLong();
    const QString targetVuid = parser.value(targetVuidOpt);
    const QString targetDid = parser.value(targetDidOpt);
    const QString targetModel = parser.value(targetModelOpt);
    const QString targetName = parser.value(targetNameOpt);
    const QString targetDeviceId = parser.value(targetDeviceIdOpt);
    const int durationSec = parser.value(durationOpt).toInt();
    const QString messageText = parser.value(msgOpt);

    std::cout << "========================================================\n";
    std::cout << "  Xiaomi Smart Speaker C++ VoIP & Intercom CLI Tool     \n";
    std::cout << "========================================================\n";
    std::cout << " Command        : " << qPrintable(cmd) << "\n";
    std::cout << " User MID       : " << qPrintable(userId) << "\n";
    std::cout << " Caller VUID    : " << callerVuid << "\n";
    std::cout << " Target Speaker : " << qPrintable(targetName) << " (" << qPrintable(targetModel) << ")\n";
    std::cout << " Target VUID    : " << qPrintable(targetVuid) << "\n";
    std::cout << " Target DID     : " << qPrintable(targetDid) << "\n";
    std::cout << " Target DeviceID: " << qPrintable(targetDeviceId) << "\n";
    std::cout << "========================================================\n";

    EmbeddedVoipBackend backend;
    g_backend = &backend;

    backend.init(userId, serviceToken, QStringLiteral("251673189"));
    backend.setCallerVuid(callerVuid);
    backend.setTarget(targetDeviceId, targetVuid, targetDid, targetModel);

    QObject::connect(&backend, &IVoipBackend::stateChanged, [](int state, const QString &statusText) {
        std::cout << "[STATE] -> " << state << " (" << qPrintable(statusText) << ")\n";
        if (state == IVoipBackend::StateRinging) {
            std::cout << "🔔🔔🔔 >>> REMOTE SPEAKER IS PHYSICALLY RINGING! <<< 🔔🔔🔔\n";
        } else if (state == IVoipBackend::StateConnected) {
            std::cout << "🎉🎉🎉 >>> CALL CONNECTED! AUDIO ACTIVE! <<< 🎉🎉🎉\n";
        }
    });

    QObject::connect(&backend, &IVoipBackend::durationChanged, [](int sec) {
        std::cout << "⏱ Call duration: " << sec << "s\r" << std::flush;
    });

    QObject::connect(&backend, &IVoipBackend::errorOccurred, [](const QString &errMsg) {
        std::cerr << "[ERROR] " << qPrintable(errMsg) << "\n";
    });

    if (cmd == QStringLiteral("wakeup")) {
        std::cout << "[voip_cli] Triggering notifyMiLinkEnable for target " << qPrintable(targetVuid) << "...\n";
        backend.sendWakeup(targetVuid);
        QTimer::singleShot(2500, &app, &QCoreApplication::quit);
    } else if (cmd == QStringLiteral("tts")) {
        std::cout << "[voip_cli] Sending TTS speech to target: '" << qPrintable(messageText) << "'...\n";
        backend.sendTts(targetDeviceId, messageText);
        QTimer::singleShot(3000, &app, &QCoreApplication::quit);
    } else if (cmd == QStringLiteral("intercom")) {
        std::cout << "[voip_cli] Initiating direct auto-answer intercom to " << qPrintable(targetName) << "...\n";
        std::cout << "[voip_cli] Speaker will auto-answer without manual pickup.\n";
        std::cout << "[voip_cli] Press Ctrl+C anytime to hang up.\n";
        backend.startDirectIntercom(targetDeviceId, targetName, targetVuid, targetModel, 105, targetDid);

        if (durationSec > 0) {
            QTimer::singleShot(durationSec * 1000, [&backend, &app, durationSec]() {
                std::cout << "\n[voip_cli] Auto-hangup timer reached (" << durationSec << "s). Hanging up...\n";
                backend.hangupCall();
                QTimer::singleShot(1500, &app, &QCoreApplication::quit);
            });
        }
    } else if (cmd == QStringLiteral("hangup")) {
        std::cout << "[voip_cli] Sending hangup signal to " << qPrintable(targetName) << "...\n";
        backend.hangupCall();
        QTimer::singleShot(1500, &app, &QCoreApplication::quit);
    } else if (cmd == QStringLiteral("status")) {
        std::cout << "[voip_cli] Querying device player status for " << qPrintable(targetName) << "...\n";
        backend.queryPlayerStatus(targetDeviceId);
        QTimer::singleShot(3000, &app, &QCoreApplication::quit);
    } else if (cmd == QStringLiteral("call")) {
        std::cout << "[voip_cli] Initiating call to " << qPrintable(targetName) << "...\n";
        std::cout << "[voip_cli] Remote speaker should physically ring/prompt '来电话了'.\n";
        std::cout << "[voip_cli] Press Ctrl+C anytime to hang up.\n";
        backend.startCall(targetDeviceId, targetName, targetVuid, targetModel, 105, targetDid);

        if (durationSec > 0) {
            QTimer::singleShot(durationSec * 1000, [&backend, &app, durationSec]() {
                std::cout << "\n[voip_cli] Auto-hangup timer reached (" << durationSec << "s). Hanging up...\n";
                backend.hangupCall();
                QTimer::singleShot(1500, &app, &QCoreApplication::quit);
            });
        }
    } else {
        std::cerr << "Unknown command: " << qPrintable(cmd) << "\n";
        parser.showHelp(1);
    }

    return app.exec();
}
