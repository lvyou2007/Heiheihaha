#pragma once
#include <graphics.h>  // EasyX 图形库
#include <stdbool.h>
#include <string.h>

// 1. 全局枚举
typedef enum { STATE_MENU, STATE_CITY, STATE_COMBAT, STATE_GAMEOVER } AppState;
typedef enum { ROLE_NORMAL, ROLE_SUPER } HumanType;
typedef enum { STATE_IDLE, STATE_WORKING, STATE_FIGHTING } HumanState;

// 2. 摄像机结构体 (你负责的核心)
typedef struct Camera {
    float x, y;    // 摄像机左上角在世界中的坐标
    float zoom;    // 缩放比例 (默认 1.0)
} Camera;

// 3. 人类节点 (组员 C 的核心)
typedef struct Human {
    int id;
    char name[20];
    float world_x, world_y; // 物理世界坐标
    float hp, max_hp;
    float hunger;
    float atk, def;
    int level;
    HumanType type;
    HumanState state;
    struct Human* prev;
    struct Human* next;
} Human;

// 4. 野怪结构体 (组员 D 的核心)
typedef struct Monster {
    char name[20];
    float hp, max_hp;
    float atk, def;
} Monster;

// 5. 全局状态容器
typedef struct GameState {
    AppState current_state;
    Camera camera;
    
    int wood, coal, food;
    float env_temp;
    float furnace_temp;
    
    Human* human_list;      // 存活人员双向链表头指针
    int total_humans;
    
    Human* hovered_target;  // 当前鼠标悬浮选中的人 (组员 B 渲染面板用)
    
    Human* fighters[100];   // 遭遇战出战数组
    int fighter_count;
    Monster current_boss;   // 当前袭击的怪物
} GameState;

// 6. 声明全局变量 (只声明，在 main 中定义)
extern GameState game;
