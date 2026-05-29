#pragma once
// monster_logic.h
// 独立的野怪生态系统接口 (不影响组员D的任何代码)
#pragma once
#include "global.h"

// 刷新野怪
void SpawnMonsters();

// 智能追猎与平滑漫步 (60 FPS)
void UpdateMonsters();

// 检查并清除已被击毙的实体野狼
void CheckAndRemoveDeadMonster();

// 鼠标悬浮获取器
Monster* GetHoveredMonster(float click_world_x, float click_world_y, float radius);