#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QPixmap>
#include <random>
#include <cmath>
#include <QList>
#include <limits>

//屏幕需要4：3
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// 角色： 高:宽 = 4:3
const int PLAYER_WIDTH  = 36;
const int PLAYER_HEIGHT = 48;
// 技能
const float PLAYER_SPEED = 5.0f;
const float DASH_SPEED = 25.0f;
const int DASH_DURATION = 8;
const int ADD_TIME_PER_KILL = 1;
// 怪
const int MONSTER_SIZE = 24;
const float MONSTER_SPEED = 3.0f;



enum GameState {
    MENU_SELECT_TIME,
    GAME_PLAYING,
    GAME_OVER
};

enum MoveMode {
    MOVE_KEYBOARD,
    MOVE_MOUSE
};

class Player : public QGraphicsPixmapItem {
public:
    bool keyW, keyS, keyA, keyD;
    bool keyUp, keyDown, keyLeft, keyRight;
    QPointF targetPos;
    MoveMode moveMode;
    bool isDashing;
    int dashTimer;
    float dashDirX, dashDirY;

    Player() {
        QPixmap pix(":/assets/Ying.png");
        setPixmap(pix.scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

        setPos(SCREEN_WIDTH/2 - PLAYER_WIDTH/2, SCREEN_HEIGHT/2 - PLAYER_HEIGHT/2);

        keyW = keyS = keyA = keyD = false;
        keyUp = keyDown = keyLeft = keyRight = false;
        moveMode = MOVE_KEYBOARD;
        targetPos = pos();
        isDashing = false;
        dashTimer = 0;
        dashDirX = dashDirY = 0;
    }

    void startDash(float dirX, float dirY) {
        float len = std::hypot(dirX, dirY);
        if (len < 0.1f) { dirX = 1; dirY = 0; len = 1; }
        dashDirX = dirX / len;
        dashDirY = dirY / len;
        isDashing = true;
        dashTimer = DASH_DURATION;
    }

    void updateMove() {
        if (isDashing && dashTimer > 0) {
            setPos(x() + dashDirX * DASH_SPEED, y() + dashDirY * DASH_SPEED);
            dashTimer--;
            clampToScreen();
            return;
        }
        isDashing = false;
        dashTimer = 0;

        if (moveMode == MOVE_KEYBOARD) {
            float dx = 0, dy = 0;
            if (keyW || keyUp)    dy -= 1;
            if (keyS || keyDown)  dy += 1;
            if (keyA || keyLeft)  dx -= 1;
            if (keyD || keyRight) dx += 1;
            float len = std::hypot(dx, dy);
            if (len > 0) { dx /= len; dy /= len; }
            setPos(x() + dx * PLAYER_SPEED, y() + dy * PLAYER_SPEED);
        } else {
            float dx = targetPos.x() - x();
            float dy = targetPos.y() - y();
            float len = std::hypot(dx, dy);
            if (len > 2) {
                dx /= len; dy /= len;
                setPos(x() + dx * PLAYER_SPEED, y() + dy * PLAYER_SPEED);
            }
        }
        clampToScreen();
    }

    void setMouseTarget(const QPointF& pos) {
        targetPos = pos;
    }

    void clampToScreen() {
        // 边界4:3
        qreal nx = qBound(0.0, x(), (qreal)SCREEN_WIDTH - PLAYER_WIDTH);
        qreal ny = qBound(0.0, y(), (qreal)SCREEN_HEIGHT - PLAYER_HEIGHT);
        setPos(nx, ny);
    }
};

class Monster : public QGraphicsPixmapItem {
public:
    float dirX, dirY;

    Monster() {
        QPixmap pix(":/assets/monster.png");
        setPixmap(pix.scaled(MONSTER_SIZE, MONSTER_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> rx(0, SCREEN_WIDTH - MONSTER_SIZE);
        std::uniform_real_distribution<> ry(0, SCREEN_HEIGHT - MONSTER_SIZE);
        setPos(rx(gen), ry(gen));
        dirX = dirY = 0;
    }

    void updateEscapeDir(qreal playerX, qreal playerY) {
        qreal dx = x() - playerX;
        qreal dy = y() - playerY;
        qreal len = std::hypot(dx, dy);
        if (len < 1e-5) len = 1;
        dirX = dx / len * MONSTER_SPEED;
        dirY = dy / len * MONSTER_SPEED;
    }

    void moveAway() {
        setPos(x() + dirX, y() + dirY);
    }

    bool isOutOfScreen() const {
        return x() < -MONSTER_SIZE || x() > SCREEN_WIDTH ||
               y() < -MONSTER_SIZE || y() > SCREEN_HEIGHT;
    }
};

class GameScene : public QGraphicsScene {
    Q_OBJECT
public:
    GameState state;
    Player* player;
    QTimer *spawnTimer, *gameTimer, *countDownTimer, *skillRegenTimer;
    int score, timeLeft, selectedGameTime;
    MoveMode moveMode;
    int skillPoint, maxSkillPoint;
    int qEnergy, maxQEnergy;

    GameScene() {
        setSceneRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        state = MENU_SELECT_TIME;
        player = nullptr;
        score = timeLeft = 0;
        selectedGameTime = 60;
        moveMode = MOVE_KEYBOARD;

        maxSkillPoint = 5;
        maxQEnergy = 100;

        spawnTimer = new QTimer(this);
        gameTimer = new QTimer(this);
        countDownTimer = new QTimer(this);
        skillRegenTimer = new QTimer(this);

        connect(spawnTimer, &QTimer::timeout, this, &GameScene::spawnMonsters);
        connect(gameTimer, &QTimer::timeout, this, &GameScene::updateGame);
        connect(countDownTimer, &QTimer::timeout, this, &GameScene::updateTime);
        connect(skillRegenTimer, &QTimer::timeout, this, &GameScene::regenerateSkillPoint);

        drawMenu();
    }

    void startGame() {
        spawnTimer->stop(); gameTimer->stop(); countDownTimer->stop(); skillRegenTimer->stop();
        clear();
        state = GAME_PLAYING;
        score = 0;
        timeLeft = selectedGameTime;
        skillPoint = 3;
        qEnergy = 0;

        player = new Player();
        player->moveMode = moveMode;
        addItem(player);

        spawnTimer->start(500);
        gameTimer->start(16);
        countDownTimer->start(1000);
        skillRegenTimer->start(5000);
    }

    void backToMenu() {
        spawnTimer->stop(); gameTimer->stop(); countDownTimer->stop(); skillRegenTimer->stop();
        state = GAME_OVER;
        clear();
        player = nullptr;
        drawMenu();
    }

    void drawMenu() {
        clear();
        QString menuText;
        if(state == GAME_OVER) menuText = QString("时间到！\n最终得分：%1\n\n").arg(score);

        QString moveStr = (moveMode == MOVE_KEYBOARD) ? "WASD / ←↑↓→ 键盘移动" : "鼠标跟随移动";
        menuText += QString("倒计时 按1：30秒  按2：60秒 \n当前模式：倒计时%1秒\n\n").arg(selectedGameTime);
        menuText += QString("按9：键盘移动  按0：鼠标移动\n当前操作：%1\n\n").arg(moveStr);
        menuText += "按Q清屏  按E向最近怪物突进\n按空格开始  ESC退出";

        auto* t = addText(menuText);
        t->setFont(QFont("Microsoft YaHei", 16));
        t->setPos(SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 120);
    }

    void spawnMonsters() {
        if (state != GAME_PLAYING || !player) return;
        std::random_device rd; std::mt19937 gen(rd());
        std::uniform_int_distribution<> cnt(3,5);
        for(int i=0; i<cnt(gen); i++) addItem(new Monster());
    }

    void regenerateSkillPoint() {
        if (state != GAME_PLAYING) return;
        if (skillPoint < maxSkillPoint) skillPoint++;
    }

    Monster* findNearestMonster() {
        if (!player || state != GAME_PLAYING) return nullptr;

        Monster* nearest = nullptr;
        double minDist = std::numeric_limits<double>::max();
        QPointF playerPos = player->pos();

        for (auto* item : items()) {
            if (auto* m = dynamic_cast<Monster*>(item)) {
                double dist = std::hypot(m->x() - playerPos.x(), m->y() - playerPos.y());
                if (dist < minDist) {
                    minDist = dist;
                    nearest = m;
                }
            }
        }
        return nearest;
    }

    void useDash() {
        if (!player || skillPoint < 1) return;

        Monster* nearest = findNearestMonster();
        if (!nearest) return;

        float dx = nearest->x() - player->x();
        float dy = nearest->y() - player->y();

        skillPoint--;
        player->startDash(dx, dy);
    }

    void useQClear() {
        if (qEnergy < maxQEnergy) return;
        qEnergy = 0;
        QList<Monster*> monsters;
        for (auto* item : items()) if (auto* m = dynamic_cast<Monster*>(item)) monsters.append(m);
        for (auto* m : monsters) {
            score += 10;
            timeLeft += ADD_TIME_PER_KILL;
            removeItem(m); delete m;
        }
    }

    void updateGame() {
        if (state != GAME_PLAYING || !player) return;
        player->updateMove();

        QList<Monster*> monsters;
        for (auto* item : items()) if (auto* m = dynamic_cast<Monster*>(item)) monsters.append(m);

        for (auto* m : monsters) {
            m->updateEscapeDir(player->x(), player->y());
            m->moveAway();
            if (m->collidesWithItem(player)) {
                score += 10;
                timeLeft += ADD_TIME_PER_KILL;
                if (qEnergy < maxQEnergy) qEnergy += 10;
                removeItem(m); delete m;
            } else if (m->isOutOfScreen()) {
                removeItem(m); delete m;
            }
        }
        updateUI();
    }

    void updateTime() {
        if (state != GAME_PLAYING) return;
        if (--timeLeft <= 0) backToMenu();
    }

    void updateUI() {
        QList<QGraphicsItem*> uiItems;
        for (auto* item : items()) {
            if (auto* t = dynamic_cast<QGraphicsTextItem*>(item)) {
                QString s = t->toPlainText();
                if (s.startsWith("倒计时")||s.startsWith("得分")||s.startsWith("技能")||s.startsWith("Q充能")||s.startsWith("操作"))
                    uiItems.append(item);
            }
        }
        for (auto* i : uiItems) { removeItem(i); delete i; }

        QString moveStr = (moveMode == MOVE_KEYBOARD) ? "键盘操作 WASD/←↑↓→" : "鼠标操作";
        addText(QString("操作模式：%1").arg(moveStr))->setPos(10,70);
        addText(QString("倒计时：%1s").arg(timeLeft))->setPos(10,10);
        addText(QString("得分：%1").arg(score))->setPos(SCREEN_WIDTH-100,10);
        addText(QString("技能点：%1/%2").arg(skillPoint).arg(maxSkillPoint))->setPos(10,30);
        addText(QString("Q充能：%1/%2").arg(qEnergy).arg(maxQEnergy))->setPos(10,50);
    }

protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (state == MENU_SELECT_TIME || state == GAME_OVER) {
            if (e->key() == Qt::Key_1) { selectedGameTime = 30; drawMenu(); }
            if (e->key() == Qt::Key_2) { selectedGameTime = 60; drawMenu(); }
            if (e->key() == Qt::Key_9) { moveMode = MOVE_KEYBOARD; drawMenu(); }
            if (e->key() == Qt::Key_0) { moveMode = MOVE_MOUSE; drawMenu(); }
            if (e->key() == Qt::Key_Space) startGame();
            if (e->key() == Qt::Key_Escape) qApp->quit();
            return;
        }

        if (!player) return;
        if (e->key() == Qt::Key_E) useDash();
        if (e->key() == Qt::Key_Q) useQClear();
        if (e->key() == Qt::Key_Escape) qApp->quit();

        if (player->moveMode == MOVE_KEYBOARD) {
            if (e->key() == Qt::Key_W) player->keyW = true;
            if (e->key() == Qt::Key_S) player->keyS = true;
            if (e->key() == Qt::Key_A) player->keyA = true;
            if (e->key() == Qt::Key_D) player->keyD = true;

            if (e->key() == Qt::Key_Up)    player->keyUp = true;
            if (e->key() == Qt::Key_Down)  player->keyDown = true;
            if (e->key() == Qt::Key_Left)  player->keyLeft = true;
            if (e->key() == Qt::Key_Right) player->keyRight = true;
        }
    }

    void keyReleaseEvent(QKeyEvent* e) override {
        if (!player || state != GAME_PLAYING || player->moveMode != MOVE_KEYBOARD) return;
        if (e->key() == Qt::Key_W) player->keyW = false;
        if (e->key() == Qt::Key_S) player->keyS = false;
        if (e->key() == Qt::Key_A) player->keyA = false;
        if (e->key() == Qt::Key_D) player->keyD = false;

        if (e->key() == Qt::Key_Up)    player->keyUp = false;
        if (e->key() == Qt::Key_Down)  player->keyDown = false;
        if (e->key() == Qt::Key_Left)  player->keyLeft = false;
        if (e->key() == Qt::Key_Right) player->keyRight = false;
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override {
        if (state == GAME_PLAYING && player && player->moveMode == MOVE_MOUSE)
            player->setMouseTarget(e->scenePos());
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QGraphicsView view;
    view.setFixedSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setRenderHint(QPainter::Antialiasing);
    GameScene* scene = new GameScene();
    view.setScene(scene);
    view.show();
    return a.exec();
}

#include "main.moc"