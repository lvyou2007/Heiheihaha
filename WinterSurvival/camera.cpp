#include "camera.h"



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

        // --- 1. 处理鼠标拖拽 ---
// 修改后：直接响应右键，不需要按 Ctrl 也不用按中键
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

                ClampCamera(); // 拖拽后应用边界限制
            }
        }

        // --- 2. 处理滚轮缩放 ---
        else if (msg.message == WM_MOUSEWHEEL) {
            float zoom_factor = 1.1f;
            float old_zoom = game.camera.zoom;
            float new_zoom = old_zoom;

            if (msg.wheel > 0) {
                new_zoom *= zoom_factor;
            }
            else if (msg.wheel < 0) {
                new_zoom /= zoom_factor;
            }

            // 此处不直接在这里限制缩放，而是让 ClampCamera 统一处理边界和最小缩放
            if (new_zoom != old_zoom) {
                float mouse_world_x = (float)msg.x / old_zoom + game.camera.x;
                float mouse_world_y = (float)msg.y / old_zoom + game.camera.y;

                game.camera.zoom = new_zoom;
                game.camera.x = mouse_world_x - (float)msg.x / new_zoom;
                game.camera.y = mouse_world_y - (float)msg.y / new_zoom;

                ClampCamera(); // 缩放后应用边界限制
            }
        }
    }
}