#define _CRT_SECURE_NO_WARNINGS
#include "global.h"
#include "camera.h"
#include "combat_sys.h"
#include "human_logic.h" // 确保能调用 InitHumanList, AddHumanToList, CreateHuman 等
#include <stdio.h>
#include <time.h>

GameState game;

// 模拟初始化 3 个测试人员（组员 C 的链表生成）
void SetupTestHumans() {
    InitHumanList();

    // 1号普通人
    Human* h1 = (Human*)malloc(sizeof(Human));
    h1->id = 1;
    strcpy(h1->name, "John (Normal)");
    h1->world_x = 100.0f; h1->world_y = 100.0f;
    h1->max_hp = 100; h1->hp = 100;
    h1->atk = 15; h1->def = 5;
    h1->hunger = 100; h1->exp = 0; h1->level = 1;
    h1->is_superman = 0; h1->prev = h1->next = NULL;
    AddHumanToList(h1);

    // 2号普通人
    Human* h2 = (Human*)malloc(sizeof(Human));
    h2->id = 2;
    strcpy(h2->name, "Mike (Normal)");
    h2->world_x = 120.0f; h2->world_y = 110.0f;
    h2->max_hp = 100; h2->hp = 100;
    h2->atk = 15; h2->def = 5;
    h2->hunger = 100; h2->exp = 0; h2->level = 1;
    h2->is_superman = 0; h2->prev = h2->next = NULL;
    AddHumanToList(h2);

    // 3号超凡人类
    Human* h3 = (Human*)malloc(sizeof(Human));
    h3->id = 3;
    strcpy(h3->name, "Clark (Super)");
    h3->world_x = 150.0f; h3->world_y = 150.0f;
    h3->max_hp = 150; h3->hp = 150;
    h3->atk = 30; h3->def = 10;
    h3->hunger = 100; h3->exp = 90; // 故意给 90 经验，方便测试一战后立刻升级
    h3->is_superman = 1; h3->prev = h3->next = NULL;
    AddHumanToList(h3);
}

// 核心可视化测试面板绘制（自适应 Unicode 版本）
void RenderTestUI() {
    cleardevice();
    setbkmode(TRANSPARENT);

    // 1. 绘制基本控制说明
    settextcolor(RGB(255, 255, 255));
    settextstyle(20, 0, _T("微软雅黑"));
    outtextxy(50, 40, _T("--- WINTER SURVIVAL 战斗与读写档集成测试面板 ---"));

    settextcolor(RGB(0, 255, 255));
    settextstyle(16, 0, _T("Consolas"));
    outtextxy(50, 90, _T("【控制指令】: [F] 触发战斗回合(攻防判定)  |  [S] 保存游戏(save.dat)  |  [L] 读取游戏(save.dat)"));
    outtextxy(156, 120, _T("[R] 重置生成一只 300HP 的暴雪巨兽 (BOSS)"));

    // 2. 绘制资源数据
    settextcolor(RGB(255, 255, 0));
    TCHAR res_buf[128]; // 改为自适应 TCHAR
    _stprintf_s(res_buf, _T("全局肉食资源 (Meat): %d  |  总人口: %d"), game.meat, game.population);
    outtextxy(50, 160, res_buf);

    // 3. 绘制野怪 BOSS 状态
    int boss_hp = GetDebugBossHp();
    int boss_max_hp = GetDebugBossMaxHp();
    if (boss_hp > 0) {
        settextcolor(RGB(255, 50, 50));
        TCHAR boss_buf[128];
        // 由于 GetDebugBossName() 返回的是 char*，在宽字符格式化下需要用 %S (大写)
        _stprintf_s(boss_buf, _T("【BOSS】: %S  |  HP: %d / %d  |  (战斗激活中)"), GetDebugBossName(), boss_hp, boss_max_hp);
        outtextxy(50, 210, boss_buf);

        // 绘制 BOSS 血条
        setfillcolor(RGB(100, 0, 0));
        solidrectangle(50, 240, 450, 255);
        setfillcolor(RGB(255, 0, 0));
        int width = (int)(400.0f * ((float)boss_hp / boss_max_hp));
        solidrectangle(50, 240, 50 + width, 255);
    }
    else {
        settextcolor(RGB(100, 100, 100));
        outtextxy(50, 210, _T("【BOSS】: 已死亡 / 未生成"));
    }

    // 4. 绘制当前出战人员 (Fighters) 的状态
    settextcolor(RGB(100, 255, 100));
    outtextxy(50, 290, _T("--- 当前前线参战人员 (Fighters 数组状态) ---"));

    int f_count = GetDebugFighterCount();
    if (f_count == 0) {
        settextcolor(RGB(150, 150, 150));
        outtextxy(50, 320, _T("(暂无参战人员，输入 F 开始战斗后，系统将自动抽取 30% 存活人口出战)"));
    }
    else {
        for (int i = 0; i < f_count; i++) {
            Human* f = GetDebugFighter(i);
            TCHAR fighter_buf[256];
            settextcolor(f->hp > 0 ? RGB(100, 255, 100) : RGB(255, 0, 0));
            // 同样，f->name 是 char*，格式化时改用 %S
            _stprintf_s(fighter_buf, _T("Fighter [%d]: ID:%d | %S | HP: %d/%d | ATK:%d | EXP: %d/100 | LV: %d %s"),
                i, f->id, f->name, f->hp, f->max_hp, f->atk, f->exp, f->level,
                f->hp > 0 ? _T("") : _T("(DEAD)"));
            outtextxy(50, 320 + i * 25, fighter_buf);
        }
    }

    // 5. 绘制后方全部人口状态
    settextcolor(RGB(200, 200, 200));
    outtextxy(50, 500, _T("--- 后方全部人口状态 (链表完整状态) ---"));
    Human* curr = game.head;
    int idx = 0;
    while (curr) {
        TCHAR human_buf[256];
        _stprintf_s(human_buf, _T("Human: ID:%d | %S | HP: %d/%d | LV: %d | EXP: %d"),
            curr->id, curr->name, curr->hp, curr->max_hp, curr->level, curr->exp);
        outtextxy(50, 530 + idx * 25, human_buf);
        curr = curr->next;
        idx++;
    }
}
int main() {
    // 创建窗口与初始状态
    initgraph(1024, 768);
    srand((unsigned)time(NULL));

    game.current_state = STATE_COMBAT;
    game.meat = 10;
    game.env_temp = -10;
    game.furnace_temp = 150;

    // 1. 初始化 3 个测试人类
    SetupTestHumans();

    // 2. 初始化生成一只高血量 Boss
    DebugSpawnBoss("暴雪冰晶巨兽", 300, 40, 15, 80, 50);

    BeginBatchDraw();
    bool running = true;
    while (running) {
        ExMessage msg;
        while (peekmessage(&msg, EM_KEY)) {
            if (msg.message == WM_KEYDOWN) {
                // 按 F 键：手动触发一次回合制战斗
                if (msg.vkcode == 'F') {
                    ProcessCombatRound();
                }
                // 按 S 键：保存游戏到本地二进制文件
                else if (msg.vkcode == 'S') {
                    if (SaveGame("save.dat")) {
                        MessageBox(GetHWnd(), _T("游戏存档保存成功！(已创建 save.dat)"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                    }
                }
                // 按 L 键：从本地二进制读取存档
                else if (msg.vkcode == 'L') {
                    if (LoadGame("save.dat")) {
                        MessageBox(GetHWnd(), _T("游戏读档成功！(已恢复场景、Boss血量、并完美重建参战指针)"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                    }
                }
                // 按 R 键：复活怪物便于反复测试
                else if (msg.vkcode == 'R') {
                    DebugSpawnBoss("暴雪冰晶巨兽", 300, 40, 15, 80, 50);
                }
            }
        }

        RenderTestUI();
        FlushBatchDraw();
        Sleep(30);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}