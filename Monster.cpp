#include "Monster.h"
#include <random>
#include <cmath>
#include <QPixmap>

Monster::Monster() {
    animFrames << QPixmap(":/assets/monsterLeft.png") << QPixmap(":/assets/monsterLeft1.png")
    << QPixmap(":/assets/monster.png") << QPixmap(":/assets/monsterRight1.png")
    << QPixmap(":/assets/monsterRight.png");
    setPixmap(animFrames[2].scaled(MONSTER_SIZE, MONSTER_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    animFrameCounter = 0;
    currentAnimFrame = 2;
    animDir = 1;
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_real_distribution<> rx(0, SCREEN_WIDTH - MONSTER_SIZE);
    std::uniform_real_distribution<> ry(0, SCREEN_HEIGHT - MONSTER_SIZE);
    setPos(rx(gen), ry(gen));
    dirX = dirY = 0;
    isDead = false;
    deadTime = 0;
}

void Monster::startDeath() {
    if (isDead) return;
    isDead = true;
    deadTime = QDateTime::currentMSecsSinceEpoch();
    QPixmap deadPix(":/assets/dead.png");
    setPixmap(deadPix.scaled(MONSTER_DEAD_SIZE, MONSTER_DEAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

bool Monster::canDelete() const {
    if (!isDead) return false;
    return QDateTime::currentMSecsSinceEpoch() >= deadTime + DEATH_DELAY_MS;
}

void Monster::updateAnimation() {
    if (isDead) return;
    animFrameCounter++;
    if (animFrameCounter >= MONSTER_ANIM_INTERVAL) {
        animFrameCounter = 0;
        currentAnimFrame += animDir;
        if (currentAnimFrame <= 0) { currentAnimFrame = 0; animDir = 1; }
        else if (currentAnimFrame >= 4) { currentAnimFrame = 4; animDir = -1; }
        setPixmap(animFrames[currentAnimFrame].scaled(MONSTER_SIZE, MONSTER_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void Monster::updateEscapeDir(qreal px, qreal py) {
    if (isDead) { dirX = 0; dirY = 0; return; }
    qreal dx = x() - px;
    qreal dy = y() - py;
    qreal dist = std::hypot(dx, dy);
    if (dist > MONSTER_ACTIVE_DISTANCE) { dirX = 0; dirY = 0; return; }
    qreal len = dist < 1e-5 ? 1 : dist;
    dirX = dx / len * MONSTER_SPEED;
    dirY = dy / len * MONSTER_SPEED;
}

void Monster::moveAway() {
    if (isDead) return;
    setPos(x() + dirX, y() + dirY);
}

bool Monster::isOutOfScreen() const {
    return x() < -MONSTER_SIZE || x() > SCREEN_WIDTH || y() < -MONSTER_SIZE || y() > SCREEN_HEIGHT;
}