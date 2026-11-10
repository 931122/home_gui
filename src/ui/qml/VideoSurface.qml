import QtQuick
import HomeGui 1.0

Item {
    id: surfaceRoot
    property alias framePresented: videoItem.framePresented

    VideoItem {
        id: videoItem
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
        Component.onCompleted: {
            console.log("[VideoSurface] Completed! videoSource:", appController.videoSource, "videoEnabled:", appController.videoEnabled, "isScreenOff:", appController.isScreenOff)
        }
    }
}
