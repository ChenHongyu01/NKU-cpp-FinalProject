#include "GameScene.h"
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QFont>
#include <QPixmap>
#include <random>
#include <cmath>
#include <QDateTime>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QApplication>

GameScene::GameScene() {
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
    selectedRole = ROLE_YING;
    nextPacManRight = false;
    bgItem = nullptr;

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
    setBackground();
    drawMenu();
}

GameScene::~GameScene() {
    qDeleteAll(pacMen);
    qDeleteAll(bombs);
    delete bgmPlayer;
    delete bgmAudioOutput;
}

void GameScene::setBackground() {
    if (bgItem) removeItem(bgItem);
    bgItem = new QGraphicsPixmapItem();
    QPixmap bg(":/assets/background.png");
    bgItem->setPixmap(bg.scaled(SCREEN_WIDTH, SCREEN_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    bgItem->setZValue(-100);
    bgItem->setPos(0,0);
    addItem(bgItem);
}

void GameScene::initAudio() {
    bgmAudioOutput = new QAudioOutput(this);
    bgmPlayer = new QMediaPlayer(this);
    bgmPlayer->setAudioOutput(bgmAudioOutput);
    bgmAudioOutput->setVolume(0.8);
    bgmPlayer->setLoops(QMediaPlayer::Infinite);
    connect(bgmPlayer, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error, QString s){ qDebug() << "BGM错误：" << s; });
}

void GameScene::playRandomBGM() {
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> bgmRand(1,3);
    bgmPlayer->setSource(QUrl(QString("qrc:/assets/bgm%1.mp3").arg(bgmRand(gen))));
    bgmPlayer->play();
}

void GameScene::playMonsterDeathSound() {
    QAudioOutput *audioOut = new QAudioOutput(this);
    audioOut->setVolume(2.0);
    QMediaPlayer *snd = new QMediaPlayer(this);
    snd->setAudioOutput(audioOut);
    snd->setSource(QUrl("qrc:/assets/dead.WAV"));
    connect(snd, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState st){
        if(st == QMediaPlayer::StoppedState) { snd->deleteLater(); audioOut->deleteLater(); }
    });
    snd->play();
}

void GameScene::playBombExplode(QPointF center) {
    QGraphicsEllipseItem* circle = new QGraphicsEllipseItem();
    circle->setPen(QPen(QColor(204, 153, 255, 200), 3));
    circle->setBrush(QColor(220, 180, 255, 80));
    circle->setRect(center.x(), center.y(), 0, 0);
    circle->setZValue(99);
    addItem(circle);

    QTimer* effectTimer = new QTimer(this);
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    connect(effectTimer, &QTimer::timeout, this, [=]() {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
        float progress = qMin(1.0f, (float)elapsed / Q_EFFECT_DURATION);
        int currentRadius = BOMB_EXPLODE_RADIUS * progress;
        circle->setRect(center.x() - currentRadius, center.y() - currentRadius, currentRadius * 2, currentRadius * 2);
        circle->setOpacity(1.0 - progress);

        for (auto* item : items()) {
            Monster* m = dynamic_cast<Monster*>(item);
            if (m != nullptr && !m->isDead) {
                qreal dist = std::hypot(m->x() + MONSTER_SIZE/2 - center.x(), m->y() + MONSTER_SIZE/2 - center.y());
                if (dist <= currentRadius) {
                    score += 10;
                    timeLeft += ADD_TIME_PER_KILL;
                    playMonsterDeathSound();
                    removeItem(m);
                    delete m;
                }
            }
        }
        if (progress >= 1.0) { effectTimer->stop(); effectTimer->deleteLater(); removeItem(circle); delete circle; }
    });
    effectTimer->start(16);
}

void GameScene::playQSkillEffect(QPointF center) {
    if (selectedRole != ROLE_YING) return;
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
            Monster* m = dynamic_cast<Monster*>(item);
            if (m != nullptr && !m->isDead) {
                qreal dist = std::hypot(m->x() + MONSTER_SIZE/2 - center.x(), m->y() + MONSTER_SIZE/2 - center.y());
                if (dist <= currentRadius) {
                    score += 10;
                    timeLeft += ADD_TIME_PER_KILL;
                    m->startDeath();
                    playMonsterDeathSound();
                }
            }
        }
        if (progress >= 1.0) { effectTimer->stop(); effectTimer->deleteLater(); removeItem(circle); delete circle; }
    });
    effectTimer->start(16);
}

void GameScene::useAccel() {
    if (!player || skillPoint < 1) return;
    skillPoint--;

    if (selectedRole == ROLE_YING) {
        player->startAccel();
    } else {
        PacMan* pm = new PacMan(!nextPacManRight, player->y());
        pacMen.append(pm);
        addItem(pm);
        nextPacManRight = !nextPacManRight;
    }
}

void GameScene::useLangQ() {
    if (qEnergy < maxQEnergy) return;
    qEnergy = 0;
    if (skillPoint < maxSkillPoint) skillPoint++;

    for (int i = 0; i < LANG_Q_SPAWN_BOMB_COUNT; i++) {
        int delay = i * LANG_Q_BOMB_INTERVAL;
        QTimer::singleShot(delay, this, [=]() {
            if (!player || state != GAME_PLAYING) return;
            QPointF bombPos = player->pos() + QPointF(PLAYER_WIDTH/2 - BOMB_SIZE/2, PLAYER_HEIGHT/2 - BOMB_SIZE/2);
            Bomb* bomb = new Bomb(bombPos);
            bombs.append(bomb);
            addItem(bomb);
        });
    }
}

void GameScene::useQClear() {
    if (!player || state != GAME_PLAYING) return;

    if (selectedRole == ROLE_YING) {
        if (qEnergy < maxQEnergy) return;
        qEnergy = 0;
        QPointF center = player->pos() + QPointF(PLAYER_WIDTH/2, PLAYER_HEIGHT/2);
        playQSkillEffect(center);
        isSpawningPaused = true;
        spawnPauseEndTime = QDateTime::currentMSecsSinceEpoch() + Q_EFFECT_DURATION + Q_SPAWN_PAUSE_DURATION;
    } else if (selectedRole == ROLE_LANG) {
        useLangQ();
    }
}

void GameScene::startGame() {
    spawnTimer->stop(); gameTimer->stop(); countDownTimer->stop(); skillRegenTimer->stop();
    qDeleteAll(pacMen); qDeleteAll(bombs); pacMen.clear(); bombs.clear();
    clear();
    setBackground();
    state = GAME_PLAYING;
    score = 0;
    timeLeft = selectedGameTime;
    skillPoint = 3;
    qEnergy = 0;
    isSpawningPaused = false;
    spawnPauseEndTime = 0;
    player = new Player(selectedRole);
    player->moveMode = moveMode;
    addItem(player);
    spawnTimer->start(500);
    gameTimer->start(16);
    countDownTimer->start(1000);
    skillRegenTimer->start(5000);
}

void GameScene::backToMenu() {
    spawnTimer->stop(); gameTimer->stop(); countDownTimer->stop(); skillRegenTimer->stop();
    qDeleteAll(pacMen); qDeleteAll(bombs); pacMen.clear(); bombs.clear();
    state = GAME_OVER;
    clear();
    setBackground();
    player = nullptr;
    drawMenu();
}

void GameScene::drawMenu() {
    for (auto* item : items()) {
        if ((dynamic_cast<QGraphicsTextItem*>(item) || dynamic_cast<QGraphicsPixmapItem*>(item) || dynamic_cast<QGraphicsRectItem*>(item)) && item != bgItem) {
            removeItem(item);
            delete item;
        }
    }

    //背景
    QGraphicsRectItem* blackBg = new QGraphicsRectItem(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    blackBg->setBrush(Qt::black);
    blackBg->setPen(Qt::NoPen);
    addItem(blackBg);
    //名字
    auto* title = addText("星琼大作战");
    title->setFont(QFont("Microsoft YaHei", 34, QFont::Bold));
    title->setDefaultTextColor(Qt::white);
    title->setPos(SCREEN_WIDTH/2 - title->boundingRect().width()/2, 60);
    // 得分/状态
    QString stateText = (state == GAME_OVER) ? QString("最终得分：%1").arg(score) : "准备开始游戏";
    auto* stateTip = addText(stateText);
    stateTip->setFont(QFont("Microsoft YaHei", 18));
    stateTip->setDefaultTextColor(Qt::white);
    stateTip->setPos(SCREEN_WIDTH/2 - stateTip->boundingRect().width()/2, 140);
    // 左右技能描述区
    const int DESC_X_LEFT = 120;
    const int DESC_X_RIGHT = SCREEN_WIDTH - 320;
    const int DESC_Y = 220;


    auto* yingDesc = addText("【绯英 技能】\n\nQ：能量100\n范围清屏\nE：1战技点\n加速冲刺");
    yingDesc->setFont(QFont("Microsoft YaHei", 14));
    yingDesc->setDefaultTextColor(Qt::white);
    yingDesc->setPos(DESC_X_LEFT, DESC_Y);

    auto* langDesc = addText("【银狼LV999 技能】\n\nQ：能量100\n召唤追踪炸弹\nE：1战技点\n召唤平移吃豆人");
    langDesc->setFont(QFont("Microsoft YaHei", 14));
    langDesc->setDefaultTextColor(Qt::white);
    langDesc->setPos(DESC_X_RIGHT, DESC_Y);

    // 中间角色预览（选中放大）
    const int SIZE_BIG = 130;
    const int SIZE_SML = 65;
    int roleY = 240;

    // Ying
    QGraphicsPixmapItem* pYing = new QGraphicsPixmapItem;
    QPixmap picYing(":/assets/Ying.png");
    if (selectedRole == ROLE_YING) {
        pYing->setPixmap(picYing.scaled(SIZE_BIG, SIZE_BIG, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pYing->setPos(SCREEN_WIDTH/2 - 170, roleY);
    } else {
        pYing->setPixmap(picYing.scaled(SIZE_SML, SIZE_SML, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pYing->setPos(SCREEN_WIDTH/2 - 135, roleY + 32);
    }
    addItem(pYing);

    // Lang
    QGraphicsPixmapItem* pLang = new QGraphicsPixmapItem;
    QPixmap picLang(":/assets/Lang.png");
    if (selectedRole == ROLE_LANG) {
        pLang->setPixmap(picLang.scaled(SIZE_BIG, SIZE_BIG, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pLang->setPos(SCREEN_WIDTH/2 + 40, roleY);
    } else {
        pLang->setPixmap(picLang.scaled(SIZE_SML, SIZE_SML, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pLang->setPos(SCREEN_WIDTH/2 + 70, roleY + 32);
    }
    addItem(pLang);

    // 角色切换提示
    auto* roleTip = addText("按 Y / L 切换角色");
    roleTip->setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    roleTip->setDefaultTextColor(Qt::yellow);
    roleTip->setPos(SCREEN_WIDTH/2 - roleTip->boundingRect().width()/2, 400);

    // 实时显示当前选择
    int y =440;
    QString strTime = QString("当前游戏一局倒计时：%1 秒").arg(selectedGameTime);
    auto* textTime = addText(strTime);
    textTime->setFont(QFont("Microsoft YaHei", 14));
    textTime->setDefaultTextColor(Qt::white);
    textTime->setPos(SCREEN_WIDTH/2 - textTime->boundingRect().width()/2, y);

    auto* tipTime = addText("按1 = 30秒  |  按2 = 60秒");
    tipTime->setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    tipTime->setDefaultTextColor(Qt::yellow);
    tipTime->setPos(SCREEN_WIDTH/2 - tipTime->boundingRect().width()/2, y+40);

    // 当前移动方式
    QString strMove = moveMode == MOVE_KEYBOARD ? "当前移动：键盘" : "当前移动：鼠标";
    auto* textMove = addText(strMove);
    textMove->setFont(QFont("Microsoft YaHei", 14));
    textMove->setDefaultTextColor(Qt::white);
    textMove->setPos(SCREEN_WIDTH/2 - textMove->boundingRect().width()/2, y+80);

    // 设置说明
    auto* tipMove = addText("按9 = 键盘awsd或上下左右  |  按0 = 跟随鼠标");
    tipMove->setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    tipMove->setDefaultTextColor(Qt::yellow);
    tipMove->setPos(SCREEN_WIDTH/2 - tipMove->boundingRect().width()/2, y + 120);

    auto* tipStart = addText("空格键开始   ESC退出");
    tipStart->setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
    tipStart->setDefaultTextColor(Qt::white);
    tipStart->setPos(SCREEN_WIDTH/2 - tipStart->boundingRect().width()/2, y + 200);

    auto* tipGame = addText("每5秒回复1战技点,每捡1星琼加10分和10点Q充能");
    tipGame->setFont(QFont("Microsoft YaHei", 14));
    tipGame->setDefaultTextColor(Qt::white);
    tipGame->setPos(SCREEN_WIDTH/2 - tipGame->boundingRect().width()/2, y + 160);
}

void GameScene::spawnMonsters() {
    if (state != GAME_PLAYING || !player) return;
    if (isSpawningPaused) {
        if (QDateTime::currentMSecsSinceEpoch() >= spawnPauseEndTime) {
            isSpawningPaused = false;
            spawnPauseEndTime = 0;
        } else return;
    }

    int aliveCount = 0;
    for (auto* item : items()) {
        Monster* m = dynamic_cast<Monster*>(item);
        if (m && !m->isDead) aliveCount++;
    }
    if (aliveCount >= MAX_MONSTER_COUNT) return;

    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> randCnt(3,5);
    for(int i=0; i<randCnt(gen); i++) addItem(new Monster());
}

void GameScene::regenerateSkillPoint() {
    if (state == GAME_PLAYING && skillPoint < maxSkillPoint) skillPoint++;
}

void GameScene::updateTime() {
    if (state == GAME_PLAYING && --timeLeft <= 0) backToMenu();
}

void GameScene::updateGame() {
    if (state != GAME_PLAYING || !player) return;
    player->updateMove();

    for (auto it = pacMen.begin(); it != pacMen.end();) {
        PacMan* pm = *it;
        pm->updateMove();

        QList<Monster*> toEat;
        for (auto* item : items()) {
            Monster* m = dynamic_cast<Monster*>(item);
            if (m != nullptr && !m->isDead && pm->collidesWithItem(m)) {
                toEat.append(m);
            }
        }

        for (auto* m : toEat) {
            score += 10;
            if (qEnergy < maxQEnergy) qEnergy += 10;
            playMonsterDeathSound();
            removeItem(m);
            delete m;
        }


        if (pm->isOut()) { removeItem(pm); delete pm; it = pacMen.erase(it); }
        else ++it;
    }

    for (auto it = bombs.begin(); it != bombs.end();) {
        Bomb* b = *it;
        b->updateMove();
        bool exploded = false;
        for (auto* item : items()) {
            Monster* m = dynamic_cast<Monster*>(item);
            if (m != nullptr && !m->isDead && b->collidesWithItem(m)) {
                playBombExplode(b->pos() + QPointF(BOMB_SIZE/2, BOMB_SIZE/2));
                removeItem(b);
                delete b;
                exploded = true;
                break;
            }
        }
        if (exploded) { it = bombs.erase(it); continue; }
        if (b->isOut()) { removeItem(b); delete b; it = bombs.erase(it); }
        else ++it;
    }

    QList<Monster*> toDelete;
    for (auto* item : items()) {
        Monster* m = dynamic_cast<Monster*>(item);
        if (!m) continue;

        m->updateEscapeDir(player->x(), player->y());
        m->moveAway();
        m->updateAnimation();

        if (!m->isDead && m->collidesWithItem(player)) {
            score += 10;
            if (qEnergy < maxQEnergy) qEnergy += 10;
            playMonsterDeathSound();
            removeItem(m);
            delete m;
        } else if (m->isOutOfScreen() || m->canDelete()) {
            toDelete.append(m);
        }
    }
    for (auto* m : toDelete) { removeItem(m); delete m; }

    updateUI();
}

void GameScene::updateUI() {
    for (auto* item : items()) {
        if (auto* t = dynamic_cast<QGraphicsTextItem*>(item)) { removeItem(item); delete item; }
    }
    // 倒计时
    QGraphicsTextItem* timeText = addText(QString("倒计时：%1s").arg(timeLeft));
    timeText->setDefaultTextColor(Qt::white);
    timeText->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    timeText->setPos(10,10);

    // 得分
    QGraphicsTextItem* scoreText = addText(QString("得分：%1").arg(score));
    scoreText->setDefaultTextColor(Qt::white);
    scoreText->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    scoreText->setPos(SCREEN_WIDTH-200,10);

    // 技能点
    QGraphicsTextItem* skillText = addText(QString("战技点：%1/%2").arg(skillPoint).arg(maxSkillPoint));
    skillText->setDefaultTextColor(Qt::white);
    skillText->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    skillText->setPos(SCREEN_WIDTH/2-300,10);

    // Q充能
    QGraphicsTextItem* qEnergyText = addText(QString("Q充能：%1/%2").arg(qEnergy).arg(maxQEnergy));
    qEnergyText->setDefaultTextColor(Qt::white);
    qEnergyText->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    qEnergyText->setPos(SCREEN_WIDTH/2+50,10);
}

void GameScene::keyPressEvent(QKeyEvent* e) {
    if (state == MENU_SELECT_TIME || state == GAME_OVER) {
        if (e->key() == Qt::Key_Y) { selectedRole = ROLE_YING; drawMenu(); }
        if (e->key() == Qt::Key_L) { selectedRole = ROLE_LANG; drawMenu(); }
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
        if (e->key() == Qt::Key_Up) player->keyUp = true;
        if (e->key() == Qt::Key_Down) player->keyDown = true;
        if (e->key() == Qt::Key_Left) player->keyLeft = true;
        if (e->key() == Qt::Key_Right) player->keyRight = true;
    }
}

void GameScene::keyReleaseEvent(QKeyEvent* e) {
    if (!player || state != GAME_PLAYING || player->moveMode != MOVE_KEYBOARD) return;
    if (e->key() == Qt::Key_W) player->keyW = false;
    if (e->key() == Qt::Key_S) player->keyS = false;
    if (e->key() == Qt::Key_A) player->keyA = false;
    if (e->key() == Qt::Key_D) player->keyD = false;
    if (e->key() == Qt::Key_Up) player->keyUp = false;
    if (e->key() == Qt::Key_Down) player->keyDown = false;
    if (e->key() == Qt::Key_Left) player->keyLeft = false;
    if (e->key() == Qt::Key_Right) player->keyRight = false;
}

void GameScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
    if (state == GAME_PLAYING && player && player->moveMode == MOVE_MOUSE)
        player->setMouseTarget(e->scenePos());
}