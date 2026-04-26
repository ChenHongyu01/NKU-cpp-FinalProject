#pragma once
#include <QGraphicsScene>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QList>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include "game_constants.h"
#include "PacMan.h"
#include "Monster.h"
#include "Bomb.h"
#include "Player.h"

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
    RoleType selectedRole;
    QList<PacMan*> pacMen;
    QList<Bomb*> bombs;
    bool nextPacManRight;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmAudioOutput;
    QGraphicsPixmapItem* bgItem;

    GameScene();
    ~GameScene();
    void setBackground();
    void initAudio();
    void playRandomBGM();
    void playMonsterDeathSound();
    void playBombExplode(QPointF center);
    void playQSkillEffect(QPointF center);
    void useAccel();
    void useLangQ();
    void useQClear();
    void startGame();
    void backToMenu();
    void drawMenu();
    void spawnMonsters();
    void regenerateSkillPoint();
    void updateTime();
    void updateGame();
    void updateUI();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override;

private slots:
    void onSpawnTimerTimeout() { spawnMonsters(); }
    void onGameTimerTimeout() { updateGame(); }
    void onCountDownTimerTimeout() { updateTime(); }
    void onSkillRegenTimerTimeout() { regenerateSkillPoint(); }
};