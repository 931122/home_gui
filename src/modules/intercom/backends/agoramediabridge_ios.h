#ifndef AGORAMEDIABRIDGE_IOS_H
#define AGORAMEDIABRIDGE_IOS_H

#include <QString>
#include <QtGlobal>

namespace AgoraMediaBridgeIos {

void init();
int joinChannel(const QString &appId,
                const QString &token,
                const QString &channelId,
                quint32 uid,
                bool speakerphone,
                bool muted);
void leaveChannel();
void setMute(bool muted);
void setSpeakerphone(bool enable);

} // namespace AgoraMediaBridgeIos

#endif // AGORAMEDIABRIDGE_IOS_H
