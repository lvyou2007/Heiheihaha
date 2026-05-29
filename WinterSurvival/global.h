#pragma once
#include <graphics.h>
#include <stdbool.h>
#include <string.h>

#define WORLD_WIDTH  3000.0f  // 大地图宽度
#define WORLD_HEIGHT 3000.0f  // 大地图高度

typedef enum { STATE_MENU, STATE_CITY, STATE_COMBAT, STATE_GAMEOVER } AppState;
typedef enum { ROLE_NORMAL, ROLE_SUPER } HumanType;
typedef enum { STATE_IDLE, STATE_WORKING, STATE_FIGHTING } HumanState;

// ---------- 野怪类型枚举 ----------
typedef enum { MONSTER_PASSIVE, MONSTER_AGGRESSIVE } MonsterType;

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

// ---------- 野怪数据结构 (新增双向链表) ----------
typedef struct Monster {
    int id;
    char name[20];
    MonsterType type;       // 攻击性 (MONSTER_AGGRESSIVE) 或 无攻击性 (MONSTER_PASSIVE)
    float world_x, world_y; // 世界坐标
    int hp, max_hp;
    int atk, def;
    int meat_reward;        // 击杀产肉量
    int exp_reward;         // 击杀经验值
    struct Monster* prev;
    struct Monster* next;

} Monster;

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

    int wood, coal, meat;   // 基础资源

    // 温度拆分
    int env_temp;           // 环境温度 (随时间降低)
    int furnace_temp;       // 熔炉温度 (消耗煤炭维持)

    // 人类双向链表
    Human* head;
    Human* tail;
    int population;

    // 野怪双向链表 (新增)
    Monster* monster_head;
    Monster* monster_tail;
    int monster_count;

    // 悬浮判定目标
    Human* hovered_target;
    Monster* hovered_monster; // 新增：当前鼠标悬浮的野怪
    Monster current_boss;
} GameState;

extern GameState game;
// ---------- 全局建筑实例与升级指针声明 ----------
extern Building mine_build;
extern Building wood_build;
extern Building furnace_build;
extern Building* selected_building; // 指向当前玩家点击选中的建筑 (为 NULL 时不显示弹窗)