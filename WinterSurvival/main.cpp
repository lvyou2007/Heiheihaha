#define _CRT_SECURE_NO_WARNINGS
#include "global.h"
#include <stdio.h>
#include <stdlib.h> 
#define _CRT_SECURE_NO_WARNINGS 
#include "camera.h"      // 引入组员A的模块
#include "render.h"      // 引入组员 B 的模块
#include "human_logic.h" // 引入组员 C 的模块
#include "combat_sys.h"  // 引入组员 D 的模块

// ... 后面的代码保持不变
// 【极其重要】全项目唯一一次定义 game 变量的地方
GameState game;

// 制造假数据，供初期开发测试用
void InitFakeData() {
    game.current_state = STATE_CITY; // 直接进入城市测试
    game.camera.x = 0;
    game.camera.y = 0;
    game.camera.zoom = 1.0f;

    game.wood = 50;
    game.coal = 100;
    game.env_temp = -15.0f;

    // 造一个假人：张三
    game.human_list = (Human*)malloc(sizeof(Human));
    game.human_list->id = 1;
    strcpy(game.human_list->name, "超凡·张三");
    game.human_list->type = ROLE_SUPER;
    game.human_list->hp = 80; game.human_list->max_hp = 100;
    game.human_list->atk = 15; game.human_list->def = 5;
    game.human_list->level = 3;
    game.human_list->world_x = 640; game.human_list->world_y = 360;
    game.human_list->prev = NULL;
    game.human_list->next = NULL;
    game.total_humans = 1;

    // 默认让鼠标悬浮在张三身上，方便组员 B 测试画人物面板
    game.hovered_target = game.human_list;
}

int main() {
    // 1. 初始化 1280x720 窗口
    initgraph(1280, 720, EX_SHOWCONSOLE); // EX_SHOWCONSOLE 方便看 printf 报错

    // 2. 初始化数据
    InitFakeData();

    // 3. 开启双缓冲，防止画面闪烁
    BeginBatchDraw();

    // 4. 游戏主循环 (Game Loop)
    while (true) {
        // [未来填入] 组员 A 的工作：处理鼠标输入、摄像机缩放
        // ProcessInput();

        switch (game.current_state) {
        case STATE_MENU:
            // [未来填入] 组员 B: DrawMainMenu();
            break;

        case STATE_CITY:
            // [未来填入] 组员 C: DailySettlement();
            // [未来填入] 组员 B: DrawWorld(); DrawCityUI(); DrawHoverTooltip();

            // 占位测试画面 (确保窗口没卡死)
            cleardevice();
            outtextxy(500, 300, _T("这里是城市大地图界面，等待组员B渲染..."));
            break;

        case STATE_COMBAT:
            // [未来填入] 组员 D: ProcessCombatRound();
            // [未来填入] 组员 B: DrawCombatPanel();
            break;
        }

        // 5. 将这帧画面推送到屏幕
        FlushBatchDraw();

        // 6. 控制帧率 (约 60FPS)
        Sleep(16);
    }

    EndBatchDraw();
    closegraph();
    // 【新增】在程序彻底关闭前，呼叫 组员C 的函数清理内存！
    FreeAllHumans();
    return 0;
}