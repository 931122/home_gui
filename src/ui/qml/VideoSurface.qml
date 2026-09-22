import QtQuick 2.12
import HomeGui 1.0

Item {
    VideoItem {
        anchors.fill: parent
        source: appController.videoSource
        backend: appController.videoBackend
        decoder: appController.videoDecoder
        sharedFrameKey: appController.videoSource.toString()
        publishFrames: true
        autoPlay: true
        networkOnline: appController.networkOnline

        onActiveDecoderChanged: {
            if (activeDecoder) {
                globalState.activeVideoDecoder = activeDecoder
            }
        }
    }
}
