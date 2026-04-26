#include <QApplication>
#include <QGraphicsView>
#include <QPainter>
#include "GameScene.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QGraphicsView view;
    view.setFixedSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setRenderHint(QPainter::Antialiasing);
    GameScene* scene = new GameScene();
    view.setScene(scene);
    view.setWindowTitle("星琼大作战");
    view.show();
    return a.exec();
}