import QtQuick
import HomeGui 1.0

Item {
    VideoItem {
        anchors.fill: parent
        source: appController.videoSource
        backend: appController.videoBackend
        decoder: appController.videoDecoder
        sharedFrameKey: appController.videoSource.toString()
        publishFrames: true
        autoPlay: appController.videoEnabled && !appController.isScreenOff
        networkOnline: appController.networkOnline

        onActiveDecoderChanged: {
            if (activeDecoder) {
                globalState.activeVideoDecoder = activeDecoder
            }
        }
    }
}
