// main.cpp
// 最终大一统集成版 (免菜单直入 + 人类自动集结战友、围攻饿狼的 RTS 即时战略版)
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

// ---------- 2. 人类游走、追鹿、以及【核心新增：战友遇袭全军集结包抄】 AI ----------
void UpdateHumanPositions() {
    // 1. 扫描整个战场，检查当前是否有任何一个战友正在被“暴雪饿狼”近身攻击
    Monster* active_fight_wolf = NULL;
    float fight_x = 0, fight_y = 0;

    Monster* m_find = game.monster_head;
    while (m_find != NULL) {
        if (m_find->type == MONSTER_AGGRESSIVE && m_find->hp > 0) {
            Human* h_check = game.head;
            while (h_check != NULL) {
                if (h_check->hp > 0) {
                    float d_x = h_check->world_x - m_find->world_x;
                    float d_y = h_check->world_y - m_find->world_y;
                    // 如果狼距离小人小于 100 像素，视为战役打响
                    if (d_x * d_x + d_y * d_y <= 100.0f * 100.0f) {
                        active_fight_wolf = m_find;
                        fight_x = m_find->world_x;
                        fight_y = m_find->world_y;
                        break;
                    }
                }
                h_check = h_check->next;
            }
        }
        if (active_fight_wolf) break; // 优先响应第一处战场
        m_find = m_find->next;
    }

    // 2. 驱动每个人类个体的行为
    Human* cur = game.head;
    while (cur != NULL) {
        float speed = 1.2f;

        // 判定 A：【民兵集结指令】如果前方正在打仗，且自身处于集结半径 (600像素) 内，放弃一切事务，急速（2.0移速）赶往现场！
        if (active_fight_wolf != NULL) {
            float f_dx = fight_x - cur->world_x;
            float f_dy = fight_y - cur->world_y;
            float f_dist = sqrtf(f_dx * f_dx + f_dy * f_dy);

            if (f_dist < 600.0f) {
                cur->world_x += (f_dx / f_dist) * 2.0f; // 集结冲锋
                cur->world_y += (f_dy / f_dist) * 2.0f;

                cur = cur->next;
                continue; // 成功分发集结，跳过日常散步和捕鹿
            }
        }

        // 判定 B：若无战争，扫描周围 300 像素内是否有可猎杀的驯鹿 (MONSTER_PASSIVE)
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

        if (target_deer != NULL) {
            // 【狩猎追击】：以 1.5 倍高速度 (1.8f) 追击野鹿
            float dx = target_deer->world_x - cur->world_x;
            float dy = target_deer->world_y - cur->world_y;
            float dist = sqrtf(dx * dx + dy * dy);

            cur->world_x += (dx / dist) * 1.8f;
            cur->world_y += (dy / dist) * 1.8f;

            if (dist < 25.0f) {
                target_deer->hp -= cur->atk; // 小人进行攻击

                if (target_deer->hp <= 0) {
                    game.meat += target_deer->meat_reward; // 爆肉
                    cur->exp += target_deer->exp_reward;   // 拿经验

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
            // C. 【休闲漫步】
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

// ---------- 3. 野怪 AI 刷新与大地图【核心新增：联军多人围殴机制】 ----------

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
                // === 数值削减：将饿狼初始属性大幅度削减，确保前期小人们能战胜它 ===
                m->max_hp = m->hp = 55 + (avg_level - 1) * 15; // 基础生命 55 
                m->atk = 12 + (avg_level - 1) * 3;             // 基础攻击降为 12 点
                m->def = 3 + (avg_level - 1) * 2;              // 基础防御降为 3 点
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

// 智能追击与大地图即时遭遇战 (每 500ms 结算一轮伤害)
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

                // === RTS 模式：直接在大地图上发生贴身攻防，每 500ms 进行一轮多人均摊伤害和群殴围攻 ===
                if (dist < 25.0f && combat_tick) {

                    // 1. 狼狠狠地咬一口被追击的那个人类
                    int dmg_to_human = cur->atk - closest_human->def;
                    if (dmg_to_human < 1) dmg_to_human = 1;
                    closest_human->hp -= dmg_to_human;

                    // 2. 【核心新增：多人联军围攻逻辑】
                    // 遍历所有存活的人类，只要他们赶到了现场（距离这只饿狼小于 30 像素），就会同时挥刀反击围攻饿狼！
                    Human* h_fight = game.head;
                    while (h_fight != NULL) {
                        if (h_fight->hp > 0) {
                            float f_dx = h_fight->world_x - cur->world_x;
                            float f_dy = h_fight->world_y - cur->world_y;
                            float f_dist = sqrtf(f_dx * f_dx + f_dy * f_dy);

                            if (f_dist <= 30.0f) { // 成功加入围攻圈
                                int dmg_to_monster = h_fight->atk - cur->def;
                                if (dmg_to_monster < 1) dmg_to_monster = 1;
                                cur->hp -= dmg_to_monster; // 多人围攻，饿狼血量急剧下降！

                                h_fight->exp += 2; // 围殴过程中获得助攻微量经验
                            }
                        }
                        h_fight = h_fight->next;
                    }

                    // 3. 击毙结算
                    if (cur->hp <= 0) {
                        game.meat += cur->meat_reward; // 爆肉食
                        closest_human->exp += cur->exp_reward; // 猎手拿走主要经验

                        // 人类升级判定
                        Human* h_up = game.head;
                        while (h_up) {
                            if (h_up->exp >= 100) {
                                h_up->level++;
                                h_up->exp -= 100;
                                h_up->max_hp += 20;
                                h_up->atk += 5;
                                h_up->def += 3;
                                h_up->hp = h_up->max_hp;
                            }
                            h_up = h_up->next;
                        }

                        // 将这只死狼安全从链表抹去
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
            // 1. 人类平滑游走、且靠近黄色驯鹿或遭遇战争时会自动跑步包抄围歼！
            UpdateHumanPositions();

            // 2. 500ms 一周期的大地图即时伤害判定节拍
            bool combat_tick = false;
            if (current_time - last_combat_round_time >= 500) {
                combat_tick = true;
                last_combat_round_time = current_time;
            }

            // 3. 野怪 AI 与大地图遭遇围殴
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