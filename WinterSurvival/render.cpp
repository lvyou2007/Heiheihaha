// render.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "render.h"
#include "ui_panel.h"
#include "camera.h"          // 组员A提供
#include <easyx.h>
#include <stdio.h>
#include <atlconv.h >
#include <windows.h>


// 贴图句柄（请替换为实际图片路径）
static IMAGE img_human_normal, img_human_super;
static IMAGE img_mine, img_woodcamp, img_furnace;

void InitRender() {
    loadimage(&img_human_normal, _T("res/human_normal.png"));
    loadimage(&img_human_super, _T("res/human_super.png"));
    loadimage(&img_mine, _T("res/mine.png"));
    loadimage(&img_woodcamp, _T("res/famu.png"));
    loadimage(&img_furnace, _T("res/furnace.png"));
    if (loadimage(&img_human_normal, _T("res/human_normal.png"))) {
        MessageBox(NULL, _T("加载 human_normal.png 失败"), _T("错误"), MB_OK);
    }
}

void DrawWorldLayer() {
    Camera cam = game.camera;

    // 遍历所有人类（双向链表）
    Human* cur = game.head;
    while (cur != NULL) {
        int screen_x, screen_y;
        WorldToScreen(cur->world_x, cur->world_y, &screen_x, &screen_y);

        // 选择贴图（超凡者用金色边框的图）
        IMAGE* img = cur->is_superman ? &img_human_super : &img_human_normal;
        int draw_w = (int)(img->getwidth() * cam.zoom);
        int draw_h = (int)(img->getheight() * cam.zoom);
        putimage(screen_x - draw_w / 2, screen_y - draw_h / 2, draw_w, draw_h, img, 0, 0, SRCCOPY);

        // 名字
        settextstyle(12, 0, _T("宋体"));
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outtextxy(screen_x - 20, screen_y - draw_h / 2 - 15,cur->name);

        // 血条
        float ratio = (float)cur->hp / cur->max_hp;
        int bar_width = (int)(50 * cam.zoom);
        int bar_height = (int)(6 * cam.zoom);
        int bar_left = screen_x - bar_width / 2;
        int bar_top = screen_y - draw_h / 2 - 8;
        setfillcolor(RGB(80, 80, 80));
        fillrectangle(bar_left, bar_top, bar_left + bar_width, bar_top + bar_height);
        setfillcolor(RGB(0, 200, 0));
        fillrectangle(bar_left, bar_top, bar_left + bar_width * ratio, bar_top + bar_height);

        cur = cur->next;
    }

    // 绘制建筑（世界坐标固定，可根据实际情况从 game 读取建筑列表）
    int mx, my;
    WorldToScreen(500, 300, &mx, &my);
    int draw_w = (int)(img_mine.getwidth() * game.camera.zoom);
    int draw_h = (int)(img_mine.getheight() * game.camera.zoom);
    putimage(mx - draw_w / 2, my - draw_h / 2, draw_w, draw_h, &img_mine, 0, 0, SRCCOPY);

    int wx, wy;
    WorldToScreen(800, 400, &wx, &wy);
    draw_w = (int)(img_woodcamp.getwidth() * game.camera.zoom);
    draw_h = (int)(img_woodcamp.getheight() * game.camera.zoom);
    putimage(wx - draw_w / 2, wy - draw_h / 2, draw_w, draw_h, &img_woodcamp, 0, 0, SRCCOPY);

    int fx, fy;
    WorldToScreen(350, 600, &fx, &fy);
    draw_w = (int)(img_furnace.getwidth() * game.camera.zoom);
    draw_h = (int)(img_furnace.getheight() * game.camera.zoom);
    putimage(fx - draw_w / 2, fy - draw_h / 2, draw_w, draw_h, &img_furnace, 0, 0, SRCCOPY);
}

void DrawUI() {
    // 左上角资源面板
    setfillcolor(RGB(40, 40, 40));
    fillrectangle(10, 10, 260, 110);
    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));
    char buf[100];
    sprintf(buf, "木头: %d", game.wood);
    outtextxy(20, 20, buf);
    sprintf(buf, "煤炭: %d", game.coal);
    outtextxy(20, 45, buf);
    sprintf(buf, "食物: %d", game.meat);
    outtextxy(20, 70, buf);
    sprintf(buf, "人口: %d", game.population);
    outtextxy(150, 20, buf);

    // 温度
    sprintf(buf, "当前温度: %d", game.env_temp);
    outtextxy(10, 680, buf);
    // 修复：使用环境和熔炉自适应计算体感温度
    int effective_temp = game.env_temp + (int)(game.furnace_temp * 0.2);
    sprintf_s(buf, "环境温度: %d C  |  熔炉: %d C  |  体感: %d C", game.env_temp, game.furnace_temp, effective_temp);
    outtextxy(10, 680, buf);

    // 底部升级按钮（绝对坐标）
    setfillcolor(RGB(70, 70, 150));
    fillrectangle(100, 650, 200, 690);
    fillrectangle(250, 650, 350, 690);
    fillrectangle(400, 650, 500, 690);
    settextcolor(BLACK);
    settextstyle(14, 0, _T("宋体"));
    outtextxy(120, 665, _T("升级矿场"));
    outtextxy(270, 665, _T("升级伐木场"));
    outtextxy(420, 665, _T("升级熔炉"));

    // 悬浮提示框
    if (game.hovered_target != NULL) {
        POINT pt;                      // 定义一个 POINT 结构体变量
        GetCursorPos(&pt);             // 获取鼠标屏幕坐标（正确用法）
        ScreenToClient(GetHWnd(), &pt);// 转换为窗口客户区坐标
        DrawHoverTooltip(game.hovered_target, pt.x, pt.y);
    }

    // 注意：战斗面板由组员D在战斗循环中自行调用 DrawCombatPanel，
    // 不在这里自动绘制，因为 GameState 中没有 fighters 和 current_boss。
    // 如果你希望在 UI 层自动检测战斗状态，可以让组员A添加一个标志，
    // 但根据当前 global.h，战斗面板由调用者控制更合理。
}

void RenderFrame() {
    cleardevice();
    DrawWorldLayer();
    DrawUI();
    FlushBatchDraw();
}