#define _CRT_SECURE_NO_WARNINGS
#include "human_logic.h"
#include <stdio.h>   
#include <stdlib.h>


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
    h->prev = h->next = NULL;

    if (rand() % 100 < 5) {
        h->is_superman = 1;
        h->max_hp = (int)(h->max_hp * 1.5);
        h->hp = h->max_hp;
        h->atk = (int)(h->atk * 1.5);
        h->def = (int)(h->def * 1.5);
    }
    // 随机分配坐标
    h->world_x = (float)(rand() % (int)WORLD_WIDTH);
    h->world_y = (float)(rand() % (int)WORLD_HEIGHT);
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
        Human* next = cur->next;  // 因为可能删除cur，先存下一个

        // === 新增：让人物每天随机朝某个方向走动几步 ===
       // 随机产生 -10 到 10 之间的世界位移
        float dx = (float)((rand() % 21) - 10);
        float dy = (float)((rand() % 21) - 10);
        cur->world_x += dx;
        cur->world_y += dy;

        // 限制不要走成出世界边界 (0,0) 到 (800, 600)
        if (cur->world_x < 0) cur->world_x = 0;
        if (cur->world_x > 800) cur->world_x = 800;
        if (cur->world_y < 0) cur->world_y = 0;
        if (cur->world_y > 600) cur->world_y = 600;

        // 1. 饱食度下降（每天-10）
        cur->hunger -= 10;
        if (cur->hunger < 0) cur->hunger = 0;
        if (cur->hunger == 0) {
            cur->hp -= 5;   // 饥饿扣血
        }

        // 2. 温度影响（如果熔炉等级低，温度 < 10 就扣血）
        int effective_temp = game.env_temp + (int)(game.furnace_temp * 0.2); // 计算体感温度
        if (effective_temp < 10) {
            cur->hp -= 3; // 只有体感温度低于 10 度时才会冻伤扣血
        }

        // 3. 经验升级（满100经验升级）
        if (cur->exp >= 100) {
            cur->level++;
            cur->exp -= 100;
            cur->max_hp += 20;
            cur->atk += 5;
            cur->def += 3;
            cur->hp = cur->max_hp;   // 升级回满血
            // 限制最大值（可选）
            if (cur->hp > cur->max_hp) cur->hp = cur->max_hp;
        }

        // 4. 生命值上限保护与死亡判定
        if (cur->hp > cur->max_hp) cur->hp = cur->max_hp;
        if (cur->hp <= 0) {
            RemoveHumanFromList(cur);   // 死亡，从链表中删除
            // 注意：cur 已经被 free，不能再访问，但 next 已经保存好了
        }

        cur = next;
    }

    // 5. 人口自然繁衍（每天增加当前人口的5%）
    if (game.population > 0) {
        int new_count = (int)(game.population * 0.05);
        if (new_count < 1 && game.population > 0) new_count = 1;
        for (int i = 0; i < new_count; i++) {
            Human* baby = CreateHuman(1);   // 等级设为1，CreateHuman内部会处理超凡概率
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