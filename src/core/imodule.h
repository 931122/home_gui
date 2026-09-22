#ifndef IMODULE_H
#define IMODULE_H

#include <QObject>
#include <QString>

#include "appconfig.h"

class GlobalState;

// 运行时模块基类。
// 所有需要由 ModuleManager 统一启动/停止/重载的能力模块都继承它。
class IModule : public QObject
{
    Q_OBJECT

public:
    explicit IModule(GlobalState *globalState, QObject *parent = nullptr)
        : QObject(parent)
        , m_globalState(globalState)
    {
    }

    ~IModule() override = default;

    virtual QString name() const = 0;
    virtual void applyConfig(const AppConfig &config) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

protected:
    GlobalState *globalState() const
    {
        // 子类通过它把状态回写到全局状态对象。
        return m_globalState;
    }

private:
    GlobalState *m_globalState;
};

#endif
