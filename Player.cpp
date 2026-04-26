#include "Player.h"
#include <QPixmap>
#include <cmath>
#include <QGraphicsScene>

Player::Player(RoleType role) : roleType(role) {
    loadIdlePixmap();
    setPos(SCREEN_WIDTH/2 - PLAYER_WIDTH/2, SCREEN_HEIGHT/2 - PLAYER_HEIGHT/2);
    keyW = keyS = keyA = keyD = false;
    keyUp = keyDown = keyLeft = keyRight = false;
    moveMode = MOVE_KEYBOARD;
    targetPos = pos();
    isAccel = false;
    accelEndTime = 0;
    frameCount = 0;
    isMoving = false;
    moveAnimTimer = 0;
    isAnimFrame1 = true;
}

Player::~Player() {
    clearTrails();
}

void Player::loadIdlePixmap() {
    if (roleType == ROLE_YING) {
        setPixmap(QPixmap(":/assets/Ying.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        setPixmap(QPixmap(":/assets/Lang.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
}

void Player::updateMoveAnimation() {
    bool moving = false;
    if (moveMode == MOVE_KEYBOARD) {
        moving = keyW || keyS || keyA || keyD || keyUp || keyDown || keyLeft || keyRight;
    } else {
        moving = (std::hypot(targetPos.x() - x(), targetPos.y() - y()) > 2);
    }
    isMoving = moving;
    if (!isMoving) {
        loadIdlePixmap();
        moveAnimTimer = 0;
        isAnimFrame1 = true;
        return;
    }
    moveAnimTimer++;
    if (moveAnimTimer >= animSwitchFrame) {
        moveAnimTimer = 0;
        if (isAnimFrame1) {
            if (roleType == ROLE_YING) {
                setPixmap(QPixmap(":/assets/Ying2.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            } else {
                setPixmap(QPixmap(":/assets/Lang2.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            isAnimFrame1 = false;
        } else {
            if (roleType == ROLE_YING) {
                setPixmap(QPixmap(":/assets/Ying1.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            } else {
                setPixmap(QPixmap(":/assets/Lang1.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            isAnimFrame1 = true;
        }
    }
}

void Player::startAccel() {
    if (roleType == ROLE_YING) {
        isAccel = true;
        accelEndTime = QDateTime::currentMSecsSinceEpoch() + ACCEL_DURATION_MS;
    }
}

void Player::updateMove() {
    updateMoveAnimation();
    bool inAccel = isAccel && QDateTime::currentMSecsSinceEpoch() < accelEndTime;
    float speed = inAccel ? PLAYER_SPEED * ACCEL_RATE : PLAYER_SPEED;
    if (moveMode == MOVE_KEYBOARD) {
        float dx = 0, dy = 0;
        if (keyW || keyUp)    dy -= 1;
        if (keyS || keyDown)  dy += 1;
        if (keyA || keyLeft)  dx -= 1;
        if (keyD || keyRight) dx += 1;
        float len = std::hypot(dx, dy);
        if (len > 0) { dx /= len; dy /= len; }
        setPos(x() + dx * speed, y() + dy * speed);
    } else {
        float dx = targetPos.x() - x();
        float dy = targetPos.y() - y();
        float len = std::hypot(dx, dy);
        if (len > 2) { dx /= len; dy /= len; setPos(x() + dx * speed, y() + dy * speed); }
    }
    clampToScreen();
    if (inAccel) spawnTrail();
    else clearTrails();
}

void Player::spawnTrail() {
    if (roleType != ROLE_YING) return;
    frameCount++;
    if (frameCount >= TRAIL_SPAWN_FRAME) {
        frameCount = 0;
        if (trailItems.size() >= TRAIL_MAX) {
            QGraphicsPixmapItem* oldest = trailItems.front();
            scene()->removeItem(oldest);
            delete oldest;
            trailItems.pop();
        }
        QGraphicsPixmapItem* trail = new QGraphicsPixmapItem(pixmap());
        trail->setPos(pos());
        trail->setOpacity(TRAIL_ALPHA);
        trail->setZValue(-1);
        scene()->addItem(trail);
        trailItems.push(trail);
    }
}

void Player::clearTrails() {
    while (!trailItems.empty()) {
        QGraphicsPixmapItem* t = trailItems.front();
        trailItems.pop();
        scene()->removeItem(t);
        delete t;
    }
    frameCount = 0;
}

void Player::setMouseTarget(const QPointF& p) {
    targetPos = p;
}

void Player::clampToScreen() {
    qreal nx = qBound(0.0, x(), (qreal)SCREEN_WIDTH - PLAYER_WIDTH);
    qreal ny = qBound(0.0, y(), (qreal)SCREEN_HEIGHT - PLAYER_HEIGHT);
    setPos(nx, ny);
}