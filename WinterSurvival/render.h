// render.h
#pragma once
#include "global.h"

// 初始化渲染资源（加载贴图等）
void InitRender();

// 主渲染入口（每帧调用）
void RenderFrame();

// 世界层绘制（人物、建筑）
void DrawWorldLayer();

// UI层绘制（资源条、按钮、悬浮面板、战斗面板）
void DrawUI();