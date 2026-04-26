#pragma once
#include <QGraphicsPixmapItem>

// 全局可调常量
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 900;
const int PLAYER_WIDTH  = 60;
const int PLAYER_HEIGHT = 80;
const int MONSTER_SIZE = 48;
const int MONSTER_DEAD_SIZE = 72;
const int PACMAN_SIZE = 80;
const int BOMB_SIZE = 80;
const int BOMB_EXPLODE_RADIUS = 150;
const int LANG_Q_SPAWN_BOMB_COUNT = 5;
const int LANG_Q_BOMB_INTERVAL = 400;
const float PLAYER_SPEED = 5.0f;
const float MONSTER_SPEED = 2.0f;
const float PACMAN_SPEED = 8.0f;
const float BOMB_SPEED = 6.0f;
const int ADD_TIME_PER_KILL = 0;
const float ACCEL_RATE = 2.0f;
const int ACCEL_DURATION_MS = 1500;
const int TRAIL_MAX = 5;
const int TRAIL_SPAWN_FRAME = 3;
const float TRAIL_ALPHA = 0.25f;
const float MONSTER_ACTIVE_DISTANCE = 100.0f;
const int MAX_MONSTER_COUNT = 20;
const int MONSTER_ANIM_INTERVAL = 5;
const int DEATH_DELAY_MS = 500;
const int Q_SKILL_MAX_RADIUS = 400;
const int Q_EFFECT_DURATION = 250;
const int Q_SPAWN_PAUSE_DURATION = 500;

// 枚举定义
enum GameState { MENU_SELECT_TIME, GAME_PLAYING, GAME_OVER };
enum MoveMode { MOVE_KEYBOARD, MOVE_MOUSE };
enum RoleType { ROLE_YING, ROLE_LANG };

// 前置声明
class Monster;
class Bomb;
class PacMan;
class GameScene;
class Player;