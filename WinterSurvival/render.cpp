// render.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "render.h"
#include "ui_panel.h"
#include "camera.h"          
#include <easyx.h>
#include <stdio.h>
#include <atlconv.h> 
#include <windows.h>

#pragma comment(lib, "msimg32.lib")

// 所有贴图句柄
static IMAGE img_human_normal, img_human_super;
static IMAGE img_mine, img_woodcamp, img_furnace;

// === 新增：加载 3 张全新图片句柄 ===
static IMAGE img_map;         // 地图背景
static IMAGE img_wood_icon;   // 木头图标
static IMAGE img_coal_icon;   // 煤炭图标

// 完美支持透明通道的 PNG 绘制函数 (彻底消除黑边与黑块)
void drawPNG(int x, int y, int w, int h, IMAGE* pSrcImg) {
    HDC dstDC = GetImageHDC(NULL);
    HDC srcDC = GetImageHDC(pSrcImg);
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(dstDC, x, y, w, h, srcDC, 0, 0, pSrcImg->getwidth(), pSrcImg->getheight(), bf);
}

void InitRender() {
    loadimage(&img_human_normal, _T("res/human_normal.png"));
    loadimage(&img_human_super, _T("res/human_super.png"));
    loadimage(&img_mine, _T("res/mine.png"));
    loadimage(&img_woodcamp, _T("res/famu.png"));
    loadimage(&img_furnace, _T("res/furnace.png"));

    // === 新增：在初始化时，成功把这 3 张图片加载到内存中 ===
    loadimage(&img_map, _T("res/map.png"));
    loadimage(&img_wood_icon, _T("res/wood.png"));
    loadimage(&img_coal_icon, _T("res/coal.png"));
}

void DrawWorldLayer() {
    Camera cam = game.camera;

    // 获取大地图四个顶点的屏幕投影坐标
    int world_left, world_top, world_right, world_bottom;
    WorldToScreen(0, 0, &world_left, &world_top);
    WorldToScreen(3000.0f, 3000.0f, &world_right, &world_bottom);

    // === 优化：使用真实的 map.png 贴图完美铺满 3000x3000px 大世界 (取代先前的绿色底色) ===
    drawPNG(world_left, world_top, world_right - world_left, world_bottom - world_top, &img_map);

    // 绘制微弱的网格参考线
    setlinecolor(RGB(180, 180, 180));
    for (int i = 500; i < 3000; i += 500) {
        int lx1, ly1, lx2, ly2;
        WorldToScreen((float)i, 0, &lx1, &ly1);
        WorldToScreen((float)i, 3000.0f, &lx2, &ly2);
        line(lx1, ly1, lx2, ly2);

        WorldToScreen(0, (float)i, &lx1, &ly1);
        WorldToScreen(3000.0f, (float)i, &lx2, &ly2);
        line(lx1, ly1, lx2, ly2);
    }

    // --- 2. 遍历绘制所有人类 ---
    const int HUMAN_BASE_SIZE = 50;
    Human* cur = game.head;
    while (cur != NULL) {
        int screen_x, screen_y;
        WorldToScreen(cur->world_x, cur->world_y, &screen_x, &screen_y);

        IMAGE* img = cur->is_superman ? &img_human_super : &img_human_normal;
        int draw_w = (int)(HUMAN_BASE_SIZE * cam.zoom);
        int draw_h = (int)(HUMAN_BASE_SIZE * cam.zoom);

        drawPNG(screen_x - draw_w / 2, screen_y - draw_h / 2, draw_w, draw_h, img);

        // 名字
        settextstyle(12, 0, _T("宋体"));
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);

        TCHAR name_buf[64];
        _stprintf_s(name_buf, _T("%S"), cur->name);
        outtextxy(screen_x - 20, screen_y - draw_h / 2 - 15, name_buf);

        // 血条
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

    // --- 3. 限制建筑尺寸并使用透明混色绘制 ---
    const int MINE_SIZE = 200;
    const int WOOD_SIZE = 200;
    const int FURNACE_SIZE = 450;

    // 大熔炉 (1500, 1500)
    int fx, fy;
    WorldToScreen(1500.0f, 1500.0f, &fx, &fy);
    int draw_w = (int)(FURNACE_SIZE * game.camera.zoom);
    int draw_h = (int)(FURNACE_SIZE * game.camera.zoom);
    drawPNG(fx - draw_w / 2, fy - draw_h / 2, draw_w, draw_h, &img_furnace);

    // 矿场 (600, 800)
    int mx, my;
    WorldToScreen(600.0f, 800.0f, &mx, &my);
    draw_w = (int)(MINE_SIZE * game.camera.zoom);
    draw_h = (int)(MINE_SIZE * game.camera.zoom);
    drawPNG(mx - draw_w / 2, my - draw_h / 2, draw_w, draw_h, &img_mine);

    // 伐木场 (2200, 2100)
    int wx, wy;
    WorldToScreen(2200.0f, 2100.0f, &wx, &wy);
    draw_w = (int)(WOOD_SIZE * game.camera.zoom);
    draw_h = (int)(WOOD_SIZE * game.camera.zoom);
    drawPNG(wx - draw_w / 2, wy - draw_h / 2, draw_w, draw_h, &img_woodcamp);
}

void DrawUI() {
    // 左上角资源面板
    setfillcolor(RGB(40, 40, 40));
    fillrectangle(10, 10, 260, 110);
    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));

    TCHAR buf[100];

    // === 优化：在绘制“木头”和“煤炭”文本的前面，绘制新引入的小贴图图标 (22x22像素) ===
    drawPNG(20, 18, 22, 22, &img_wood_icon);
    _stprintf_s(buf, _T("木头: %d"), game.wood);
    outtextxy(50, 20, buf); // 文字往后移动，给小图标腾出空间

    drawPNG(20, 43, 22, 22, &img_coal_icon);
    _stprintf_s(buf, _T("煤炭: %d"), game.coal);
    outtextxy(50, 45, buf); // 文字往后移动，给小图标腾出空间

    _stprintf_s(buf, _T("食物: %d"), game.meat);
    outtextxy(20, 70, buf);

    _stprintf_s(buf, _T("人口: %d"), game.population);
    outtextxy(150, 20, buf);

    // 体感温度自适应计算
    int effective_temp = game.env_temp + (int)(game.furnace_temp * 0.2);
    _stprintf_s(buf, _T("环境温度: %d C  |  熔炉: %d C  |  体感: %d C"), game.env_temp, game.furnace_temp, effective_temp);
    outtextxy(10, 680, buf);


    // 悬浮提示框
    if (game.hovered_target != NULL) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(GetHWnd(), &pt);
        DrawHoverTooltip(game.hovered_target, pt.x, pt.y);
    }

    // ==================== 2. 新增：渲染选中的升级弹窗 ====================
    if (selected_building != NULL) {
        DrawUpgradePanel(selected_building);
    }
}

void RenderFrame() {
    cleardevice();
    DrawWorldLayer();
    DrawUI();
    FlushBatchDraw();
}