// main.cpp
// 最终大一统集成版 (免菜单直入版)：包含游戏核心循环、双缓冲、状态机以及完整的野怪智能 AI 与生态系统
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

// 链接 Windows 多媒体库以提升 Sleep 精度
#pragma comment(lib, "winmm.lib")

// ---------- 1. 全局变量实体定义 ----------
GameState game;

// 建筑配置自适应 1~3 级数值平衡
Building mine_build = { 0, 1, 150, 5 };
Building wood_build = { 1, 1, 100, 15 };
Building furnace_build = { 2, 1, 200, 100 };
Building* selected_building = NULL;

// ---------- 2. 核心集成：原本属于野怪模块的全部 AI 算法实体 ----------

static int active_combat_monster_id = 0; // 记录当前交战野怪 ID

// 计算当前人类平均等级
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

// 刷新野怪 (数量与总人口正相关，温顺动物高刷新率，解决挨饿问题)
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

// 智能追猎与平滑随机移动 (每帧 60 FPS 调用)
void UpdateMonsters() {
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

                if (dist < 25.0f) {
                    active_combat_monster_id = cur->id;

                    strcpy(game.current_boss.name, cur->name);
                    game.current_boss.hp = cur->hp;
                    game.current_boss.max_hp = cur->max_hp;
                    game.current_boss.atk = cur->atk;
                    game.current_boss.def = cur->def;

                    game.current_state = STATE_COMBAT;

                    // 碰触瞬间，强制在后台执行一次战斗判定，瞬间完成战士和生命值初始化
                    ProcessCombatRound();
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

// 检查并清除已被击毙或驱逐的实体野狼 (防原地遇敌死锁)
void CheckAndRemoveDeadMonster() {
    if (active_combat_monster_id != 0 && game.current_boss.hp <= 0) {
        Monster* cur = game.monster_head;
        while (cur != NULL) {
            if (cur->id == active_combat_monster_id) {
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
        active_combat_monster_id = 0;
    }
    else if (active_combat_monster_id != 0 && game.current_state == STATE_CITY) {
        Monster* cur = game.monster_head;
        while (cur != NULL) {
            if (cur->id == active_combat_monster_id) {
                cur->world_x = 50.0f;
                cur->world_y = 50.0f;
                break;
            }
            cur = cur->next;
        }
        active_combat_monster_id = 0;
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

// ---------- 3. 全局生存参数初始化 ----------
void InitGame() {
    game.current_state = STATE_CITY; // === 核心修改：开局直接进入 STATE_CITY（经营状态） ===
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

// ---------- 4. 商业级主循环控制入口 ----------
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

            // === 已安全移除：STATE_MENU 分支以及 DrawMainMenuLocal 相关代码 ===

        case STATE_CITY: {
            UpdateHumanPositions();
            UpdateMonsters();
            CheckAndRemoveDeadMonster();

            RenderFrame();

            if (current_time - last_settlement_time >= 10000) {
                DailySettlement();
                SpawnMonsters();
                last_settlement_time = current_time;
            }
            break;
        }

        case STATE_COMBAT: {
            UpdateHumanPositions();
            UpdateMonsters();

            cleardevice();
            DrawWorldLayer();
            DrawUI();

            int f_count = GetDebugFighterCount();
            Human** fighters = GetFightersPtr();

            Monster* boss = &game.current_boss;

            DrawCombatPanel(fighters, f_count, boss);
            FlushBatchDraw();

            if (current_time - last_combat_round_time >= 500) {
                ProcessCombatRound();
                last_combat_round_time = current_time;
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