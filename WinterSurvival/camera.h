#pragma once
#include "global.h"
// 初始化摄像机位置与缩放，使其适配 800x600 的世界
void InitCamera();
// 坐标转换辅助函数
void WorldToScreen(float world_x, float world_y, int* screen_x, int* screen_y);
void ScreenToWorld(int screen_x, int screen_y, float* world_x, float* world_y);
// 你的接口声明
void ProcessInput(); // 处理鼠标拖拽和滚轮缩放
