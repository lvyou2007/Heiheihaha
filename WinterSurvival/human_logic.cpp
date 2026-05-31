#define _CRT_SECURE_NO_WARNINGS
#include "human_logic.h"
#include <stdio.h>   
#include <stdlib.h>
#include <math.h>

static void RandomName(char* name) {
    const char* first[] = { "勇者", "智者", "迅捷", "沉稳", "狂热" };
    const char* last[] = { "战士", "猎人", "工匠", "农夫", "祭司" };
    sprintf(name, "%s%s", first[rand() % 5], last[rand() % 5]);
}


void InitHumanList() {
    game.head = game.tail = NULL;
    game.population = 0;
    // 可选：清空其他全局状态
}

// 把原来的 UpdateLifeCycle 直接改名为 DailySettlement
Human* CreateHuman(int base_level) {
    Human* h = (Human*)malloc(sizeof(Human));
    static int next_id = 1;
    h->id = next_id++;
    RandomName(h->name);

    h->max_hp = 80 + base_level * 10;
    h->hp = h->max_hp;
    h->atk = 10 + base_level * 2;
    h->def = 5 + base_level;
    h->hunger = 100;
    h->exp = 0;
    h->level = base_level;
    h->is_superman = 0;
    h->target_monster = NULL;
    h->is_selected = 0;
    h->prev = h->next = NULL;


    if (rand() % 100 < 5) {
        h->is_superman = 1;
        h->max_hp = (int)(h->max_hp * 1.5);
        h->hp = h->max_hp;
        h->atk = (int)(h->atk * 1.5);
        h->def = (int)(h->def * 1.5);
    }
    // === 核心修改：新生儿坐标主要集中在大熔炉 (1500, 1500) 附近半径 200 像素内 ===
    float angle = (float)(rand() % 360) * 3.1415926f / 180.0f; // 随机 360 度方向
    float radius = (float)(rand() % 200);                      // 0 到 200 像素随机半径
    h->world_x = 1500.0f + cosf(angle) * radius;
    h->world_y = 1500.0f + sinf(angle) * radius;

    return h;
}

void AddHumanToList(Human* new_human) {
    if (game.head == NULL) {
        game.head = game.tail = new_human;
    }
    else {
        game.tail->next = new_human;
        new_human->prev = game.tail;
        game.tail = new_human;
    }
    game.population++;
}
void RemoveHumanFromList(Human* target) {
    if (target == NULL) return;

    // 让前一个节点跳过 target
    if (target->prev != NULL) {
        target->prev->next = target->next;
    }
    else {
        // target 是头节点
        game.head = target->next;
    }

    // 让后一个节点跳过 target
    if (target->next != NULL) {
        target->next->prev = target->prev;
    }
    else {
        // target 是尾节点
        game.tail = target->prev;
    }

    free(target);
    game.population--;
}

void FreeAllHumans() {
    Human* cur = game.head;
    while (cur != NULL) {
        Human* next = cur->next;   // 先保存下一个节点
        free(cur);                 // 释放当前节点
        cur = next;                // 移到下一个
    }
    game.head = game.tail = NULL;
    game.population = 0;
}
void PrintAllHumans() {
    Human* cur = game.head;
    while (cur) {
        printf("ID:%d %s HP:%d\n", cur->id, cur->name, cur->hp);
        cur = cur->next;
    }
}
void DailySettlement() {
    Human* cur = game.head;
    while (cur != NULL) {
        Human* next = cur->next;
        // 1. 饱食度下降
        cur->hunger -= 10;
        // 自动吃肉
        if (cur->hunger < 80 && game.meat > 0) {
            game.meat--;
            cur->hunger += 10;
            if (cur->hunger > 100) cur->hunger = 100;
        }
        if (cur->hunger < 0) cur->hunger = 0;
        if (cur->hunger == 0) cur->hp -= 5;

        // 2. 温度影响
        int effective_temp = game.env_temp + (int)(game.furnace_temp * 0.2);
        if (effective_temp < 10) cur->hp -= 3;

        // 3. 经验升级
        if (cur->exp >= 100) {
            cur->level++;
            cur->exp -= 100;
            cur->max_hp += 20;
            cur->atk += 5;
            cur->def += 3;
            cur->hp = cur->max_hp;
        }
        if (cur->hp > cur->max_hp) cur->hp = cur->max_hp;

        // 4. 死亡判定
        if (cur->hp <= 0) {
            RemoveHumanFromList(cur);
        }

        cur = next;
    }

    // ========== 移出循环的代码 ==========
    // 环境温度下降（每天一次）
    if (game.env_temp > -45) {
        game.env_temp -= 3;
        if (game.env_temp < -45) game.env_temp = -45;
    }
    // 建筑资源产出（每天一次）
    game.wood += wood_build.produce_rate;
    game.coal += mine_build.produce_rate;
    // ==================================

    // 每日自动采集食物
    game.meat += game.population * 3;

    // 熔炉燃料消耗
    int coal_needed = furnace_build.level * 4;
    if (game.coal >= coal_needed) {
        game.coal -= coal_needed;
        game.furnace_temp = furnace_build.produce_rate;
    }
    else {
        game.coal = 0;
        game.furnace_temp = 0;
    }

    // 游戏结束判定
    if (game.population <= 0) {
        game.current_state = STATE_GAMEOVER;
    }

    // 人口繁衍
    if (game.population > 0) {
        int new_count = (int)(game.population * 0.05);
        if (new_count < 1 && game.population > 0) new_count = 1;
        for (int i = 0; i < new_count; i++) {
            Human* baby = CreateHuman(1);
            AddHumanToList(baby);
        }
    }
}

Human** GetFighters(int* out_count) {
    if (game.population == 0) {
        *out_count = 0;
        return NULL;
    }

    int needed = (int)(game.population * 0.3);
    if (needed < 1) needed = 1;

    // 分配返回的指针数组
    Human** fighters = (Human**)malloc(sizeof(Human*) * needed);

    // 临时数组存放所有人指针
    Human** all = (Human**)malloc(sizeof(Human*) * game.population);
    int idx = 0;
    Human* cur = game.head;
    while (cur != NULL) {
        all[idx++] = cur;
        cur = cur->next;
    }

    // Fisher-Yates 随机打乱
    for (int i = game.population - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Human* tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }

    // 取前 needed 个
    for (int i = 0; i < needed; i++) {
        fighters[i] = all[i];
    }

    free(all);
    *out_count = needed;
    return fighters;
}
void GetAverageAttributes(int* avg_atk, int* avg_def, int* avg_hp) {
    if (game.population == 0) {
        *avg_atk = *avg_def = *avg_hp = 10;
        return;
    }
    int sum_atk = 0, sum_def = 0, sum_hp = 0;
    Human* cur = game.head;
    while (cur) {
        sum_atk += cur->atk;
        sum_def += cur->def;
        sum_hp += cur->max_hp;
        cur = cur->next;
    }
    *avg_atk = sum_atk / game.population;
    *avg_def = sum_def / game.population;
    *avg_hp = sum_hp / game.population;
}
Human* GetHoveredHuman(float click_world_x, float click_world_y, float radius) {
    if (game.head == NULL) return NULL;

    Human* closest = NULL;
    float min_dist_sq = radius * radius;   // 平方比较，避免开方

    Human* cur = game.head;
    while (cur != NULL) {
        float dx = cur->world_x - click_world_x;
        float dy = cur->world_y - click_world_y;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = cur;
        }
        cur = cur->next;
    }
    return closest;
}
// === 新增：全图高精平滑散步算法 (60 FPS 每帧调用) ===：由于LCG（线性同余）随机数发生器相关性漂移，人物经常往一个方向运动
#include <math.h>

// === 升级：全图无漂移、无碰撞干涉的完全随机漫步算法 (60 FPS) ===
//void UpdateHumanPositions() {
    //Human* cur = game.head;
   // while (cur != NULL) {
    //    float speed = 1.5f; // 散步速度（适中）

        // 1. 每 2.5 秒 (2500毫秒) 换一次散步方向
    //    int time_cycle = (int)(GetTickCount() / 2500);

        // 2. 使用高度离散的数学哈希噪点公式 (Shader 经典算法)
        // 彻底绕过 MSVC 编译器的 srand 线性相关性 Bug，为每个 ID 产生绝对独立的随机方向
    //    double raw = sin(cur->id * 12.9898 + time_cycle * 78.233) * 43758.5453123;
    //    double fraction = raw - floor(raw); // 取小数部分 [0.0, 1.0)
    //    float angle = (float)(fraction * 2.0 * 3.1415926535); // 转换为 [0, 2*PI] 弧度

        // 3. 平滑应用位移
   //     cur->world_x += cosf(angle) * speed;
    //    cur->world_y += sinf(angle) * speed;

        // 4. 全图 $3000 \times 3000$ 边界硬约束，防止小人走出世界
    //    if (cur->world_x < 100.0f) cur->world_x = 100.0f;
    //    if (cur->world_x > 2900.0f) cur->world_x = 2900.0f;
    //    if (cur->world_y < 100.0f) cur->world_y = 100.0f;
     //   if (cur->world_y > 2900.0f) cur->world_y = 2900.0f;

    //    cur = cur->next;
  //  }
//}