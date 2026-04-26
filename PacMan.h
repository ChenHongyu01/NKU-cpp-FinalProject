#pragma once
#include <QGraphicsPixmapItem>
#include "game_constants.h"

class PacMan : public QGraphicsPixmapItem {
public:
    bool moveRight;
    PacMan(bool right, qreal startY);
    void updateMove();
    bool isOut() const;
};