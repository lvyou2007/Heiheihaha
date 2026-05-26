#pragma once
#include "global.h"

// 组员D 的接口声明
void ProcessCombatRound();
bool SaveGame(const char* filepath);
bool LoadGame(const char* filepath);
void FreeAllHumans(void);