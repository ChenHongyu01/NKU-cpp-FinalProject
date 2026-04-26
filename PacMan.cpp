#include "PacMan.h"
#include <QPixmap>

PacMan::PacMan(bool right, qreal startY) : moveRight(right) {
    if (moveRight) {
        setPixmap(QPixmap(":/assets/PacmanRight.png").scaled(PACMAN_SIZE, PACMAN_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setPos(0, startY);
    } else {
        setPixmap(QPixmap(":/assets/PacmanLeft.png").scaled(PACMAN_SIZE, PACMAN_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setPos(SCREEN_WIDTH - PACMAN_SIZE, startY);
    }
    setZValue(50);
}

void PacMan::updateMove() {
    if (moveRight) setX(x() + PACMAN_SPEED);
    else setX(x() - PACMAN_SPEED);
}

bool PacMan::isOut() const {
    return (moveRight && x() > SCREEN_WIDTH) || (!moveRight && x() < -PACMAN_SIZE);
}