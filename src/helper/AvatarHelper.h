#pragma once

#include <M5StickCPlus2.h>  // oder allgemein <M5Unified.h>, je nach Projekt
#include <Avatar.h>         // m5stack-avatar
using namespace m5avatar;

class AvatarHelper {
public:
    AvatarHelper();
    void init(int topOffset = -40, int leftOffset = -40, float scale = 0.8f);
    void update();
    void setIdle(bool idle);
    void setIdleExpression(Expression expr);
    bool isAvatarIdle() const;
    void setExpression(Expression expression);

private:
    Avatar avatar;
    Expression idleExpression;
    bool isIdle;
    int top;
    int left;
    unsigned long lastFaceUpdate;
};
