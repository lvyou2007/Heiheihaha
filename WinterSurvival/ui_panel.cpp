// ui_panel.cpp
#include "ui_panel.h"
#include <easyx.h>
#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

void DrawHoverTooltip(Human* target, int mouse_screen_x, int mouse_screen_y) {
    if (target == NULL) return;

    int tip_w = 220, tip_h = 170;
    int left = mouse_screen_x + 15;
    int top = mouse_screen_y + 15;

    // 边界检测
    if (left + tip_w > 1280) left = mouse_screen_x - tip_w - 15;
    if (top + tip_h > 720)   top = mouse_screen_y - tip_h - 15;

    // 背景
    setfillcolor(RGB(30, 30, 30));
    fillroundrect(left, top, left + tip_w, top + tip_h, 10, 10);
    settextstyle(14, 0, _T("宋体"));

    // 名字（超凡者金色）
    if (target->is_superman) settextcolor(RGB(255, 215, 0));
    else settextcolor(WHITE);
    outtextxy(left + 10, top + 10, target->name);

    // 属性
    settextcolor(WHITE);
    char buf[64];
    sprintf(buf, "等级: %d", target->level);
    outtextxy(left + 10, top + 35, buf);
    sprintf(buf, "攻击: %d  防御: %d", target->atk, target->def);
    outtextxy(left + 10, top + 60, buf);

    // 血条
    float ratio = (float)target->hp / target->max_hp;
    int bar_w = 180, bar_h = 12;
    setfillcolor(RGB(60, 60, 60));
    fillrectangle(left + 10, top + 85, left + 10 + bar_w, top + 85 + bar_h);
    setfillcolor(RGB(0, 200, 0));
    fillrectangle(left + 10, top + 85, left + 10 + bar_w * ratio, top + 85 + bar_h);
    sprintf(buf, "HP: %d/%d", target->hp, target->max_hp);
    outtextxy(left + 10, top + 100, buf);

    // 饱食度
    sprintf(buf, "饱食度: %d", target->hunger);
    outtextxy(left + 10, top + 130, buf);
}

// 战斗面板（需要 global.h 中定义了 Monster 结构体，或者由组员D提供定义）
// 如果 Monster 未定义，请暂时注释掉此函数，或添加临时定义。
void DrawCombatPanel(Human** fighters, int fighter_count, Monster* enemy) {
    if (enemy == NULL) return;

    int panel_w = 900, panel_h = 500;
    int left = (1280 - panel_w) / 2;
    int top = (720 - panel_h) / 2;

    // 主背景
    setfillcolor(RGB(20, 20, 40));
    fillroundrect(left, top, left + panel_w, top + panel_h, 15, 15);
    setlinecolor(WHITE);
    line(left + panel_w / 2, top + 40, left + panel_w / 2, top + panel_h - 20);

    // 标题
    settextstyle(20, 0, _T("黑体"));
    settextcolor(RGB(100, 200, 100));
    char title[64];
    sprintf(title, "人类战队 (出战: %d)", fighter_count);
    outtextxy(left + 50, top + 15, title);
    settextcolor(RGB(200, 100, 100));
    outtextxy(left + panel_w / 2 + 50, top + 15, enemy->name);

    // 左侧人类列表
    int start_y = top + 60;
    int step = 35;
    int max_display = (panel_h - 80) / step;
    for (int i = 0; i < fighter_count && i < max_display; i++) {
        Human* h = fighters[i];
        if (h == NULL) continue;
        int y = start_y + i * step;

        settextstyle(12, 0, _T("宋体"));
        settextcolor(WHITE);
        outtextxy(left + 20, y, h->name);

        float r = (float)h->hp / h->max_hp;
        int bar_w = 150;
        setfillcolor(RGB(80, 80, 80));
        fillrectangle(left + 120, y - 8, left + 120 + bar_w, y + 8);
        setfillcolor(RGB(0, 200, 0));
        fillrectangle(left + 120, y - 8, left + 120 + bar_w * r, y + 8);
        char hp_txt[32];
        sprintf(hp_txt, "%d/%d", h->hp, h->max_hp);
        outtextxy(left + 280, y - 5, hp_txt);
    }
    if (fighter_count > max_display) {
        outtextxy(left + 50, start_y + max_display * step, _T("... 更多勇士 ..."));
    }

    // 右侧BOSS血条
    int boss_bar_x = left + panel_w / 2 + 80;
    int boss_bar_y = top + 100;
    int boss_bar_w = 300, boss_bar_h = 30;
    float br = (float)enemy->hp / enemy->max_hp;
    setfillcolor(RGB(80, 30, 30));
    fillrectangle(boss_bar_x, boss_bar_y, boss_bar_x + boss_bar_w, boss_bar_y + boss_bar_h);
    setfillcolor(RGB(200, 0, 0));
    fillrectangle(boss_bar_x, boss_bar_y, boss_bar_x + boss_bar_w * br, boss_bar_y + boss_bar_h);
    settextstyle(16, 0, _T("黑体"));
    char boss_info[64];
    sprintf(boss_info, "血量: %d/%d", enemy->hp, enemy->max_hp);
    outtextxy(boss_bar_x + 20, boss_bar_y - 25, boss_info);
    sprintf(boss_info, "攻击: %d  防御: %d", enemy->atk, enemy->def);
    outtextxy(boss_bar_x + 20, boss_bar_y + 40, boss_info);

    // 关闭按钮（仅示意）
    setfillcolor(RGB(150, 50, 50));
    fillcircle(left + panel_w - 20, top + 20, 12);
    settextcolor(WHITE);
    outtextxy(left + panel_w - 28, top + 12, _T("X"));
}