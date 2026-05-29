// ui_panel.h
#pragma once
#include "global.h"

// 悬浮提示框（鼠标移到人物上时显示）
void DrawHoverTooltip(Human* target, int mouse_screen_x, int mouse_screen_y);

// 战斗面板（由组员D在战斗时调用，传入出战人类数组和野怪指针）
void DrawCombatPanel(Human** fighters, int fighter_count, Monster* enemy);

//建筑面板
void DrawUpgradePanel(Building* b);