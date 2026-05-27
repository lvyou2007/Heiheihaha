// human_logic.h
#pragma once
#include <easyx.h>   // EasyX图形库
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define WORLD_WIDTH  800   // 世界地图宽度（逻辑单位）
#define WORLD_HEIGHT 600   // 世界地图高度（逻辑单位）

// ---------- 人类数据结构 ----------
typedef struct Human {
    int id;                 // 唯一ID
    char name[20];          // 名字（可随机生成）
    int hp, max_hp;         // 当前生命值，最大生命值
    int atk, def;           // 攻击力，防御力
    int hunger;             // 饱食度（0-100，0时开始扣血）
    int exp;                // 当前经验值
    int level;              // 等级
    int is_superman;        // 1=超凡人类，0=普通
    float world_x;      // 世界坐标 X
    float world_y;      // 世界坐标 Y
    struct Human* prev;     // 双向链表前驱
    struct Human* next;     // 双向链表后继
} Human;

// ---------- 建筑数据结构 ----------
typedef struct Building {
    int type;               // 0=矿场, 1=伐木场, 2=熔炉
    int level;
    int cost_wood;          // 升级所需木材
    int produce_rate;       // 每单位时间产量
} Building;

// ---------- 全局游戏状态 ----------
typedef struct Game {
    int wood;               // 木材数量
    int coal;               // 煤炭数量
    int meat;               // 肉类（食物）
    int current_temp;       // 当前温度（由熔炉等级决定）
    int day;                // 游戏天数
    int is_in_combat;       // 是否战斗中（由组员A控制）
    Human* head;            // 人类链表的头指针
    Human* tail;            // 人类链表的尾指针（可选）
    int population;         // 当前总人数（方便统计）
} Game;

extern Game game;   // 全局变量声明（定义放在 main.c 或 game.c）

void FreeAllHumans();   // 放在其他声明旁边
Human* CreateHuman(int base_level);
void AddHumanToList(Human* new_human);
void RemoveHumanFromList(Human* target);
void PrintAllHumans();

// 核心生命周期（简化版，稍后完善）
void DailySettlement();

// 辅助：获取参战人员（供组员D调用，先留空实现）
Human** GetFighters(int* out_count);
//注意：这个函数返回的 fighters 数组是动态分配的，调用者（组员D）用完必须 free(fighters)。你可以在代码注释里提醒他。
Human* GetHoveredHuman(float click_world_x, float click_world_y, float radius);
void InitHumanList();