#include "AvatarHelper.h"


AvatarHelper::AvatarHelper()
    : isIdle(true), top(-40), left(-40), lastFaceUpdate(0),
      idleExpression(Expression::Sleepy) {  // Default face
}

bool AvatarHelper::isAvatarIdle() const {
    return isIdle;
}

void AvatarHelper::setIdle(bool idle) {
    isIdle = idle;
}

void AvatarHelper::setExpression(Expression expression) {
    avatar.setExpression(expression);
}

void AvatarHelper::init(int topOffset, int leftOffset, float scale) {
    avatar.init();
    top = topOffset;
    left = leftOffset;
    avatar.setPosition(top, left);
    avatar.setScale(scale);
    avatar.setExpression(Expression::Neutral);
    avatar.setMouthOpenRatio(0.1f); // mouth open

    lastFaceUpdate = millis();
}

void AvatarHelper::update() {
    avatar.setPosition(top, left);
}

void AvatarHelper::setIdleExpression(Expression expr) {
    idleExpression = expr;
}

