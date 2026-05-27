#pragma once
#include <graphics.h>
#include <stdbool.h>
#include <string.h>


#define WORLD_WIDTH  3000.0f  // 统一放在宪法里，改成大地图
#define WORLD_HEIGHT 3000.0f

typedef enum { STATE_MENU, STATE_CITY, STATE_COMBAT, STATE_GAMEOVER } AppState;
typedef enum { ROLE_NORMAL, ROLE_SUPER } HumanType;
typedef enum { STATE_IDLE, STATE_WORKING, STATE_FIGHTING } HumanState;

typedef struct Camera { float x, y; float zoom; } Camera;

// ---------- 人类数据结构 ----------
typedef struct Human {
    int id;
    char name[20];
    float world_x, world_y;
    int hp, max_hp;
    int atk, def;
    int hunger;
    int exp;
    int level;
    int is_superman;
    struct Human* prev;
    struct Human* next;
} Human;

// ---------- 建筑数据结构 ----------
typedef struct Building {
    int type; // 0=矿场, 1=伐木场, 2=熔炉
    int level;
    int cost_wood;
    int produce_rate;
} Building;

// ---------- 全局状态容器 ----------
typedef struct GameState {
    AppState current_state;
    Camera camera;

    int wood, coal, meat; // 加上了 meat
    int current_temp;     // 当前温度

    Human* head;          // 人类链表头指针
    Human* tail;          // 尾指针
    int population;       // 当前总人数

    Human* hovered_target;
} GameState;

extern GameState game;