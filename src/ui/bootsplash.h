#ifndef BOOTSPLASH_H
#define BOOTSPLASH_H

#include <memory>

#include "core/appconfig.h"

class BootSplash
{
public:
    explicit BootSplash(const PlatformConfig &config);
    ~BootSplash();

    void close();
    void raise();
    bool minimumVisibleElapsed(int milliseconds) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

void drawEarlyFramebufferSplash(const PlatformConfig &config);

#endif
