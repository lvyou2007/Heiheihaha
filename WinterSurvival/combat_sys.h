#pragma once
#include "global.h"

// 组员D 的接口声明
void ProcessCombatRound();
bool SaveGame(const char* filepath);
bool LoadGame(const char* filepath);
void FreeAllHumans(void);
int GetDebugFighterCount(void);
Human** GetFightersPtr(void);
Monster* GetCurrentBossPtr(void);
// 测试专用接口声明
//void DebugSpawnBoss(const char* name, int hp, int atk, int def, int meat, int exp);
//int GetDebugBossHp();
//int GetDebugBossMaxHp();
//const char* GetDebugBossName();
//int GetDebugFighterCount();
//Human* GetDebugFighter(int idx);