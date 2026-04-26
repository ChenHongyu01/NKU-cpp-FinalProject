#pragma once
#include <QGraphicsPixmapItem>
#include <QList>
#include <QPixmap>
#include <QDateTime>
#include "game_constants.h"

class Monster : public QGraphicsPixmapItem {
public:
    float dirX, dirY;
    int animFrameCounter;
    int currentAnimFrame;
    int animDir;
    QList<QPixmap> animFrames;
    bool isDead;
    qint64 deadTime;

    Monster();
    void startDeath();
    bool canDelete() const;
    void updateAnimation();
    void updateEscapeDir(qreal px, qreal py);
    void moveAway();
    bool isOutOfScreen() const;
};