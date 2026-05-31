#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "winmm.lib")

static char current_music[256] = { 0 };

// 获取当前 exe 所在目录（不含 exe 文件名）
static void GetExeDirectory(char* out_path, size_t size) {
    GetModuleFileNameA(NULL, out_path, (DWORD)size);
    char* last_slash = strrchr(out_path, '\\');
    if (last_slash) *last_slash = '\0';
}

void PlayBGM(const char* filename) {
    char fullpath[512];
    char exe_dir[512];
    char cmd[512];
    char alias[256];

    // 1. 获取 exe 所在目录
    GetExeDirectory(exe_dir, sizeof(exe_dir));
    // 2. 拼接完整路径
    sprintf(fullpath, "%s\\%s", exe_dir, filename);

    // 3. 用文件名（不含扩展名）作为别名
    strcpy(alias, filename);
    char* dot = strrchr(alias, '.');
    if (dot) *dot = '\0';

    // 4. 关闭已有的同名音乐
    sprintf(cmd, "close %s", alias);
    mciSendStringA(cmd, NULL, 0, NULL);

    // 5. 打开音乐文件
    sprintf(cmd, "open \"%s\" alias %s", fullpath, alias);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        // 打开失败，弹出友好提示（仅 Debug 或遇到问题时显示）
        char errmsg[512];
        sprintf(errmsg, "无法打开音乐文件：\n%s\n\n请确保文件位于 exe 所在目录，或重新编译项目。", fullpath);
        MessageBoxA(NULL, errmsg, "背景音乐错误", MB_OK | MB_ICONWARNING);
        return;
    }

    // 6. 循环播放
    sprintf(cmd, "play %s repeat", alias);
    mciSendStringA(cmd, NULL, 0, NULL);

    strcpy(current_music, alias);
}

void StopBGM(void) {
    if (current_music[0] != '\0') {
        char cmd[512];
        sprintf(cmd, "close %s", current_music);
        mciSendStringA(cmd, NULL, 0, NULL);
        current_music[0] = '\0';
    }
}