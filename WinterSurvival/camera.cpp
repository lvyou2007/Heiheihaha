#include "camera.h"
extern Human* GetHoveredHuman(float click_world_x, float click_world_y, float radius);


// 坐标转换：将世界坐标转换为屏幕像素坐标
void WorldToScreen(float world_x, float world_y, int* screen_x, int* screen_y) {
    if (screen_x) *screen_x = (int)((world_x - game.camera.x) * game.camera.zoom);
    if (screen_y) *screen_y = (int)((world_y - game.camera.y) * game.camera.zoom);
}

// 坐标转换：将屏幕像素坐标转换为世界坐标
void ScreenToWorld(int screen_x, int screen_y, float* world_x, float* world_y) {
    if (world_x) *world_x = (float)screen_x / game.camera.zoom + game.camera.x;
    if (world_y) *world_y = (float)screen_y / game.camera.zoom + game.camera.y;
}

// 约束摄像机位置与缩放，防止超出 3000×3000 边界
static void ClampCamera() {
    int sw = getwidth();
    int sh = getheight();

    // 1. 限制最小缩放比例，防止地图缩得比屏幕还小而出现大片黑边
    float min_zoom_x = (float)sw / WORLD_WIDTH;
    float min_zoom_y = (float)sh / WORLD_HEIGHT;
    float min_zoom = (min_zoom_x > min_zoom_y) ? min_zoom_x : min_zoom_y;

    if (game.camera.zoom < min_zoom) {
        game.camera.zoom = min_zoom;
    }
    // 限制最大缩放倍数，防止过度放大
    if (game.camera.zoom > 10.0f) {
        game.camera.zoom = 10.0f;
    }

    // 计算当前缩放比例下，屏幕在世界中所占的视口宽高
    float view_w = (float)sw / game.camera.zoom;
    float view_h = (float)sh / game.camera.zoom;

    // 2. 限制 X 轴范围
    if (view_w >= WORLD_WIDTH) {
        // 如果视口比世界还宽，则将地图水平居中显示
        game.camera.x = (WORLD_WIDTH - view_w) / 2.0f;
    }
    else {
        if (game.camera.x < 0) {
            game.camera.x = 0;
        }
        else if (game.camera.x + view_w > WORLD_WIDTH) {
            game.camera.x = WORLD_WIDTH - view_w;
        }
    }

    // 3. 限制 Y 轴范围
    if (view_h >= WORLD_HEIGHT) {
        // 如果视口比世界还高，则将地图垂直居中显示
        game.camera.y = (WORLD_HEIGHT - view_h) / 2.0f;
    }
    else {
        if (game.camera.y < 0) {
            game.camera.y = 0;
        }
        else if (game.camera.y + view_h > WORLD_HEIGHT) {
            game.camera.y = WORLD_HEIGHT - view_h;
        }
    }
}

// 初始化摄像机，使其居中适应屏幕
void InitCamera() {
    int sw = getwidth();
    int sh = getheight();

    // 计算初始缩放：刚好能完整容纳 3000×3000 区域
    float zoom_x = (float)sw / WORLD_WIDTH;
    float zoom_y = (float)sh / WORLD_HEIGHT;
    game.camera.zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;

    // 居中世界坐标 (400, 300)
    float view_w = (float)sw / game.camera.zoom;
    float view_h = (float)sh / game.camera.zoom;
    game.camera.x = (WORLD_WIDTH - view_w) / 2.0f;
    game.camera.y = (WORLD_HEIGHT - view_h) / 2.0f;

    ClampCamera();
}

// 内部拖拽状态记录
static bool is_dragging = false;
static int last_mouse_x = 0;
static int last_mouse_y = 0;

void ProcessInput() {
    ExMessage msg;

    while (peekmessage(&msg, EM_MOUSE)) {

        // --- 1. 新增：处理鼠标左键点击（点击建筑与升级交互） ---
        if (msg.message == WM_LBUTTONDOWN) {

            // A. 如果升级弹窗已经打开，优先拦截并判定弹窗内部的点击事件
            if (selected_building != NULL) {
                // 弹窗绝对坐标：left=440, top=210, right=840, bottom=510

                // 1. 检测是否点击了右上角的 "X" 关闭圆圈 (圆心在 820, 230，半径 12)
                int dx = msg.x - 820;
                int dy = msg.y - 230;
                if (dx * dx + dy * dy <= 12 * 12) {
                    selected_building = NULL; // 关闭弹窗
                    continue;
                }

                // 2. 检测是否点击了底部的 "升级" 按钮 (540, 440) -> (740, 480)
                if (msg.x >= 540 && msg.x <= 740 && msg.y >= 440 && msg.y <= 480) {
                    if (selected_building->level < 3) {
                        if (game.wood >= selected_building->cost_wood) {
                            // 扣除木材并升级
                            game.wood -= selected_building->cost_wood;
                            selected_building->level++;

                            // 根据新等级，更新对应建筑的指标与下一级消耗
                            if (selected_building->type == 0) { // 矿场
                                if (selected_building->level == 2) {
                                    selected_building->produce_rate = 12;
                                    selected_building->cost_wood = 300;
                                }
                                else if (selected_building->level == 3) {
                                    selected_building->produce_rate = 25;
                                    selected_building->cost_wood = 9999; // 满级标记
                                }
                            }
                            else if (selected_building->type == 1) { // 伐木场
                                if (selected_building->level == 2) {
                                    selected_building->produce_rate = 30;
                                    selected_building->cost_wood = 250;
                                }
                                else if (selected_building->level == 3) {
                                    selected_building->produce_rate = 50;
                                    selected_building->cost_wood = 9999;
                                }
                            }
                            else if (selected_building->type == 2) { // 熔炉
                                if (selected_building->level == 2) {
                                    selected_building->produce_rate = 200; // 熔炉温度变200
                                    game.furnace_temp = 200;               // 同步全局状态
                                    selected_building->cost_wood = 450;
                                }
                                else if (selected_building->level == 3) {
                                    selected_building->produce_rate = 350; // 熔炉温度变350
                                    game.furnace_temp = 350;
                                    selected_building->cost_wood = 9999;
                                }
                            }
                        }
                        else {
                            MessageBox(GetHWnd(), _T("储备木材不足，无法升级！"), _T("提示"), MB_OK | MB_ICONWARNING);
                        }
                    }
                    continue;
                }

                // 3. 点击弹窗外部，自动关闭弹窗
                if (msg.x < 440 || msg.x > 840 || msg.y < 210 || msg.y > 510) {
                    selected_building = NULL;
                }
                continue; // 拦截，不触发地图建筑选择
            }

            // B. 弹窗未打开时，检测是否点击了世界地图上的实体建筑物
            float world_click_x, world_click_y;
            ScreenToWorld(msg.x, msg.y, &world_click_x, &world_click_y);

            // 1. 判定大熔炉 (中心在 1500, 1500，世界判定半径 225)
            float dx = world_click_x - 1500.0f;
            float dy = world_click_y - 1500.0f;
            if (dx * dx + dy * dy <= 225.0f * 225.0f) {
                selected_building = &furnace_build;
                continue;
            }

            // 2. 判定矿场 (中心在 600, 800，世界判定半径 100)
            dx = world_click_x - 600.0f;
            dy = world_click_y - 800.0f;
            if (dx * dx + dy * dy <= 100.0f * 100.0f) {
                selected_building = &mine_build;
                continue;
            }

            // 3. 判定伐木场 (中心在 2200, 2100，世界判定半径 100)
            dx = world_click_x - 2200.0f;
            dy = world_click_y - 2100.0f;
            if (dx * dx + dy * dy <= 100.0f * 100.0f) {
                selected_building = &wood_build;
                continue;
            }
        }

        // --- 2. 处理鼠标右键拖拽 (保持原样) ---
        if (msg.message == WM_RBUTTONDOWN) {
            is_dragging = true;
            last_mouse_x = msg.x;
            last_mouse_y = msg.y;
        }
        else if (msg.message == WM_RBUTTONUP) {
            is_dragging = false;
        }
        else if (msg.message == WM_MOUSEMOVE) {
            if (is_dragging) {
                int dx = msg.x - last_mouse_x;
                int dy = msg.y - last_mouse_y;

                game.camera.x -= (float)dx / game.camera.zoom;
                game.camera.y -= (float)dy / game.camera.zoom;

                last_mouse_x = msg.x;
                last_mouse_y = msg.y;

                ClampCamera();
            }

            float world_mouse_x, world_mouse_y;
            ScreenToWorld(msg.x, msg.y, &world_mouse_x, &world_mouse_y);
            game.hovered_target = GetHoveredHuman(world_mouse_x, world_mouse_y, 25.0f);
        }

        // --- 3. 处理滚轮缩放 (保持原样) ---
        else if (msg.message == WM_MOUSEWHEEL) {
            float zoom_factor = 1.1f;
            float old_zoom = game.camera.zoom;
            float new_zoom = old_zoom;

            if (msg.wheel > 0) new_zoom *= zoom_factor;
            else if (msg.wheel < 0) new_zoom /= zoom_factor;

            if (new_zoom != old_zoom) {
                float mouse_world_x = (float)msg.x / old_zoom + game.camera.x;
                float mouse_world_y = (float)msg.y / old_zoom + game.camera.y;

                game.camera.zoom = new_zoom;
                game.camera.x = mouse_world_x - (float)msg.x / new_zoom;
                game.camera.y = mouse_world_y - (float)msg.y / new_zoom;

                ClampCamera();
            }
        }
    }
}