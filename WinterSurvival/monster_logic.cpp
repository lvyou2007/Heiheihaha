// monster_logic.cpp
// 独立的野怪生态系统实现 (采用无漂移正弦哈希散步算法)
#define _CRT_SECURE_NO_WARNINGS
#include "monster_logic.h"
#include <stdlib.h>
#include <math.h>
#include <windows.h>

static int active_combat_monster_id = 0; // 记录当前正在和人类大战的野怪 ID

// 辅助计算人类平均等级
static int GetAverageHumanLevel() {
    if (game.population == 0) return 1;
    int sum = 0;
    Human* cur = game.head;
    while (cur) {
        sum += cur->level;
        cur = cur->next;
    }
    return sum / game.population;
}

// 刷新野怪 (数量与总人口正相关，实力与平均等级正相关)
void SpawnMonsters() {
    int max_monsters = 5 + (game.population / 2);
    if (game.monster_count >= max_monsters) return;

    int spawn_chance = 40 + game.population * 3;
    if (spawn_chance > 80) spawn_chance = 80;

    if (rand() % 100 < spawn_chance) {
        Monster* m = (Monster*)malloc(sizeof(Monster));
        if (!m) return;

        static int monster_id_generator = 1001;
        m->id = monster_id_generator++;
        m->prev = m->next = NULL;

        int avg_level = GetAverageHumanLevel();

        if (rand() % 100 < 70) {
            m->type = MONSTER_PASSIVE;
            strcpy(m->name, "温顺驯鹿");
            m->max_hp = m->hp = 50 + (avg_level - 1) * 15;
            m->atk = 0;
            m->def = 3 + (avg_level - 1) * 2;
            m->meat_reward = 60 + (avg_level - 1) * 15; // 升级产肉，防止大面积挨饿
            m->exp_reward = 15 + (avg_level - 1) * 5;
        }
        else {
            m->type = MONSTER_AGGRESSIVE;
            strcpy(m->name, "暴雪饿狼");
            m->max_hp = m->hp = 100 + (avg_level - 1) * 25;
            m->atk = 28 + (avg_level - 1) * 4;
            m->def = 8 + (avg_level - 1) * 3;
            m->meat_reward = 40 + (avg_level - 1) * 10;
            m->exp_reward = 45 + (avg_level - 1) * 8;
        }

        // 随机在地图边缘生成
        if (rand() % 2 == 0) {
            m->world_x = (float)(rand() % 3000);
            m->world_y = (rand() % 2 == 0) ? 50.0f : 2950.0f;
        }
        else {
            m->world_x = (rand() % 2 == 0) ? 50.0f : 2950.0f;
            m->world_y = (float)(rand() % 3000);
        }

        // 插入野怪双向链表
        if (game.monster_head == NULL) {
            game.monster_head = game.monster_tail = m;
        }
        else {
            game.monster_tail->next = m;
            m->prev = game.monster_tail;
            game.monster_tail = m;
        }
        game.monster_count++;
    }
}

// 智能追击与平滑随机移动 (与人类散步算法高度一致，在 60 FPS 下运行)
void UpdateMonsters() {
    Monster* cur = game.monster_head;
    while (cur != NULL) {
        Monster* next = cur->next;

        float speed = 1.0f; // 适中移动速度

        if (cur->type == MONSTER_PASSIVE) {
            // A. 温顺野怪：使用无相关性哈希漫步算法在 $3000 \times 3000$ 大地图随机漫步
            int time_cycle = (int)(GetTickCount() / 3000);
            double raw = sin(cur->id * 17.1414 + time_cycle * 53.987) * 23145.1245;
            double fraction = raw - floor(raw);
            float angle = (float)(fraction * 2.0 * 3.14159265);

            cur->world_x += cosf(angle) * speed;
            cur->world_y += sinf(angle) * speed;
        }
        else if (cur->type == MONSTER_AGGRESSIVE) {
            // B. 暴雪饿狼：寻找距离最近的存活人类
            Human* closest_human = NULL;
            float min_dist = 400.0f;

            Human* h = game.head;
            while (h != NULL) {
                if (h->hp > 0) {
                    float dx = h->world_x - cur->world_x;
                    float dy = h->world_y - cur->world_y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest_human = h;
                    }
                }
                h = h->next;
            }

            if (closest_human != NULL) {
                // 朝着最近人类的方向奔袭
                float dx = closest_human->world_x - cur->world_x;
                float dy = closest_human->world_y - cur->world_y;
                float dist = sqrtf(dx * dx + dy * dy);

                cur->world_x += (dx / dist) * 1.5f; // 追击移速提高
                cur->world_y += (dy / dist) * 1.5f;

                // === 桥梁对接：当狼触碰人类时，自动将数据写入组员 D 的卡槽，拉起回合制战斗弹窗 ===
                if (dist < 25.0f) {
                    active_combat_monster_id = cur->id; // 锁定这只狼的 ID

                    // 将这只狼的属性完好无损地拷贝给组员 D 的战斗卡槽
                    strcpy(game.current_boss.name, cur->name);
                    game.current_boss.hp = cur->hp;
                    game.current_boss.max_hp = cur->max_hp;
                    game.current_boss.atk = cur->atk;
                    game.current_boss.def = cur->def;

                    game.current_state = STATE_COMBAT; // 切换为战斗大弹窗状态
                }
            }
            else {
                // 没有嗅探到人类时，野外随机踱步
                int time_cycle = (int)(GetTickCount() / 4000);
                double raw = sin(cur->id * 23.9876 + time_cycle * 31.5432) * 53214.1235;
                double fraction = raw - floor(raw);
                float angle = (float)(fraction * 2.0 * 3.14159265);

                cur->world_x += cosf(angle) * speed;
                cur->world_y += sinf(angle) * speed;
            }
        }

        // 边界保护
        if (cur->world_x < 50.0f) cur->world_x = 50.0f;
        if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
        if (cur->world_y < 50.0f) cur->world_y = 50.0f;
        if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;

        cur = next;
    }
}

// 检查并清除已被击毙的实体野狼
void CheckAndRemoveDeadMonster() {
    // 如果战斗中，怪物死亡
    if (active_combat_monster_id != 0 && game.current_boss.hp <= 0) {
        Monster* cur = game.monster_head;
        while (cur != NULL) {
            if (cur->id == active_combat_monster_id) {
                // 1. 战后清理：从大地图双向链表中永久删除这只野怪
                if (cur->prev != NULL) cur->prev->next = cur->next;
                else game.monster_head = cur->next;

                if (cur->next != NULL) cur->next->prev = cur->prev;
                else game.monster_tail = cur->prev;

                free(cur);
                game.monster_count--;
                break;
            }
            cur = cur->next;
        }
        active_combat_monster_id = 0; // 重置
    }
}

Monster* GetHoveredMonster(float click_world_x, float click_world_y, float radius) {
    Monster* closest = NULL;
    float min_dist_sq = radius * radius;

    Monster* cur = game.monster_head;
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