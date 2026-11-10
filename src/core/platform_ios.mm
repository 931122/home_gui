#include "platformhelper.h"
#import <UIKit/UIKit.h>

static UIWindow* getActiveKeyWindow()
{
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            for (UIWindow *w in windowScene.windows) {
                if (w.isKeyWindow) {
                    return w;
                }
            }
            if (windowScene.windows.count > 0) {
                return windowScene.windows.firstObject;
            }
        }
    }
    for (UIWindow *w in [UIApplication sharedApplication].windows) {
        if (w.isKeyWindow) {
            return w;
        }
    }
    if ([UIApplication sharedApplication].windows.count > 0) {
        return [UIApplication sharedApplication].windows.firstObject;
    }
    return nil;
}

void PlatformHelper::setStatusBarVisible(bool visible)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        const BOOL hidden = !visible;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [[UIApplication sharedApplication] setStatusBarHidden:hidden withAnimation:UIStatusBarAnimationFade];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
#pragma clang diagnostic pop

        UIWindow *targetWindow = getActiveKeyWindow();
        if (targetWindow && targetWindow.rootViewController) {
            UIViewController *vc = targetWindow.rootViewController;
            if ([vc respondsToSelector:@selector(setPrefersStatusBarHidden:)]) {
                typedef void (*SetHiddenFn)(id, SEL, BOOL);
                SetHiddenFn fn = (SetHiddenFn)[vc methodForSelector:@selector(setPrefersStatusBarHidden:)];
                if (fn) {
                    fn(vc, @selector(setPrefersStatusBarHidden:), hidden);
                }
            }
            [vc setNeedsStatusBarAppearanceUpdate];
        }
    });
}

static UIEdgeInsets getIosSafeAreaInsets()
{
    __block UIEdgeInsets insets = UIEdgeInsetsZero;
    void (^querySafe)(void) = ^{
        UIWindow *targetWindow = getActiveKeyWindow();
        if (targetWindow) {
            insets = targetWindow.safeAreaInsets;
        }
    };

    if ([NSThread isMainThread]) {
        querySafe();
    } else {
        dispatch_sync(dispatch_get_main_queue(), querySafe);
    }
    return insets;
}

qreal PlatformHelper::safeAreaTop()
{
    return getIosSafeAreaInsets().top;
}

qreal PlatformHelper::safeAreaBottom()
{
    return getIosSafeAreaInsets().bottom;
}

qreal PlatformHelper::safeAreaLeft()
{
    return getIosSafeAreaInsets().left;
}

qreal PlatformHelper::safeAreaRight()
{
    return getIosSafeAreaInsets().right;
}
