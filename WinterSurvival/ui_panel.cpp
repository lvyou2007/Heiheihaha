// ui_panel.cpp
// 已适配 Unicode 字符集，修复 LNK/E0304/C2665 报错
#define _CRT_SECURE_NO_WARNINGS
#include "ui_panel.h"
#include <easyx.h>
#include <stdio.h>
#include <tchar.h>

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

    // 名字（自适应 %S 格式化窄字符 target->name 并在超凡者时显示金色）
    TCHAR name_buf[64];
    _stprintf_s(name_buf, _T("%s"), target->name);
    if (target->is_superman) settextcolor(RGB(255, 215, 0));
    else settextcolor(WHITE);
    outtextxy(left + 10, top + 10, name_buf);

    // 属性 (采用 TCHAR 数组)
    settextcolor(WHITE);
    TCHAR buf[64];
    _stprintf_s(buf, _T("等级: %d"), target->level);
    outtextxy(left + 10, top + 35, buf);
    _stprintf_s(buf, _T("攻击: %d  防御: %d"), target->atk, target->def);
    outtextxy(left + 10, top + 60, buf);

    // 血条
    float ratio = (float)target->hp / target->max_hp;
    int bar_w = 180, bar_h = 12;
    setfillcolor(RGB(60, 60, 60));
    fillrectangle(left + 10, top + 85, left + 10 + bar_w, top + 85 + bar_h);
    setfillcolor(RGB(0, 200, 0));
    fillrectangle(left + 10, top + 85, left + 10 + (int)(bar_w * ratio), top + 85 + bar_h);

    _stprintf_s(buf, _T("HP: %d/%d"), target->hp, target->max_hp);
    outtextxy(left + 10, top + 100, buf);

    // 2. 【核心新增】：经验进度条 (深蓝色)
    float exp_ratio = (float)target->exp / 100.0f;
    setfillcolor(RGB(60, 60, 60));
    fillrectangle(left + 10, top + 122, left + 10 + bar_w, top + 122 + bar_h);
    setfillcolor(RGB(0, 150, 255)); // 科技蓝
    fillrectangle(left + 10, top + 122, left + 10 + (int)(bar_w * exp_ratio), top + 122 + bar_h);

    _stprintf_s(buf, _T("EXP: %d/100"), target->exp);
    outtextxy(left + 10, top + 137, buf);

    // 饱食度
    _stprintf_s(buf, _T("饱食度: %d"), target->hunger);
    outtextxy(left + 100, top + 137, buf);
}

// === 【核心新增整个函数】：野怪专属的半透明红色敌对悬浮面板 ===
void DrawMonsterTooltip(Monster* target, int mouse_screen_x, int mouse_screen_y) {
    if (target == NULL) return;

    int tip_w = 220, tip_h = 135;
    int left = mouse_screen_x + 15;
    int top = mouse_screen_y + 15;

    if (left + tip_w > 1280) left = mouse_screen_x - tip_w - 15;
    if (top + tip_h > 720)   top = mouse_screen_y - tip_h - 15;

    // 暗红色底框，象征敌对/野生生态
    setfillcolor(RGB(50, 25, 25));
    fillroundrect(left, top, left + tip_w, top + tip_h, 10, 10);
    setlinecolor(RGB(200, 50, 50));
    roundrect(left, top, left + tip_w, top + tip_h, 10, 10);

    settextstyle(14, 0, _T("宋体"));

    // 名字颜色判定
    TCHAR name_buf[64];
    _stprintf_s(name_buf, _T("%s"), target->name);
    if (target->type == MONSTER_AGGRESSIVE) settextcolor(RGB(255, 69, 0));
    else settextcolor(RGB(255, 215, 0));
    outtextxy(left + 10, top + 10, name_buf);

    settextcolor(WHITE);
    TCHAR buf[64];
    _stprintf_s(buf, _T("类型: %s"), target->type == MONSTER_AGGRESSIVE ? _T("暴虐野兽 (敌对)") : _T("温顺驯鹿 (无害)"));
    outtextxy(left + 10, top + 35, buf);
    _stprintf_s(buf, _T("攻击: %d  防御: %d"), target->atk, target->def);
    outtextxy(left + 10, top + 60, buf);

    // 生命血条 (红色)
    float ratio = (float)target->hp / target->max_hp;
    int bar_w = 180, bar_h = 12;
    setfillcolor(RGB(80, 80, 80));
    fillrectangle(left + 10, top + 85, left + 10 + bar_w, top + 85 + bar_h);
    setfillcolor(RGB(220, 0, 0));
    fillrectangle(left + 10, top + 85, left + 10 + (int)(bar_w * ratio), top + 85 + bar_h);

    _stprintf_s(buf, _T("HP: %d/%d"), target->hp, target->max_hp);
    outtextxy(left + 10, top + 100, buf);
}

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
    TCHAR title[64];
    _stprintf_s(title, _T("人类战队 (出战: %d)"), fighter_count);
    outtextxy(left + 50, top + 15, title);

    // Boss 名字格式化自适应 %S
    TCHAR enemy_name[64];
    _stprintf_s(enemy_name, _T("%s"), enemy->name);
    settextcolor(RGB(200, 100, 100));
    outtextxy(left + panel_w / 2 + 50, top + 15, enemy_name);

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

        // 格式化战士人名
        TCHAR h_name[64];
        _stprintf_s(h_name, _T("%s"), h->name);
        outtextxy(left + 20, y, h_name);

        float r = (float)h->hp / h->max_hp;
        int bar_w = 150;
        setfillcolor(RGB(80, 80, 80));
        fillrectangle(left + 120, y - 8, left + 120 + bar_w, y + 8);
        setfillcolor(RGB(0, 200, 0));
        fillrectangle(left + 120, y - 8, left + 120 + (int)(bar_w * r), y + 8);

        TCHAR hp_txt[32];
        _stprintf_s(hp_txt, _T("%d/%d"), h->hp, h->max_hp);
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
    fillrectangle(boss_bar_x, boss_bar_y, boss_bar_x + (int)(boss_bar_w * br), boss_bar_y + boss_bar_h);
    settextstyle(16, 0, _T("黑体"));

    TCHAR boss_info[64];
    _stprintf_s(boss_info, _T("血量: %d/%d"), enemy->hp, enemy->max_hp);
    outtextxy(boss_bar_x + 20, boss_bar_y - 25, boss_info);
    _stprintf_s(boss_info, _T("攻击: %d  防御: %d"), enemy->atk, enemy->def);
    outtextxy(boss_bar_x + 20, boss_bar_y + 40, boss_info);

    // 关闭按钮
    setfillcolor(RGB(150, 50, 50));
    fillcircle(left + panel_w - 20, top + 20, 12);
    settextcolor(WHITE);
    outtextxy(left + panel_w - 28, top + 12, _T("X"));
}

// 追加到 ui_panel.cpp 尾部
void DrawUpgradePanel(Building* b) {
    if (b == NULL) return;

    // 弹窗固定居中尺寸：400x300
    int w = 400, h = 300;
    int left = (1280 - w) / 2; // 440
    int top = (720 - h) / 2;   // 210

    // 1. 绘制面板背景与边框
    setfillcolor(RGB(25, 30, 40));
    fillroundrect(left, top, left + w, top + h, 15, 15);
    setlinecolor(RGB(0, 200, 255));
    roundrect(left, top, left + w, top + h, 15, 15);

    // 2. 绘制标题和当前等级
    setbkmode(TRANSPARENT);
    settextstyle(20, 0, _T("黑体"));
    settextcolor(RGB(0, 255, 255));

    TCHAR title[64];
    if (b->type == 0) _stprintf_s(title, _T("【 矿 场 控 制 枢 纽 】"));
    else if (b->type == 1) _stprintf_s(title, _T("【 伐 木 场 控 制 枢 纽 】"));
    else if (b->type == 2) _stprintf_s(title, _T("【 能 源 大 熔 炉 】"));
    outtextxy(left + 60, top + 25, title);

    settextstyle(16, 0, _T("宋体"));
    settextcolor(WHITE);
    TCHAR lv_buf[32];
    if (b->level >= MAX_BUILDING_LEVEL) _stprintf_s(lv_buf, _T("当前等级: MAX"));
    else _stprintf_s(lv_buf, _T("当前等级: %d / %d 级"), b->level, MAX_BUILDING_LEVEL);
    outtextxy(left + 40, top + 75, lv_buf);

    // 3. 绘制核心指标属性对比（使用与升级一致的公式）
    TCHAR attr_buf1[128];
    TCHAR attr_buf2[128];
    settextcolor(RGB(200, 200, 200));

    if (b->type == 0) { // 矿场
        _stprintf_s(attr_buf1, _T("当前煤炭生产速率: %d / 每天"), b->produce_rate);
        if (b->level < MAX_BUILDING_LEVEL) {
            int next_rate = 5 + (b->level) * 5;          // 下一级等级 = b->level+1，公式 produce_rate = 5 + (level-1)*5，代入得 next_rate = 5 + b->level*5
            int next_cost = 150 + (b->level + 1) * 50;   // 下一级消耗木材
            _stprintf_s(attr_buf2, _T("升级后生产速率: %d / 每天 (+%d)  消耗木材: %d"), next_rate, next_rate - b->produce_rate, next_cost);
        }
    }
    else if (b->type == 1) { // 伐木场
        _stprintf_s(attr_buf1, _T("当前木材生产速率: %d / 每天"), b->produce_rate);
        if (b->level < MAX_BUILDING_LEVEL) {
            int next_rate = 15 + (b->level) * 10;        // 同理 produce_rate = 15 + (level-1)*10
            int next_cost = 100 + (b->level + 1) * 40;
            _stprintf_s(attr_buf2, _T("升级后生产速率: %d / 每天 (+%d)  消耗木材: %d"), next_rate, next_rate - b->produce_rate, next_cost);
        }
    }
    else if (b->type == 2) { // 熔炉
        _stprintf_s(attr_buf1, _T("当前核芯辐射温度: %d C (提供体感 +%d C)"), b->produce_rate, (int)(b->produce_rate * 0.2));
        if (b->level < MAX_BUILDING_LEVEL) {
            int next_temp = 100 + (b->level) * 80;       // produce_rate = 100 + (level-1)*80
            int next_cost = 200 + (b->level + 1) * 60;
            _stprintf_s(attr_buf2, _T("升级后辐射温度: %d C (体感 +%d C)  消耗木材: %d"), next_temp, (int)(next_temp * 0.2), next_cost);
        }
    }
    outtextxy(left + 40, top + 115, attr_buf1);
    if (b->level < MAX_BUILDING_LEVEL) {
        settextcolor(RGB(100, 255, 100)); // 升级提升属性显示为绿色
        outtextxy(left + 40, top + 145, attr_buf2);
    }

    // 4. 绘制升级开销
    settextcolor(WHITE);
    TCHAR cost_buf[128];
    if (b->level >= MAX_BUILDING_LEVEL) {
        _stprintf_s(cost_buf, _T("升级所需木材: MAX"));
    }
    else {
        _stprintf_s(cost_buf, _T("升级所需木材: %d  (当前拥有: %d)"), b->cost_wood, game.wood);
    }
    outtextxy(left + 40, top + 185, cost_buf);

    // 5. 绘制底部交互按钮
    int btn_x = left + 100; // 540
    int btn_y = top + 230;  // 440
    int btn_w = 200, btn_h = 40;

    if (b->level >= MAX_BUILDING_LEVEL) {
        setfillcolor(RGB(80, 80, 80)); // 满级灰色按钮
        fillroundrect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 8, 8);
        settextcolor(RGB(180, 180, 180));
        outtextxy(btn_x + 55, btn_y + 12, _T("已升满 (MAX)"));
    }
    else {
        // 判断资源是否足够
        if (game.wood >= b->cost_wood) {
            setfillcolor(RGB(0, 150, 255)); // 蓝色高亮升级按钮
            settextcolor(WHITE);
        }
        else {
            setfillcolor(RGB(80, 50, 50));  // 资源不足暗红色按钮
            settextcolor(RGB(180, 150, 150));
        }
        fillroundrect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 8, 8);
        outtextxy(btn_x + 65, btn_y + 12, _T("点击升级"));
    }

    // 6. 绘制右上角 "X" 关闭圆圈
    setfillcolor(RGB(150, 50, 50));
    fillcircle(left + w - 20, top + 20, 12); // 圆心在 820, 230
    settextcolor(WHITE);
    settextstyle(12, 0, _T("Arial"));
    outtextxy(left + w - 24, top + 13, _T("X"));
}