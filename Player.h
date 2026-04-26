#pragma once
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <queue>
#include <QList>
#include <QDateTime>
#include "game_constants.h"

class Player : public QGraphicsPixmapItem {
public:
    bool keyW, keyS, keyA, keyD;
    bool keyUp, keyDown, keyLeft, keyRight;
    QPointF targetPos;
    MoveMode moveMode;
    bool isAccel;
    qint64 accelEndTime;
    int frameCount;
    std::queue<QGraphicsPixmapItem*> trailItems;
    bool isMoving;
    int moveAnimTimer;
    bool isAnimFrame1;
    const int animSwitchFrame = 8;
    RoleType roleType;

    Player(RoleType role);
    ~Player();
    void loadIdlePixmap();
    void updateMoveAnimation();
    void startAccel();
    void updateMove();
    void spawnTrail();
    void clearTrails();
    void setMouseTarget(const QPointF& p);
    void clampToScreen();
};