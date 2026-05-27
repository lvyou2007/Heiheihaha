#define _CRT_SECURE_NO_WARNINGS

#include "global.h"
#include "camera.h"      // 提供 ProcessInput(), InitCamera()
#include "render.h"      // 提供 DrawMainMenu(), DrawWorld(), DrawCityUI(), DrawCombatPanel()
#include "human_logic.h" // 提供 InitHumanList(), CreateHuman(), AddHumanToList(), DailySettlement(), FreeAllHumans()
#include "combat_sys.h"  // 提供 ProcessCombatRound()

#include <Windows.h>     // 提供高精度计时器 API
#include <stdbool.h>
#include<stdio.h>
#include <time.h>        // 提供 time() 用于随机数种子

// 链接 Windows 多媒体库，用于提升 Sleep 函数的精度
#pragma comment(lib, "winmm.lib")

// 1. 全局变量定义 (在 global.h 中声明为 extern)
GameState game;

// ==========================================
// === 真实游戏数据初始化 (替代之前的假数据) ===
// ==========================================
void InitGameData() {
    // 1. 设置随机数种子 (极其重要！确保组员C每次生成的变异和随机数都不一样)
    srand((unsigned int)time(NULL));

    // 为了方便我们现在测试画面，直接跳过菜单，进入城市大地图
    game.current_state = STATE_CITY;

    // 2. 初始化初始资源
    game.wood = 100;
    game.coal = 50;
    game.meat = 50;
    game.current_temp = 20;

    // 3. 调用组员 C 的真实逻辑：初始化空链表 (这步替代了旧的 human_list = NULL)
    InitHumanList();

    // 4. 调用组员 C 的真实逻辑：生成 5 名初始幸存者加入部落！
    for (int i = 0; i < 5; i++) {
        Human* starter = CreateHuman(1); // 初始生成等级为 1 的人类
        AddHumanToList(starter);
    }

    // 初始状态下没有鼠标悬浮目标
    game.hovered_target = NULL;
}


// 2. 游戏主循环控制入口
int main() {
    // --- 【引擎初始化阶段】 ---

    // 初始化 1280x720 像素分辨率的游戏窗口
    initgraph(1280, 720);

    // 启用 EasyX 双缓冲机制，避免画面闪烁
    BeginBatchDraw();

    // 提升 Windows 系统线程调度和 Sleep 的精度到 1 毫秒
    timeBeginPeriod(1);

    // ==========================================
    // === 核心底层系统初始化 (在心跳开始前) ===
    // ==========================================

    // 1. 摄像机初始化：计算初始缩放比例并居中，防止缩放为 0 导致黑屏崩溃
    InitCamera();

    // 2. 数据初始化：生成真实的人口链表、资源
    InitGameData();

    // ==========================================


    // --- 【时间解耦与帧率控制变量】 ---

    // 每日结算时间戳寄存器 (单位：毫秒)
    DWORD last_settlement_time = GetTickCount();

    // 战斗回合判定时间戳寄存器 (单位：毫秒)
    DWORD last_combat_round_time = GetTickCount();

    // 高精度 60 FPS 帧控变量
    LARGE_INTEGER cpu_frequency;
    LARGE_INTEGER frame_start_time, frame_end_time;

    QueryPerformanceFrequency(&cpu_frequency);
    const double target_frame_seconds = 1.0 / 60.0; // 每帧 16.67ms
    QueryPerformanceCounter(&frame_start_time);

    // 运行主循环标识
    bool is_running = true;

    // --- 【游戏主循环 (Game Loop) 开始】 ---
    while (is_running) {

        // 1. 【输入处理层】 
        // 每一帧不论处于何种状态，都首先调用摄像机和鼠标键盘输入管理器
        ProcessInput();

        DWORD current_time = GetTickCount();

        // 2. 【状态机调度层 (State Machine)】
        switch (game.current_state) {

        case STATE_MENU: {
            // DrawMainMenu();
            break;
        }

        case STATE_CITY: {
            // DrawWorld();   
            // DrawCityUI();  

            // --- 【时间解耦】 每日结算逻辑每 1000 毫秒（1秒）执行一次 ---
            if (current_time - last_settlement_time >= 1000) {

                // 【调用组员C的内政引擎】处理饥饿、寒冷、繁衍、升级！
                DailySettlement();

                last_settlement_time = current_time; // 刷新结算时间戳

                // 【调试输出】在控制台打印当前存活人口，证明后台逻辑在跑！
                printf("度过了一天！当前存活人数: %d 人\n", game.population);
            }
            break;
        }

        case STATE_COMBAT: {
            // DrawWorld();
            // DrawCombatPanel();

            // --- 【时间解耦】 战斗回合每 500 毫秒（0.5秒）执行一次伤害判定 ---
            if (current_time - last_combat_round_time >= 500) {
                // ProcessCombatRound(); 
                last_combat_round_time = current_time;
            }
            break;
        }

        case STATE_GAMEOVER: {
            // 游戏结束状态：释放人员双向链表，清理内存，终止游戏主循环
            FreeAllHumans();
            is_running = false;
            break;
        }
        }

        // 3. 【画面呈递层】
        FlushBatchDraw();

        // 4. 【高精度 FPS 锁定器与 Sleep 补偿】
        QueryPerformanceCounter(&frame_end_time);
        double frame_elapsed_seconds = (double)(frame_end_time.QuadPart - frame_start_time.QuadPart) / cpu_frequency.QuadPart;

        if (frame_elapsed_seconds < target_frame_seconds) {
            double remaining_seconds = target_frame_seconds - frame_elapsed_seconds;
            DWORD sleep_ms = (DWORD)(remaining_seconds * 1000.0);
            if (sleep_ms > 1) {
                Sleep(sleep_ms - 1);
            }

            double final_elapsed = 0.0;
            do {
                QueryPerformanceCounter(&frame_end_time);
                final_elapsed = (double)(frame_end_time.QuadPart - frame_start_time.QuadPart) / cpu_frequency.QuadPart;
            } while (final_elapsed < target_frame_seconds);
        }

        frame_start_time = frame_end_time;
    }

    // --- 【清理收尾阶段】 ---
    EndBatchDraw();
    timeEndPeriod(1);
    closegraph();

    // 如果是直接点关闭窗口退出的，保险起见再调用一次释放内存
    FreeAllHumans();
    return 0;
}