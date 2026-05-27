#pragma once
#include "global.h" // 结构体全过来

// 组员C 的接口声明
void InitHumanList();
Human* CreateHuman(int base_level);
void AddHumanToList(Human* new_human);
void RemoveHumanFromList(Human* target);
void PrintAllHumans();
void FreeAllHumans();

void DailySettlement();
Human** GetFighters(int* out_count);
void GetAverageAttributes(int* avg_atk, int* avg_def, int* avg_hp);
Human* GetHoveredHuman(float click_world_x, float click_world_y, float radius);