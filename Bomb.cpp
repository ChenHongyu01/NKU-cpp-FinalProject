#include "Bomb.h"
#include "Monster.h"
#include <QPixmap>
#include <cmath>
#include <QGraphicsScene>

Bomb::Bomb(QPointF pos) : targetMonster(nullptr) {
    setPixmap(QPixmap(":/assets/bomb.png").scaled(BOMB_SIZE, BOMB_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    setPos(pos);
    setZValue(60);
}

Monster* Bomb::findNearestMonster() {
    Monster* nearest = nullptr;
    float minDist = 99999.0f;
    for (auto* item : scene()->items()) {
        Monster* m = dynamic_cast<Monster*>(item);
        if (m != nullptr && !m->isDead) {
            float dist = std::hypot(m->x() - x(), m->y() - y());
            if (dist < minDist) {
                minDist = dist;
                nearest = m;
            }
        }
    }
    return nearest;
}

void Bomb::updateMove() {
    targetMonster = findNearestMonster();
    if (targetMonster == nullptr) return;
    float dx = targetMonster->x() - x();
    float dy = targetMonster->y() - y();
    float len = std::hypot(dx, dy);
    if (len > 0) { dx /= len; dy /= len; }
    setPos(x() + dx * BOMB_SPEED, y() + dy * BOMB_SPEED);
}

bool Bomb::isOut() const {
    return x() < -BOMB_SIZE || x() > SCREEN_WIDTH || y() < -BOMB_SIZE || y() > SCREEN_HEIGHT;
}