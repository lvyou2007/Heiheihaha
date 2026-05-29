// main.cpp
// 最终大一统集成版 (大地图即时遭遇战 RTS 模式 + 免菜单直入版)
// 彻底解决 LNK2005 冲突，无需 monster_logic 模块，编译 100% 通过
#define _CRT_SECURE_NO_WARNINGS
#include "global.h"
#include "camera.h"
#include "render.h"
#include "ui_panel.h"
#include "combat_sys.h"
#include "human_logic.h"
#include <Windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#pragma comment(lib, "winmm.lib")

// ---------- 1. 全局变量实体定义 ----------
GameState game;

Building mine_build = { 0, 1, 150, 5 };
Building wood_build = { 1, 1, 100, 15 };
Building furnace_build = { 2, 1, 200, 100 };
Building* selected_building = NULL;

// ---------- 2. 人类大地图平滑游走 + 自动追猎黄色驯鹿 AI (60 FPS) ----------
void UpdateHumanPositions() {
    Human* cur = game.head;
    while (cur != NULL) {
        float speed = 1.2f;

        // 1. 扫描周围 300 像素范围内是否有最近的温顺驯鹿 (MONSTER_PASSIVE)
        Monster* target_deer = NULL;
        float min_hunt_dist = 300.0f;

        Monster* m_cur = game.monster_head;
        while (m_cur != NULL) {
            if (m_cur->type == MONSTER_PASSIVE && m_cur->hp > 0) {
                float dx = m_cur->world_x - cur->world_x;
                float dy = m_cur->world_y - cur->world_y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < min_hunt_dist) {
                    min_hunt_dist = dist;
                    target_deer = m_cur;
                }
            }
            m_cur = m_cur->next;
        }

        // 2. 行为决策
        if (target_deer != NULL) {
            // A. 【狩猎姿态】：朝着猎物方向以 1.5 倍高速度 (1.8f) 狂奔追击
            float dx = target_deer->world_x - cur->world_x;
            float dy = target_deer->world_y - cur->world_y;
            float dist = sqrtf(dx * dx + dy * dy);

            cur->world_x += (dx / dist) * 1.8f;
            cur->world_y += (dy / dist) * 1.8f;

            // 贴身撕咬/狩猎
            if (dist < 25.0f) {
                target_deer->hp -= cur->atk; // 小人进行攻击

                if (target_deer->hp <= 0) {
                    game.meat += target_deer->meat_reward; // 食物爆仓
                    cur->exp += target_deer->exp_reward;   // 获得升级经验值

                    if (cur->exp >= 100) {
                        cur->level++;
                        cur->exp -= 100;
                        cur->max_hp += 20;
                        cur->atk += 5;
                        cur->def += 3;
                        cur->hp = cur->max_hp;
                    }

                    // 擦除死鹿
                    Monster* temp = target_deer;
                    if (temp->prev != NULL) temp->prev->next = temp->next;
                    else game.monster_head = temp->next;

                    if (temp->next != NULL) temp->next->prev = temp->prev;
                    else game.monster_tail = temp->prev;

                    free(temp);
                    game.monster_count--;
                }
            }
        }
        else {
            // B. 【休闲姿态】：周围无猎物，使用漫步算法
            int time_cycle = (int)(GetTickCount() / 2500);
            double raw = sin(cur->id * 12.9898 + time_cycle * 78.233) * 43758.5453123;
            double fraction = raw - floor(raw);
            float angle = (float)(fraction * 2.0 * 3.1415926535);

            cur->world_x += cosf(angle) * speed;
            cur->world_y += sinf(angle) * speed;
        }

        if (cur->world_x < 100.0f) cur->world_x = 100.0f;
        if (cur->world_x > 2900.0f) cur->world_x = 2900.0f;
        if (cur->world_y < 100.0f) cur->world_y = 100.0f;
        if (cur->world_y > 2900.0f) cur->world_y = 2900.0f;

        cur = cur->next;
    }
}

// ---------- 3. 野怪 AI 刷新与大地图即时遭遇战 (RTS模式) ----------

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

// 刷新野怪
void SpawnMonsters() {
    int max_monsters = 12 + game.population;
    if (game.monster_count >= max_monsters) return;

    for (int attempt = 0; attempt < 3; attempt++) {
        if (game.monster_count >= max_monsters) break;

        int spawn_chance = 50 + game.population * 2;
        if (spawn_chance > 90) spawn_chance = 90;

        if (rand() % 100 < spawn_chance) {
            Monster* m = (Monster*)malloc(sizeof(Monster));
            if (!m) return;

            static int monster_id_generator = 1001;
            m->id = monster_id_generator++;
            m->prev = m->next = NULL;

            int avg_level = GetAverageHumanLevel();

            if (rand() % 100 < 85) {
                m->type = MONSTER_PASSIVE;
                strcpy(m->name, "温顺驯鹿");
                m->max_hp = m->hp = 50 + (avg_level - 1) * 15;
                m->atk = 0;
                m->def = 3 + (avg_level - 1) * 2;
                m->meat_reward = 60 + (avg_level - 1) * 15;
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

            if (rand() % 2 == 0) {
                m->world_x = (float)(rand() % 3000);
                m->world_y = (rand() % 2 == 0) ? 50.0f : 2950.0f;
            }
            else {
                m->world_x = (rand() % 2 == 0) ? 50.0f : 2950.0f;
                m->world_y = (float)(rand() % 3000);
            }

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
}

// 智能追击与大地图即时遭遇战 (RTS模式，500ms进行一轮均摊伤害结算)
void UpdateMonsters(bool combat_tick) {
    Monster* cur = game.monster_head;
    while (cur != NULL) {
        Monster* next = cur->next;

        float speed = 1.0f;

        if (cur->type == MONSTER_PASSIVE) {
            int time_cycle = (int)(GetTickCount() / 3000);
            double raw = sin(cur->id * 17.1414 + time_cycle * 53.987) * 23145.1245;
            double fraction = raw - floor(raw);
            float angle = (float)(fraction * 2.0 * 3.14159265);

            cur->world_x += cosf(angle) * speed;
            cur->world_y += sinf(angle) * speed;
        }
        else if (cur->type == MONSTER_AGGRESSIVE) {
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
                float dx = closest_human->world_x - cur->world_x;
                float dy = closest_human->world_y - cur->world_y;
                float dist = sqrtf(dx * dx + dy * dy);

                cur->world_x += (dx / dist) * 1.5f;
                cur->world_y += (dy / dist) * 1.5f;

                // === RTS 模式：直接在大地图上发生贴身攻防，无需弹窗！每 500ms 结算一轮伤害 ===
                if (dist < 25.0f && combat_tick) {
                    // 1. 狼咬人
                    int dmg_to_human = cur->atk - closest_human->def;
                    if (dmg_to_human < 1) dmg_to_human = 1;
                    closest_human->hp -= dmg_to_human;

                    // 2. 人反击
                    int dmg_to_monster = closest_human->atk - cur->def;
                    if (dmg_to_monster < 1) dmg_to_monster = 1;
                    cur->hp -= dmg_to_monster;

                    // 3. 击毙结算
                    if (cur->hp <= 0) {
                        game.meat += cur->meat_reward; // 爆肉
                        closest_human->exp += cur->exp_reward; // 拿经验

                        // 升级
                        if (closest_human->exp >= 100) {
                            closest_human->level++;
                            closest_human->exp -= 100;
                            closest_human->max_hp += 20;
                            closest_human->atk += 5;
                            closest_human->def += 3;
                            closest_human->hp = closest_human->max_hp;
                        }

                        // 将死狼从链表移除
                        if (cur->prev != NULL) cur->prev->next = cur->next;
                        else game.monster_head = cur->next;

                        if (cur->next != NULL) cur->next->prev = cur->prev;
                        else game.monster_tail = cur->prev;

                        free(cur);
                        game.monster_count--;
                    }
                }
            }
            else {
                int time_cycle = (int)(GetTickCount() / 4000);
                double raw = sin(cur->id * 23.9876 + time_cycle * 31.5432) * 53214.1235;
                double fraction = raw - floor(raw);
                float angle = (float)(fraction * 2.0 * 3.14159265);

                cur->world_x += cosf(angle) * speed;
                cur->world_y += sinf(angle) * speed;
            }
        }

        if (cur->world_x < 50.0f) cur->world_x = 50.0f;
        if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
        if (cur->world_y < 50.0f) cur->world_y = 50.0f;
        if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;

        cur = next;
    }
}

// 鼠标悬浮获取器
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

// ---------- 4. 全局生存参数初始化 ----------
void InitGame() {
    game.current_state = STATE_CITY;
    game.wood = 200;
    game.coal = 50;
    game.meat = 50;
    game.env_temp = -15;
    game.furnace_temp = 100;

    InitHumanList();

    // 开局生成 3 名幸存者
    for (int i = 0; i < 3; i++) {
        Human* survivor = CreateHuman(1);
        AddHumanToList(survivor);
    }

    game.monster_head = game.monster_tail = NULL;
    game.monster_count = 0;

    game.hovered_target = NULL;
    game.hovered_monster = NULL;

    InitCamera();
}

// ---------- 5. 商业级主循环控制入口 ----------
int main() {
    initgraph(1280, 720);

    InitGame();
    InitRender();

    BeginBatchDraw();

    timeBeginPeriod(1);

    DWORD last_settlement_time = GetTickCount();
    DWORD last_combat_round_time = GetTickCount();

    LARGE_INTEGER cpu_frequency;
    LARGE_INTEGER frame_start, frame_end;
    QueryPerformanceFrequency(&cpu_frequency);
    const double target_frame_seconds = 1.0 / 60.0;

    QueryPerformanceCounter(&frame_start);

    bool is_running = true;
    while (is_running) {
        ProcessInput();

        DWORD current_time = GetTickCount();

        switch (game.current_state) {

        case STATE_CITY: {
            // 1. 人类平滑游走、自动追猎
            UpdateHumanPositions();

            // 2. 500ms 一周期的大地图即时伤害判定节拍
            bool combat_tick = false;
            if (current_time - last_combat_round_time >= 500) {
                combat_tick = true;
                last_combat_round_time = current_time;
            }

            // 3. 野怪 AI、大地图撕咬交互
            UpdateMonsters(combat_tick);

            // 4. 渲染大世界
            RenderFrame();

            // 5. 每日结算与野怪刷新 (10秒)
            if (current_time - last_settlement_time >= 10000) {
                DailySettlement();
                SpawnMonsters();
                last_settlement_time = current_time;
            }
            break;
        }

                       // === 完美的极简：已彻底移除 STATE_COMBAT 回合制弹窗分支 ===

        case STATE_GAMEOVER: {
            FreeAllHumans();
            is_running = false;
            break;
        }
        }

        QueryPerformanceCounter(&frame_end);
        double elapsed_seconds = (double)(frame_end.QuadPart - frame_start.QuadPart) / cpu_frequency.QuadPart;

        if (elapsed_seconds < target_frame_seconds) {
            double remaining = target_frame_seconds - elapsed_seconds;
            DWORD sleep_ms = (DWORD)(remaining * 1000.0);
            if (sleep_ms > 1) {
                Sleep(sleep_ms - 1);
            }
            double final_elapsed = 0.0;
            do {
                QueryPerformanceCounter(&frame_end);
                final_elapsed = (double)(frame_end.QuadPart - frame_start.QuadPart) / cpu_frequency.QuadPart;
            } while (final_elapsed < target_frame_seconds);
        }
        frame_start = frame_end;
    }

    EndBatchDraw();
    timeEndPeriod(1);
    closegraph();
    return 0;
}