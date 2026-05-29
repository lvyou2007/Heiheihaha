// combat_sys.cpp
// 组员D: 动态战斗引擎 + 序列化存档/读档
// 已完美适配 2026-05-26 最终版 global.h 并修复链接与类型 Bug
#define _CRT_SECURE_NO_WARNINGS
#include "combat_sys.h"
#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ---------- 战斗系统私有静态状态 (不污染全局 global.h) ----------
static Human* fighters[100];
static int fighter_count = 0;
static Monster current_boss; // 当前交战的怪物数据

// 声明外部函数（调用组员 C 在 human_logic.cpp 中编写的内存清理函数，避免多重定义冲突）
extern void FreeAllHumans(void);

// ---------- 辅助函数：随机抽取 fighters ----------
static int RandomSelectFighters(int need_cnt, int start_idx) {
    Human* candidates[1024];
    int cand_cnt = 0;

    // 适配最新 global.h：使用 game.head 代替原来的 game.human_list
    Human* cur = game.head;
    while (cur) {
        if (cur->hp > 0) {
            int already = 0;
            for (int i = 0; i < fighter_count; i++) {
                if (fighters[i] == cur) {
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

    // Fisher-Yates 随机打乱抽取
    for (int i = 0; i < need_cnt; i++) {
        int idx = rand() % (cand_cnt - i);
        fighters[start_idx + i] = candidates[idx];
        Human* tmp = candidates[idx];
        candidates[idx] = candidates[cand_cnt - i - 1];
        candidates[cand_cnt - i - 1] = tmp;
    }
    return need_cnt;
}

// 计算当前 fighters 中所有存活人类的平均血量百分比
static float AvgHpPercent(void) {
    if (fighter_count == 0) return 100.0f;
    float sum = 0.0f;
    int alive = 0;
    for (int i = 0; i < fighter_count; i++) {
        if (fighters[i]->hp > 0) {
            // 显式转换类型计算百分比
            sum += ((float)fighters[i]->hp / fighters[i]->max_hp) * 100.0f;
            alive++;
        }
    }
    if (alive == 0) return 100.0f;
    return sum / alive;
}

// ---------- 公共接口实现 ----------

void ProcessCombatRound(void) {
    // 如果没有怪物或怪物已死，清空战斗状态并返回
    if (current_boss.hp <= 0) {
        // 由于最新版 Human 删去了 state 变量，移除了原有的 cur->state = STATE_IDLE 逻辑
        fighter_count = 0;
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    // 初次进入战斗时初始化 fighters (尚未有参战人员)
    static int support_triggered = 0;
    if (fighter_count == 0) {
        int total_alive = 0;
        Human* cur = game.head; // 改为使用 game.head
        while (cur) {
            if (cur->hp > 0) total_alive++;
            cur = cur->next;
        }
        if (total_alive == 0) return; // 无人可战，退出

        int need = (int)(total_alive * 0.3f);
        if (need < 1) need = 1;
        fighter_count = RandomSelectFighters(need, 0);
        support_triggered = 0; // 重置支援标记
    }

    // 回合制战斗：人类攻击阶段
    float total_atk = 0;
    for (int i = 0; i < fighter_count; i++) {
        if (fighters[i]->hp > 0)
            total_atk += fighters[i]->atk;
    }
    float damage = total_atk - current_boss.def;
    if (damage < 0) damage = 0;
    current_boss.hp -= (int)damage; // 属性转换为 int 处理

    // 如果野怪被击杀，结束战斗并进行丰厚的战利品结算
    if (current_boss.hp <= 0) {
        // 1. 战利品增加食物资源（肉类）
        game.meat += current_boss.meat_reward;

        // 2. 存活的参战人员平分/获得经验值
        for (int i = 0; i < fighter_count; i++) {
            if (fighters[i]->hp > 0) {
                fighters[i]->exp += current_boss.exp_reward;
                // 升级机制判定
                if (fighters[i]->exp >= 100) {
                    fighters[i]->level++;
                    fighters[i]->exp -= 100;
                    fighters[i]->max_hp += 20;
                    fighters[i]->atk += 5;
                    fighters[i]->def += 3;
                    fighters[i]->hp = fighters[i]->max_hp; // 升级回满生命
                }
            }
        }
        fighter_count = 0;
        support_triggered = 0;
        return;
    }

    // 野怪反击（均摊伤害）
    int alive_fighters = 0;
    for (int i = 0; i < fighter_count; i++) {
        if (fighters[i]->hp > 0) alive_fighters++;
    }
    if (alive_fighters > 0) {
        float dmg_per_human = (float)current_boss.atk / alive_fighters;
        for (int i = 0; i < fighter_count; i++) {
            if (fighters[i]->hp > 0) {
                float real_damage = dmg_per_human - fighters[i]->def;
                if (real_damage < 0) real_damage = 0;
                fighters[i]->hp -= (int)real_damage;
                if (fighters[i]->hp < 0) fighters[i]->hp = 0;
            }
        }
    }

    // 支援机制（每场战斗最多一次）
    if (!support_triggered) {
        float avg_percent = AvgHpPercent();
        if (avg_percent < 60.0f) {
            int remaining = 0;
            Human* cur = game.head; // 改为 game.head
            while (cur) {
                if (cur->hp > 0) {
                    int already = 0;
                    for (int i = 0; i < fighter_count; i++) {
                        if (fighters[i] == cur) {
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
                int new_cnt = RandomSelectFighters(add_cnt, fighter_count);
                if (new_cnt > 0) {
                    fighter_count += new_cnt;
                    support_triggered = 1;
                }
            }
        }
    }

    // 检查是否所有参战人类都已死亡
    int any_alive = 0;
    for (int i = 0; i < fighter_count; i++) {
        if (fighters[i]->hp > 0) {
            any_alive = 1;
            break;
        }
    }
    if (!any_alive) {
        fighter_count = 0;
        support_triggered = 0;
    }
}

// ---------- 序列化数据结构（不含指针，100%贴合最新 global.h）----------
typedef struct {
    int id;
    char name[20];
    float world_x, world_y;
    int hp, max_hp;
    int hunger;
    int atk, def;
    int exp;
    int level;
    int is_superman;
} HumanSaveData;

typedef struct {
    char name[20];
    int hp, max_hp;
    int atk, def;
    int meat_reward;
    int exp_reward;
} MonsterSaveData;

// 保存游戏状态到文件
bool SaveGame(const char* filepath) {
    FILE* fp = fopen(filepath, "wb");
    if (!fp) return false;

    // 1. 保存基础类型数据（改写为 int 与 meat，贴合最新宪法）
    fwrite(&game.current_state, sizeof(AppState), 1, fp);
    fwrite(&game.camera, sizeof(Camera), 1, fp);
    fwrite(&game.wood, sizeof(int), 1, fp);
    fwrite(&game.coal, sizeof(int), 1, fp);
    fwrite(&game.meat, sizeof(int), 1, fp);      // 修改为 meat
    fwrite(&game.env_temp, sizeof(int), 1, fp);   // 修改为 int
    fwrite(&game.furnace_temp, sizeof(int), 1, fp);// 修改为 int
    fwrite(&game.population, sizeof(int), 1, fp); // 修改为 population统计

    // 保存战斗模块特有的静态变量
    fwrite(&fighter_count, sizeof(int), 1, fp);
    for (int i = 0; i < fighter_count; i++) {
        fwrite(&fighters[i]->id, sizeof(int), 1, fp); // 仅写入 ID，用于读档时重连
    }

    // 保存当前战斗的怪物数据
    MonsterSaveData boss_data;
    strcpy(boss_data.name, current_boss.name);
    boss_data.hp = current_boss.hp;
    boss_data.max_hp = current_boss.max_hp;
    boss_data.atk = current_boss.atk;
    boss_data.def = current_boss.def;
    boss_data.meat_reward = current_boss.meat_reward;
    boss_data.exp_reward = current_boss.exp_reward;
    fwrite(&boss_data, sizeof(MonsterSaveData), 1, fp);

    // 2. 保存人类链表数据
    int human_count = game.population;
    fwrite(&human_count, sizeof(int), 1, fp);
    Human* cur = game.head; // 改为 game.head
    while (cur) {
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
        data.exp = cur->exp;
        data.level = cur->level;
        data.is_superman = cur->is_superman;
        fwrite(&data, sizeof(HumanSaveData), 1, fp);
        cur = cur->next;
    }

    fclose(fp);
    return true;
}

// 读取存档并反序列化
bool LoadGame(const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return false;

    // 1. 读取基础数据（完全匹配 int 类型）
    fread(&game.current_state, sizeof(AppState), 1, fp);
    fread(&game.camera, sizeof(Camera), 1, fp);
    fread(&game.wood, sizeof(int), 1, fp);
    fread(&game.coal, sizeof(int), 1, fp);
    fread(&game.meat, sizeof(int), 1, fp);       // 读取 meat
    fread(&game.env_temp, sizeof(int), 1, fp);
    fread(&game.furnace_temp, sizeof(int), 1, fp);
    fread(&game.population, sizeof(int), 1, fp);  // 读取 population

    // 读取战斗特有状态
    int saved_fighter_count;
    fread(&saved_fighter_count, sizeof(int), 1, fp);
    int saved_fighter_ids[100];
    for (int i = 0; i < saved_fighter_count; i++) {
        fread(&saved_fighter_ids[i], sizeof(int), 1, fp);
    }

    // 读取怪物数据
    MonsterSaveData boss_data;
    fread(&boss_data, sizeof(MonsterSaveData), 1, fp);
    strcpy(current_boss.name, boss_data.name);
    current_boss.hp = boss_data.hp;
    current_boss.max_hp = boss_data.max_hp;
    current_boss.atk = boss_data.atk;
    current_boss.def = boss_data.def;
    current_boss.meat_reward = boss_data.meat_reward;
    current_boss.exp_reward = boss_data.exp_reward;

    // 清空重置指针字段
    game.hovered_target = NULL;
    game.hovered_monster = NULL;

    // 2. 读取人类数量并重建链表
    int human_count;
    fread(&human_count, sizeof(int), 1, fp);

    // 释放本地旧链表（调用 human_logic 中的释放函数，避免多重定义冲突）
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
        new_human->id = data.id;
        strcpy(new_human->name, data.name);
        new_human->world_x = data.world_x;
        new_human->world_y = data.world_y;
        new_human->hp = data.hp;
        new_human->max_hp = data.max_hp;
        new_human->hunger = data.hunger;
        new_human->atk = data.atk;
        new_human->def = data.def;
        new_human->exp = data.exp;
        new_human->level = data.level;
        new_human->is_superman = data.is_superman;
        new_human->prev = prev;
        new_human->next = NULL;

        if (prev) {
            prev->next = new_human;
        }
        else {
            game.head = new_human; // 指向头指针
        }
        prev = new_human;
    }
    game.tail = prev; // 恢复双向链表的尾指针

    // 3. 【核心技术点：重连战斗中人员的指针】
    // 读档后，旧的地址已经失效。此处根据刚才读取的 ID，重新绑定参战人员的真实内存指针
    fighter_count = 0;
    for (int i = 0; i < saved_fighter_count; i++) {
        int target_id = saved_fighter_ids[i];
        Human* curr = game.head;
        while (curr) {
            if (curr->id == target_id) {
                fighters[fighter_count++] = curr;
                break;
            }
            curr = curr->next;
        }
    }

    fclose(fp);
    return true;
}
// 追加到 combat_sys.cpp 尾部，用于测试模块读取状态
//void DebugSpawnBoss(const char* name, int hp, int atk, int def, int meat, int exp) {
  //  strcpy(current_boss.name, name);
    //current_boss.max_hp = hp;
    //current_boss.hp = hp;
    //current_boss.atk = atk;
    //current_boss.def = def;
    //current_boss.meat_reward = meat;
    //current_boss.exp_reward = exp;
//}

//int GetDebugBossHp() { return current_boss.hp; }
//int GetDebugBossMaxHp() { return current_boss.max_hp; }
//const char* GetDebugBossName() { return current_boss.name; }
//int GetDebugFighterCount() { return fighter_count; }
//Human* GetDebugFighter(int idx) { return fighters[idx]; }