#pragma once
#include <QGraphicsPixmapItem>
#include <QPointF>
#include "game_constants.h"

class Monster;
class Bomb : public QGraphicsPixmapItem {
public:
    Monster* targetMonster;
    Bomb(QPointF pos);
    Monster* findNearestMonster();
    void updateMove();
    bool isOut() const;
};