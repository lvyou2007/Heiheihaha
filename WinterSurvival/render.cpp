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

    // --- 1. 先绘制大地图底色（绿地沙盘）以铺满 3000x3000px 视口 ---
    int world_left, world_top, world_right, world_bottom;
    WorldToScreen(0, 0, &world_left, &world_top);
    WorldToScreen(3000.0f, 3000.0f, &world_right, &world_bottom);

    setfillcolor(RGB(34, 110, 34)); // 深森林绿
    solidrectangle(world_left, world_top, world_right, world_bottom);

    // 绘制 3000x3000px 地图内的经纬参考线（每 500 像素一条格线，方便定位）
    setlinecolor(RGB(46, 125, 50));
    for (int i = 500; i < 3000; i += 500) {
        int lx1, ly1, lx2, ly2;
        WorldToScreen((float)i, 0, &lx1, &ly1);
        WorldToScreen((float)i, 3000.0f, &lx2, &ly2);
        line(lx1, ly1, lx2, ly2);

        WorldToScreen(0, (float)i, &lx1, &ly1);
        WorldToScreen(3000.0f, (float)i, &lx2, &ly2);
        line(lx1, ly1, lx2, ly2);
    }

    // --- 2. 遍历绘制所有人类 (保持原样) ---
    Human* cur = game.head;
    while (cur != NULL) {
        int screen_x, screen_y;
        WorldToScreen(cur->world_x, cur->world_y, &screen_x, &screen_y);

        IMAGE* img = cur->is_superman ? &img_human_super : &img_human_normal;
        int draw_w = (int)(img->getwidth() * cam.zoom);
        int draw_h = (int)(img->getheight() * cam.zoom);
        putimage(screen_x - draw_w / 2, screen_y - draw_h / 2, draw_w, draw_h, img, 0, 0, SRCCOPY);

        settextstyle(12, 0, _T("宋体"));
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outtextxy(screen_x - 20, screen_y - draw_h / 2 - 15, cur->name);

        float ratio = (float)cur->hp / cur->max_hp;
        int bar_width = (int)(50 * cam.zoom);
        int bar_height = (int)(6 * cam.zoom);
        int bar_left = screen_x - bar_width / 2;
        int bar_top = screen_y - draw_h / 2 - 8;
        setfillcolor(RGB(80, 80, 80));
        fillrectangle(bar_left, bar_top, bar_left + bar_width, bar_top + bar_height);
        setfillcolor(RGB(0, 200, 0));
        fillrectangle(bar_left, bar_top, bar_left + (int)(bar_width * ratio), bar_top + bar_height);

        cur = cur->next;
    }

    // --- 3. 核心重构：在代码中强制指定建筑物的“物理世界尺寸”，防止爆屏 ---
    const int MINE_SIZE = 200;      // 强制限制矿场大小为世界坐标下的 200x200
    const int WOOD_SIZE = 200;      // 强制限制伐木场大小为世界坐标下的 200x200
    const int FURNACE_SIZE = 450;   // 强制限制核心大熔炉大小为世界坐标下的 450x450

    // 建筑 A：大熔炉，完美固定在 3000x3000px 大地图的正中心点 (1500, 1500)
    int fx, fy;
    WorldToScreen(1500.0f, 1500.0f, &fx, &fy);
    int draw_w = (int)(FURNACE_SIZE * game.camera.zoom);
    int draw_h = (int)(FURNACE_SIZE * game.camera.zoom);
    putimage(fx - draw_w / 2, fy - draw_h / 2, draw_w, draw_h, &img_furnace, 0, 0, SRCCOPY);

    // 建筑 B：矿场，散落在左上方世界坐标 (600, 800) 处
    int mx, my;
    WorldToScreen(600.0f, 800.0f, &mx, &my);
    draw_w = (int)(MINE_SIZE * game.camera.zoom);
    draw_h = (int)(MINE_SIZE * game.camera.zoom);
    putimage(mx - draw_w / 2, my - draw_h / 2, draw_w, draw_h, &img_mine, 0, 0, SRCCOPY);

    // 建筑 C：伐木场，散落在右下方世界坐标 (2200, 2100) 处
    int wx, wy;
    WorldToScreen(2200.0f, 2100.0f, &wx, &wy);
    draw_w = (int)(WOOD_SIZE * game.camera.zoom);
    draw_h = (int)(WOOD_SIZE * game.camera.zoom);
    putimage(wx - draw_w / 2, wy - draw_h / 2, draw_w, draw_h, &img_woodcamp, 0, 0, SRCCOPY);
}

void DrawUI() {
    // 左上角资源面板
    setfillcolor(RGB(40, 40, 40));
    fillrectangle(10, 10, 260, 110);
    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));

    // === 修复点：将 char 改为自适应的 TCHAR 数组 ===
    TCHAR buf[100];

    // === 修复点：改用自适应的 _stprintf_s 和 _T 宏占位符 ===
    _stprintf_s(buf, _T("木头: %d"), game.wood);
    outtextxy(20, 20, buf);

    _stprintf_s(buf, _T("煤炭: %d"), game.coal);
    outtextxy(20, 45, buf);

    _stprintf_s(buf, _T("肉食: %d"), game.meat); // 改为 meat
    outtextxy(20, 70, buf);

    _stprintf_s(buf, _T("人口: %d"), game.population);
    outtextxy(150, 20, buf);

    // 温度 (结合熔炉热量进行自适应体感温度计算)
    int effective_temp = game.env_temp + (int)(game.furnace_temp * 0.2);
    _stprintf_s(buf, _T("环境温度: %d C  |  熔炉: %d C  |  体感: %d C"), game.env_temp, game.furnace_temp, effective_temp);
    outtextxy(10, 680, buf);

    // 底部升级按钮
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
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(GetHWnd(), &pt);
        DrawHoverTooltip(game.hovered_target, pt.x, pt.y);
    }
}
    // 注意：战斗面板由组员D在战斗循环中自行调用 DrawCombatPanel，
    // 不在这里自动绘制，因为 GameState 中没有 fighters 和 current_boss。
    // 如果你希望在 UI 层自动检测战斗状态，可以让组员A添加一个标志，
    // 但根据当前 global.h，战斗面板由调用者控制更合理。

void RenderFrame() {
    cleardevice();
    DrawWorldLayer();
    DrawUI();
    FlushBatchDraw();
}