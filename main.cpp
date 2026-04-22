#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QPixmap>
#include <QDateTime>
#include <random>
#include <cmath>
#include <QList>
#include <queue>
#include <limits>
#include <QDebug>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

//屏幕
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 900;
//角色
const int PLAYER_WIDTH  = 60;
const int PLAYER_HEIGHT = 80;
//星琼
const int MONSTER_SIZE = 48;
const int MONSTER_DEAD_SIZE = 72;
//基础属性
const float PLAYER_SPEED = 5.0f;
const float MONSTER_SPEED = 2.8f;
const int ADD_TIME_PER_KILL = 0;
const int MAX_MONSTER_COUNT = 20;
const int MONSTER_ANIM_INTERVAL = 5;
const float MONSTER_ACTIVE_DISTANCE = 100.0f;

//技能
const float ACCEL_RATE = 2.0f;
const int ACCEL_DURATION_MS = 1500;
const int TRAIL_MAX = 5;
const int TRAIL_SPAWN_FRAME = 3;
const float TRAIL_ALPHA = 0.25f;
const int DEATH_DELAY_MS = 500;
const int Q_SKILL_MAX_RADIUS = 400;
const int Q_EFFECT_DURATION = 250;
const int Q_SPAWN_PAUSE_DURATION = 500;

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
    bool isAccel;
    qint64 accelEndTime;
    int frameCount;
    std::queue<QGraphicsPixmapItem*> trailItems;
    bool isMoving;
    int moveAnimTimer;
    bool isAnimFrame1;
    const int animSwitchFrame = 8;

    Player() {
        setPixmap(QPixmap(":/assets/Ying.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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

    void updateMoveAnimation() {
        bool moving = false;
        if (moveMode == MOVE_KEYBOARD) {
            moving = keyW || keyS || keyA || keyD || keyUp || keyDown || keyLeft || keyRight;
        } else {
            moving = (std::hypot(targetPos.x() - x(), targetPos.y() - y()) > 2);
        }
        isMoving = moving;
        if (!isMoving) {
            setPixmap(QPixmap(":/assets/Ying.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            moveAnimTimer = 0;
            isAnimFrame1 = true;
            return;
        }
        moveAnimTimer++;
        if (moveAnimTimer >= animSwitchFrame) {
            moveAnimTimer = 0;
            if (isAnimFrame1) {
                setPixmap(QPixmap(":/assets/Ying2.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                isAnimFrame1 = false;
            } else {
                setPixmap(QPixmap(":/assets/Ying1.png").scaled(PLAYER_WIDTH, PLAYER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                isAnimFrame1 = true;
            }
        }
    }

    void startAccel() {
        isAccel = true;
        accelEndTime = QDateTime::currentMSecsSinceEpoch() + ACCEL_DURATION_MS;
    }

    void updateMove() {
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

    void spawnTrail() {
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

    void clearTrails() {
        while (!trailItems.empty()) {
            QGraphicsPixmapItem* t = trailItems.front();
            trailItems.pop();
            scene()->removeItem(t);
            delete t;
        }
        frameCount = 0;
    }

    void setMouseTarget(const QPointF& p) { targetPos = p; }
    void clampToScreen() {
        qreal nx = qBound(0.0, x(), (qreal)SCREEN_WIDTH - PLAYER_WIDTH);
        qreal ny = qBound(0.0, y(), (qreal)SCREEN_HEIGHT - PLAYER_HEIGHT);
        setPos(nx, ny);
    }
    ~Player() { clearTrails(); }
};

class Monster : public QGraphicsPixmapItem {
public:
    float dirX, dirY;
    int animFrameCounter;
    int currentAnimFrame;
    int animDir;
    QList<QPixmap> animFrames;
    bool isDead;
    qint64 deadTime;

    Monster() {
        animFrames << QPixmap(":/assets/monsterLeft.png")
        << QPixmap(":/assets/monsterLeft1.png")
        << QPixmap(":/assets/monster.png")
        << QPixmap(":/assets/monsterRight1.png")
        << QPixmap(":/assets/monsterRight.png");
        setPixmap(animFrames[2].scaled(MONSTER_SIZE, MONSTER_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        animFrameCounter = 0;
        currentAnimFrame = 2;
        animDir = 1;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> rx(0, SCREEN_WIDTH - MONSTER_SIZE);
        std::uniform_real_distribution<> ry(0, SCREEN_HEIGHT - MONSTER_SIZE);
        setPos(rx(gen), ry(gen));
        dirX = dirY = 0;
        isDead = false;
        deadTime = 0;
    }

    void startDeath() {
        if (isDead) return;
        isDead = true;
        deadTime = QDateTime::currentMSecsSinceEpoch();
        QPixmap deadPix(":/assets/dead.png");
        setPixmap(deadPix.scaled(MONSTER_DEAD_SIZE, MONSTER_DEAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    bool canDelete() const {
        if (!isDead) return false;
        return QDateTime::currentMSecsSinceEpoch() >= deadTime + DEATH_DELAY_MS;
    }

    void updateAnimation() {
        if (isDead) return;
        animFrameCounter++;
        if (animFrameCounter >= MONSTER_ANIM_INTERVAL) {
            animFrameCounter = 0;
            currentAnimFrame += animDir;
            if (currentAnimFrame <= 0) {
                currentAnimFrame = 0;
                animDir = 1;
            } else if (currentAnimFrame >= 4) {
                currentAnimFrame = 4;
                animDir = -1;
            }
            setPixmap(animFrames[currentAnimFrame].scaled(MONSTER_SIZE, MONSTER_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    void updateEscapeDir(qreal px, qreal py) {
        if (isDead) {
            dirX = 0;
            dirY = 0;
            return;
        }
        qreal dx = x() - px;
        qreal dy = y() - py;
        qreal dist = std::hypot(dx, dy);
        if (dist > MONSTER_ACTIVE_DISTANCE) {
            dirX = 0;
            dirY = 0;
            return;
        }
        qreal len = dist < 1e-5 ? 1 : dist;
        dirX = dx / len * MONSTER_SPEED;
        dirY = dy / len * MONSTER_SPEED;
    }

    void moveAway() {
        if (isDead) return;
        setPos(x() + dirX, y() + dirY);
    }

    bool isOutOfScreen() const {
        return x() < -MONSTER_SIZE || x() > SCREEN_WIDTH || y() < -MONSTER_SIZE || y() > SCREEN_HEIGHT;
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
    bool isSpawningPaused;
    qint64 spawnPauseEndTime;

    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmAudioOutput;

    GameScene() {
        setSceneRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        state = MENU_SELECT_TIME;
        player = nullptr;
        score = timeLeft = 0;
        selectedGameTime = 60;
        moveMode = MOVE_KEYBOARD;
        maxSkillPoint = 5;
        maxQEnergy = 100;
        isSpawningPaused = false;
        spawnPauseEndTime = 0;

        spawnTimer = new QTimer(this);
        gameTimer = new QTimer(this);
        countDownTimer = new QTimer(this);
        skillRegenTimer = new QTimer(this);

        connect(spawnTimer, &QTimer::timeout, this, &GameScene::spawnMonsters);
        connect(gameTimer, &QTimer::timeout, this, &GameScene::updateGame);
        connect(countDownTimer, &QTimer::timeout, this, &GameScene::updateTime);
        connect(skillRegenTimer, &QTimer::timeout, this, &GameScene::regenerateSkillPoint);

        initAudio();
        playRandomBGM();
        drawMenu();
    }

    void initAudio() {
        bgmAudioOutput = new QAudioOutput(this);
        bgmPlayer = new QMediaPlayer(this);
        bgmPlayer->setAudioOutput(bgmAudioOutput);
        bgmAudioOutput->setVolume(0.8);
        bgmPlayer->setLoops(QMediaPlayer::Infinite);

        connect(bgmPlayer, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error e, QString s){qDebug()<<"BGM_ERR:"<<s;});
    }

    void playRandomBGM() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> bgmRand(1,3);
        QString path = QString("qrc:/assets/bgm%1.mp3").arg(bgmRand(gen));
        bgmPlayer->setSource(QUrl(path));
        bgmPlayer->play();
    }


    void playMonsterDeathSound() {
        QAudioOutput *audioOut = new QAudioOutput(this);
        audioOut->setVolume(2.0);
        QMediaPlayer *snd = new QMediaPlayer(this);
        snd->setAudioOutput(audioOut);
        snd->setSource(QUrl("qrc:/assets/dead.WAV"));

        connect(snd, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState st){
            if(st == QMediaPlayer::StoppedState) {
                snd->deleteLater();
                audioOut->deleteLater();
            }
        });
        snd->play();
    }

    void playQSkillEffect(QPointF center) {
        QGraphicsEllipseItem* circle = new QGraphicsEllipseItem();
        circle->setPen(QPen(QColor(255, 105, 180, 200), 3));
        circle->setBrush(QColor(255, 183, 212, 80));
        circle->setRect(center.x(), center.y(), 0, 0);
        circle->setZValue(99);
        addItem(circle);

        QTimer* effectTimer = new QTimer(this);
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        connect(effectTimer, &QTimer::timeout, this, [=]() {
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
            float progress = qMin(1.0f, (float)elapsed / Q_EFFECT_DURATION);
            int currentRadius = Q_SKILL_MAX_RADIUS * progress;
            circle->setRect(center.x() - currentRadius, center.y() - currentRadius, currentRadius * 2, currentRadius * 2);
            circle->setOpacity(1.0 - progress);

            for (auto* item : items()) {
                if (auto* m = dynamic_cast<Monster*>(item)) {
                    if (!m->isDead) {
                        qreal mx = m->x() + MONSTER_SIZE / 2;
                        qreal my = m->y() + MONSTER_SIZE / 2;
                        qreal dist = std::hypot(mx - center.x(), my - center.y());
                        if (dist <= currentRadius) {
                            score += 10;
                            timeLeft += ADD_TIME_PER_KILL;
                            playMonsterDeathSound();
                            m->startDeath();
                        }
                    }
                }
            }
            if (progress >= 1.0) {
                effectTimer->stop();
                effectTimer->deleteLater();
                removeItem(circle);
                delete circle;
            }
        });
        effectTimer->start(16);
    }

    void startGame() {
        spawnTimer->stop(); gameTimer->stop(); countDownTimer->stop(); skillRegenTimer->stop();
        clear();
        state = GAME_PLAYING;
        score = 0;
        timeLeft = selectedGameTime;
        skillPoint = 3;
        qEnergy = 0;
        isSpawningPaused = false;
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
        menuText += QString("倒计时 按1：30秒 按2：60秒 \n当前模式：倒计时%1秒\n\n").arg(selectedGameTime);
        menuText += QString("按9：键盘移动 按0：鼠标移动\n当前操作：%1\n\n").arg(moveStr);
        menuText += "按Q范围清屏 按E 1秒加速冲刺\n按空格开始 ESC退出";
        auto* t = addText(menuText);
        t->setFont(QFont("Microsoft YaHei", 16));
        t->setPos(SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 120);
    }

    void spawnMonsters() {
        if (isSpawningPaused) {
            if (QDateTime::currentMSecsSinceEpoch() >= spawnPauseEndTime) {
                isSpawningPaused = false;
            } else {
                return;
            }
        }
        if (state != GAME_PLAYING || !player) return;
        int currentMonsterCount = 0;
        for (auto* item : items()) {
            if (auto* m = dynamic_cast<Monster*>(item)) {
                if (!m->isDead) currentMonsterCount++;
            }
        }
        if (currentMonsterCount >= MAX_MONSTER_COUNT) return;
        std::random_device rd; std::mt19937 gen(rd());
        std::uniform_int_distribution<> cnt(3,5);
        for(int i=0; i<cnt(gen); i++) addItem(new Monster());
    }

    void regenerateSkillPoint() {
        if (state != GAME_PLAYING) return;
        if (skillPoint < maxSkillPoint) skillPoint++;
    }

    void useAccel() {
        if (!player || skillPoint < 1) return;
        skillPoint--;
        player->startAccel();
    }

    void useQClear() {
        if (qEnergy < maxQEnergy || !player || state != GAME_PLAYING) return;
        qEnergy = 0;
        QPointF playerCenter = player->pos() + QPointF(PLAYER_WIDTH/2, PLAYER_HEIGHT/2);
        playQSkillEffect(playerCenter);
        isSpawningPaused = true;
        spawnPauseEndTime = QDateTime::currentMSecsSinceEpoch() + Q_EFFECT_DURATION + Q_SPAWN_PAUSE_DURATION;
    }

    void updateGame() {
        if (state != GAME_PLAYING || !player) return;
        player->updateMove();
        QList<Monster*> monsters;
        for (auto* item : items()) {
            if (auto* m = dynamic_cast<Monster*>(item)) {
                monsters.append(m);
                m->updateAnimation();
            }
        }
        for (auto* m : monsters) {
            m->updateEscapeDir(player->x(), player->y());
            m->moveAway();
            if (m->isDead) continue;
            if (m->collidesWithItem(player)) {
                score += 10;
                timeLeft += ADD_TIME_PER_KILL;
                if (qEnergy < maxQEnergy) qEnergy += 10;
                playMonsterDeathSound();
                removeItem(m);
                delete m;
            } else if (m->isOutOfScreen()) {
                removeItem(m);
                delete m;
            }
        }
        for (auto* m : monsters) {
            if (m->canDelete()) {
                removeItem(m);
                delete m;
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
        if (e->key() == Qt::Key_E) useAccel();
        if (e->key() == Qt::Key_Q) useQClear();
        if (e->key() == Qt::Key_Escape) backToMenu();
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

    ~GameScene() {
        delete bgmPlayer;
        delete bgmAudioOutput;
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