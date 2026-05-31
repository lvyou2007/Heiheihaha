// main.cpp
// 最终大一统集成版 (免菜单直入 + 人类自动集结战友、围攻饿狼的 RTS 即时战略版)
#define _CRT_SECURE_NO_WARNINGS
#include "global.h"
#include "camera.h"
#include "render.h"
#include "ui_panel.h"
#include "combat_sys.h"
#include "human_logic.h"
#include "audio.h"
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
    Human* cur = game.head;
    while (cur != NULL) {
        float speed = 1.2f;
        float target_x = 0, target_y = 0;
        bool has_target = false;

        // ========= 最高优先级：玩家手动指派的目标 =========
        // ========= 最高优先级：玩家手动指派的目标 =========
        if (cur->target_monster != NULL && cur->target_monster->hp > 0) {
            float dx = cur->target_monster->world_x - cur->world_x;
            float dy = cur->target_monster->world_y - cur->world_y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 25.0f) {
                // 移动
                cur->world_x += (dx / dist) * speed;
                cur->world_y += (dy / dist) * speed;
            }
            else {
                // 攻击冷却检查（500毫秒）
                DWORD now = GetTickCount();
                if (now - cur->last_attack_time >= 500) {
                    cur->last_attack_time = now;

                    int dmg = cur->atk - cur->target_monster->def;
                    if (dmg < 1) dmg = 1;
                    cur->target_monster->hp -= dmg;

                    // 攻击后微调后退效果
                    cur->world_x -= (dx / dist) * 5.0f;
                    cur->world_y -= (dy / dist) * 5.0f;

                    // 如果怪物死亡，进行结算
                    if (cur->target_monster->hp <= 0) {
                        // 1. 清除所有指向这只怪物的人类（包括当前攻击者和其他人）
                        Monster* dying = cur->target_monster;
                        Human* h_clear = game.head;
                        while (h_clear) {
                            if (h_clear->target_monster == dying) {
                                h_clear->target_monster = NULL;
                                h_clear->is_selected = 0;
                            }
                            h_clear = h_clear->next;
                        }

                        // 2. 结算奖励
                        game.meat += dying->meat_reward;
                        cur->exp += dying->exp_reward;
                        if (cur->exp >= 100) {
                            cur->level++;
                            cur->exp -= 100;
                            cur->max_hp += 20;
                            cur->atk += 5;
                            cur->def += 3;
                            cur->hp = cur->max_hp;
                        }

                        // 3. 从怪物链表中删除 dying
                        if (dying->prev) dying->prev->next = dying->next;
                        else game.monster_head = dying->next;
                        if (dying->next) dying->next->prev = dying->prev;
                        else game.monster_tail = dying->prev;
                        free(dying);
                        game.monster_count--;
                    }
                }
            }
            // 边界约束（保持不变）
            if (cur->world_x < 50.0f) cur->world_x = 50.0f;
            if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
            if (cur->world_y < 50.0f) cur->world_y = 50.0f;
            if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;
            cur = cur->next;
            continue;
        }
        // ========= 第二优先级：自动集结响应战场 =========
        else {
            Monster* active_wolf = NULL;
            float fight_x = 0, fight_y = 0;
            // 扫描是否有饿狼正在攻击人类（简化：找任意 hp>0 的饿狼且有人类在其 100 半径内）
            Monster* m = game.monster_head;
            while (m != NULL) {
                if (m->type == MONSTER_AGGRESSIVE && m->hp > 0) {
                    Human* h = game.head;
                    while (h != NULL) {
                        if (h->hp > 0) {
                            float dx = h->world_x - m->world_x;
                            float dy = h->world_y - m->world_y;
                            if (dx * dx + dy * dy <= 100.0f * 100.0f) {
                                active_wolf = m;
                                fight_x = m->world_x;
                                fight_y = m->world_y;
                                break;
                            }
                        }
                        h = h->next;
                    }
                }
                if (active_wolf) break;
                m = m->next;
            }
            if (active_wolf != NULL) {
                // 如果在战场 600 像素内，则集结
                float dx = fight_x - cur->world_x;
                float dy = fight_y - cur->world_y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 600.0f) {
                    target_x = fight_x;
                    target_y = fight_y;
                    has_target = true;
                    speed = 2.0f;   // 冲锋速度
                }
            }
        }

        // 如果已经通过上述两个优先级找到了目标，则移动
        if (has_target) {
            float dx = target_x - cur->world_x;
            float dy = target_y - cur->world_y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 1.0f) {
                cur->world_x += (dx / dist) * speed;
                cur->world_y += (dy / dist) * speed;
            }
            // 边界约束
            if (cur->world_x < 50.0f) cur->world_x = 50.0f;
            if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
            if (cur->world_y < 50.0f) cur->world_y = 50.0f;
            if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;
            cur = cur->next;
            continue;
        }

        // ========= 第三优先级：自动狩猎驯鹿 =========
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
            float dx = target_deer->world_x - cur->world_x;
            float dy = target_deer->world_y - cur->world_y;
            float dist = sqrtf(dx * dx + dy * dy);
            cur->world_x += (dx / dist) * 1.8f;
            cur->world_y += (dy / dist) * 1.8f;
            if (dist < 25.0f) {
                // 攻击驯鹿（代码同你原来的）
                target_deer->hp -= cur->atk;
                if (target_deer->hp <= 0) {
                    game.meat += target_deer->meat_reward;
                    cur->exp += target_deer->exp_reward;
                    // 升级逻辑（可复用原代码）
                    // ...
                    // 删除死鹿
                    if (target_deer->prev) target_deer->prev->next = target_deer->next;
                    else game.monster_head = target_deer->next;
                    if (target_deer->next) target_deer->next->prev = target_deer->prev;
                    else game.monster_tail = target_deer->prev;
                    free(target_deer);
                    game.monster_count--;
                }
            }
            // 边界约束
            if (cur->world_x < 50.0f) cur->world_x = 50.0f;
            if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
            if (cur->world_y < 50.0f) cur->world_y = 50.0f;
            if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;
            cur = cur->next;
            continue;
        }

        // ========= 第四优先级：随机漫步 =========
        int time_cycle = (int)(GetTickCount() / 2500);
        double raw = sin(cur->id * 12.9898 + time_cycle * 78.233) * 43758.5453123;
        double fraction = raw - floor(raw);
        float angle = (float)(fraction * 2.0 * 3.1415926535);
        cur->world_x += cosf(angle) * speed;
        cur->world_y += sinf(angle) * speed;

        // 边界约束
        if (cur->world_x < 50.0f) cur->world_x = 50.0f;
        if (cur->world_x > 2950.0f) cur->world_x = 2950.0f;
        if (cur->world_y < 50.0f) cur->world_y = 50.0f;
        if (cur->world_y > 2950.0f) cur->world_y = 2950.0f;

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
                m->atk = 5;
                m->def = 3 + (avg_level - 1) * 2;
                m->meat_reward = 60 + (avg_level - 1) * 15;
                m->exp_reward = 15 + (avg_level - 1) * 5;
            }
            else {
                m->type = MONSTER_AGGRESSIVE;
                strcpy(m->name, "暴雪饿狼");
                // === 数值削减：将饿狼初始属性大幅度削减，确保前期小人们能战胜它 ===
                m->max_hp = m->hp = 55 + (avg_level - 1) * 15; // 基础生命 55 
                m->atk = 35 + (avg_level - 1) * 3;             // 基础攻击提升为 35 点
                m->def = 5 + (avg_level - 1) * 2;              // 基础防御降为 5 点
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
// 智能追击与大地图即时遭遇战 (每 500ms 结算一轮伤害)
void UpdateMonsters(bool combat_tick) {
    Monster* cur = game.monster_head;
    while (cur != NULL) {
        Monster* next = cur->next;
        float speed = 1.0f;

        if (cur->type == MONSTER_PASSIVE) {
            // === 被动野怪：随机漫步（您的原有逻辑） ===
            int time_cycle = (int)(GetTickCount() / 3000);
            double raw = sin(cur->id * 17.1414 + time_cycle * 53.987) * 23145.1245;
            double fraction = raw - floor(raw);
            float angle = (float)(fraction * 2.0 * 3.14159265);
            cur->world_x += cosf(angle) * speed;
            cur->world_y += sinf(angle) * speed;
        }
        else if (cur->type == MONSTER_AGGRESSIVE) {
            // === 主动野怪：追击人类 ===
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

                if (dist < 25.0f && combat_tick) {
                    // 1. 狼攻击被追击的人类
                    int dmg_to_human = cur->atk - closest_human->def;
                    if (dmg_to_human < 1) dmg_to_human = 1;
                    closest_human->hp -= dmg_to_human;

                    // 2. 多人围攻（距离小于30像素的人类全部反击）
                    Human* h_fight = game.head;
                    while (h_fight != NULL) {
                        if (h_fight->hp > 0) {
                            float f_dx = h_fight->world_x - cur->world_x;
                            float f_dy = h_fight->world_y - cur->world_y;
                            float f_dist = sqrtf(f_dx * f_dx + f_dy * f_dy);
                            if (f_dist <= 30.0f) {
                                int dmg_to_monster = h_fight->atk - cur->def;
                                if (dmg_to_monster < 1) dmg_to_monster = 1;
                                cur->hp -= dmg_to_monster;
                                h_fight->exp += 2;
                            }
                        }
                        h_fight = h_fight->next;
                    }

                    // 3. 怪物死亡处理
                    if (cur->hp <= 0) {
                        // 清除所有指向这只怪物的人类（光圈清除）
                        Human* h_clear = game.head;
                        while (h_clear != NULL) {
                            if (h_clear->target_monster == cur) {
                                h_clear->target_monster = NULL;
                                h_clear->is_selected = 0;
                            }
                            h_clear = h_clear->next;
                        }

                        game.meat += cur->meat_reward;
                        closest_human->exp += cur->exp_reward;

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

                        // 从链表中删除怪物
                        if (cur->prev) cur->prev->next = cur->next;
                        else game.monster_head = cur->next;
                        if (cur->next) cur->next->prev = cur->prev;
                        else game.monster_tail = cur->prev;
                        free(cur);
                        game.monster_count--;
                    }
                }
            }
            else {
                // 无人类目标时随机漫步
                int time_cycle = (int)(GetTickCount() / 4000);
                double raw = sin(cur->id * 23.9876 + time_cycle * 31.5432) * 53214.1235;
                double fraction = raw - floor(raw);
                float angle = (float)(fraction * 2.0 * 3.14159265);
                cur->world_x += cosf(angle) * speed;
                cur->world_y += sinf(angle) * speed;
            }
        }

        // 边界约束
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
    game.is_selecting_target = 0;
    game.pending_attack_monster = NULL;

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

    // ---------- 背景音乐（循环播放）----------
    PlayBGM("bgm.mp3");   // 确保 bgm.mp3 在 exe 同目录或项目目录下

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
            // 释放所有人类内存
            FreeAllHumans();
            // 释放所有怪物内存
            Monster* m = game.monster_head;
            while (m != NULL) {
                Monster* next = m->next;
                free(m);
                m = next;
            }
            game.monster_head = game.monster_tail = NULL;
            game.monster_count = 0;

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