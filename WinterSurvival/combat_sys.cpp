// combat_sys.cpp
// 组员D: 动态战斗引擎 + 序列化存档/读档
// 适配最新的 global.h (2026-05-26版本)

#include "combat_sys.h"
#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ---------- 辅助函数：随机抽取 fighters ----------
// 从 game.human_list 中抽取 need_cnt 个存活且尚未在 fighters 数组中的成员
// 将新抽取的成员存入 game.fighters 数组，从 start_idx 位置开始存放
// 返回实际抽取人数
static int RandomSelectFighters(int need_cnt, int start_idx) {
    // 收集所有候选（存活且尚未在 fighters 中）
    Human* candidates[1024];
    int cand_cnt = 0;
    Human* cur = game.human_list;
    while (cur) {
        if (cur->hp > 0) {
            int already = 0;
            for (int i = 0; i < game.fighter_count; i++) {
                if (game.fighters[i] == cur) {
                    already = 1;
                    break;
                }
            }
            if (!already && cand_cnt < 1024)
                candidates[cand_cnt++] = cur;
        }
        cur = cur->next;
    }
    int available = cand_cnt;
    if (need_cnt > available) need_cnt = available;
    // 随机抽取 need_cnt 个不同的人 (Fisher-Yates)
    for (int i = 0; i < need_cnt; i++) {
        int idx = rand() % (cand_cnt - i);
        game.fighters[start_idx + i] = candidates[idx];
        // 交换到末尾避免重复
        Human* tmp = candidates[idx];
        candidates[idx] = candidates[cand_cnt - i - 1];
        candidates[cand_cnt - i - 1] = tmp;
    }
    return need_cnt;
}

// 计算当前 fighters 中所有存活人类的平均血量百分比
static float AvgHpPercent(void) {
    if (game.fighter_count == 0) return 100.0f;
    float sum = 0.0f;
    int alive = 0;
    for (int i = 0; i < game.fighter_count; i++) {
        if (game.fighters[i]->hp > 0) {
            sum += (game.fighters[i]->hp / game.fighters[i]->max_hp) * 100.0f;
            alive++;
        }
    }
    if (alive == 0) return 100.0f;
    return sum / alive;
}

// ---------- 公共接口实现 ----------

void ProcessCombatRound(void) {
    // 如果没有怪物或怪物已死，清空战斗状态并返回
    if (game.current_boss.hp <= 0) {
        // 恢复所有参战人员状态（如果有）
        for (int i = 0; i < game.fighter_count; i++) {
            if (game.fighters[i]->hp > 0)
                game.fighters[i]->state = STATE_IDLE;
        }
        game.fighter_count = 0;
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    // 初次进入战斗时初始化 fighters (尚未有参战人员)
    static int support_triggered = 0;
    if (game.fighter_count == 0) {
        int total_alive = 0;
        Human* cur = game.human_list;
        while (cur) {
            if (cur->hp > 0) total_alive++;
            cur = cur->next;
        }
        if (total_alive == 0) return; // 无人可战，无法战斗

        int need = (int)(total_alive * 0.3f);
        if (need < 1) need = 1;
        game.fighter_count = RandomSelectFighters(need, 0);
        // 标记参战人员状态
        for (int i = 0; i < game.fighter_count; i++) {
            game.fighters[i]->state = STATE_FIGHTING;
        }
        support_triggered = 0; // 重置支援标记
    }

    // 回合制战斗（一轮攻防）
    // 人类攻击阶段
    float total_atk = 0;
    for (int i = 0; i < game.fighter_count; i++) {
        if (game.fighters[i]->hp > 0)
            total_atk += game.fighters[i]->atk;
    }
    float damage = total_atk - game.current_boss.def;
    if (damage < 0) damage = 0;
    game.current_boss.hp -= damage;

    // 如果怪物死亡，结束战斗
    if (game.current_boss.hp <= 0) {
        // 恢复所有参战人员状态
        for (int i = 0; i < game.fighter_count; i++) {
            if (game.fighters[i]->hp > 0)
                game.fighters[i]->state = STATE_IDLE;
        }
        game.fighter_count = 0;
        support_triggered = 0;
        return;
    }

    // 野怪反击（均摊伤害）
    int alive_fighters = 0;
    for (int i = 0; i < game.fighter_count; i++) {
        if (game.fighters[i]->hp > 0) alive_fighters++;
    }
    if (alive_fighters > 0) {
        float dmg_per_human = game.current_boss.atk / alive_fighters;
        for (int i = 0; i < game.fighter_count; i++) {
            if (game.fighters[i]->hp > 0) {
                float real_damage = dmg_per_human - game.fighters[i]->def;
                if (real_damage < 0) real_damage = 0;
                game.fighters[i]->hp -= real_damage;
                if (game.fighters[i]->hp < 0) game.fighters[i]->hp = 0;
            }
        }
    }

    // 支援机制（每场战斗最多一次）
    if (!support_triggered) {
        float avg_percent = AvgHpPercent();
        if (avg_percent < 60.0f) {
            // 统计剩余未参战的存活人类数量
            int remaining = 0;
            Human* cur = game.human_list;
            while (cur) {
                if (cur->hp > 0) {
                    int already = 0;
                    for (int i = 0; i < game.fighter_count; i++) {
                        if (game.fighters[i] == cur) {
                            already = 1;
                            break;
                        }
                    }
                    if (!already) remaining++;
                }
                cur = cur->next;
            }
            if (remaining > 0) {
                int add_cnt = (int)(remaining * 0.3f);
                if (add_cnt < 1) add_cnt = 1;
                int new_cnt = RandomSelectFighters(add_cnt, game.fighter_count);
                if (new_cnt > 0) {
                    game.fighter_count += new_cnt;
                    support_triggered = 1;
                }
            }
        }
    }

    // 检查是否所有参战人类都已死亡
    int any_alive = 0;
    for (int i = 0; i < game.fighter_count; i++) {
        if (game.fighters[i]->hp > 0) {
            any_alive = 1;
            break;
        }
    }
    if (!any_alive) {
        // 全军覆没，战斗失败，怪物依然存活
        for (int i = 0; i < game.fighter_count; i++) {
            game.fighters[i]->state = STATE_IDLE;
        }
        game.fighter_count = 0;
        support_triggered = 0;
    }
}

// ---------- 序列化数据结构（不含指针）----------
typedef struct {
    int id;
    char name[20];
    float world_x, world_y;
    float hp, max_hp;
    float hunger;
    float atk, def;
    int level;
    HumanType type;
    HumanState state;
} HumanSaveData;

typedef struct {
    char name[20];
    float hp, max_hp;
    float atk, def;
} MonsterSaveData;

// 保存游戏状态到文件，成功返回 true，失败返回 false
bool SaveGame(const char* filepath) {
    FILE* fp = fopen(filepath, "wb");
    if (!fp) return false;

    // 1. 保存基础类型数据（不包含指针）
    fwrite(&game.current_state, sizeof(AppState), 1, fp);
    fwrite(&game.camera, sizeof(Camera), 1, fp);
    fwrite(&game.wood, sizeof(int), 1, fp);
    fwrite(&game.coal, sizeof(int), 1, fp);
    fwrite(&game.food, sizeof(int), 1, fp);
    fwrite(&game.env_temp, sizeof(float), 1, fp);
    fwrite(&game.furnace_temp, sizeof(float), 1, fp);
    fwrite(&game.total_humans, sizeof(int), 1, fp);
    fwrite(&game.fighter_count, sizeof(int), 1, fp);
    // 保存 current_boss
    MonsterSaveData boss_data;
    strcpy(boss_data.name, game.current_boss.name);
    boss_data.hp = game.current_boss.hp;
    boss_data.max_hp = game.current_boss.max_hp;
    boss_data.atk = game.current_boss.atk;
    boss_data.def = game.current_boss.def;
    fwrite(&boss_data, sizeof(MonsterSaveData), 1, fp);

    // 2. 保存人类链表数据
    int human_count = 0;
    Human* cur = game.human_list;
    while (cur) {
        human_count++;
        cur = cur->next;
    }
    fwrite(&human_count, sizeof(int), 1, fp);
    cur = game.human_list;
    for (int i = 0; i < human_count; i++) {
        HumanSaveData data;
        data.id = cur->id;
        strcpy(data.name, cur->name);
        data.world_x = cur->world_x;
        data.world_y = cur->world_y;
        data.hp = cur->hp;
        data.max_hp = cur->max_hp;
        data.hunger = cur->hunger;
        data.atk = cur->atk;
        data.def = cur->def;
        data.level = cur->level;
        data.type = cur->type;
        data.state = cur->state;
        fwrite(&data, sizeof(HumanSaveData), 1, fp);
        cur = cur->next;
    }

    fclose(fp);
    return true;
}

// 释放所有人类节点
void FreeAllHumans(void) {
    Human* cur = game.human_list;
    while (cur) {
        Human* next = cur->next;
        free(cur);
        cur = next;
    }
    game.human_list = NULL;
    game.total_humans = 0;
}

bool LoadGame(const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return false;

    // 1. 读取基础数据
    fread(&game.current_state, sizeof(AppState), 1, fp);
    fread(&game.camera, sizeof(Camera), 1, fp);
    fread(&game.wood, sizeof(int), 1, fp);
    fread(&game.coal, sizeof(int), 1, fp);
    fread(&game.food, sizeof(int), 1, fp);
    fread(&game.env_temp, sizeof(float), 1, fp);
    fread(&game.furnace_temp, sizeof(float), 1, fp);
    fread(&game.total_humans, sizeof(int), 1, fp);
    fread(&game.fighter_count, sizeof(int), 1, fp);
    // 读 current_boss
    MonsterSaveData boss_data;
    fread(&boss_data, sizeof(MonsterSaveData), 1, fp);
    strcpy(game.current_boss.name, boss_data.name);
    game.current_boss.hp = boss_data.hp;
    game.current_boss.max_hp = boss_data.max_hp;
    game.current_boss.atk = boss_data.atk;
    game.current_boss.def = boss_data.def;

    // 指针字段初始化
    game.hovered_target = NULL;
    // 清空 fighters 数组（战斗状态不恢复，读档后由主逻辑重新触发战斗）
    for (int i = 0; i < game.fighter_count; i++) {
        game.fighters[i] = NULL;
    }
    game.fighter_count = 0;   // 强制清空

    // 2. 读取人类数量并重建链表
    int human_count;
    fread(&human_count, sizeof(int), 1, fp);
    // 释放旧链表
    FreeAllHumans();

    Human* prev = NULL;
    for (int i = 0; i < human_count; i++) {
        HumanSaveData data;
        fread(&data, sizeof(HumanSaveData), 1, fp);
        Human* new_human = (Human*)malloc(sizeof(Human));
        if (!new_human) {
            fclose(fp);
            return false;
        }
        // 逐成员赋值，避免复合字面量语法错误
        new_human->id = data.id;
        strcpy(new_human->name, data.name);
        new_human->world_x = data.world_x;
        new_human->world_y = data.world_y;
        new_human->hp = data.hp;
        new_human->max_hp = data.max_hp;
        new_human->hunger = data.hunger;
        new_human->atk = data.atk;
        new_human->def = data.def;
        new_human->level = data.level;
        new_human->type = data.type;
        new_human->state = data.state;
        new_human->prev = prev;
        new_human->next = NULL;

        if (prev) {
            prev->next = new_human;
        }
        else {
            game.human_list = new_human;
        }
        prev = new_human;
    }

    fclose(fp);
    return true;
}