#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")

static char current_music[256] = { 0 };  // 记录当前播放的文件别名

void PlayBGM(const char* filename) {
    char fullpath[512];
    char exePath[512];

    // 获取当前 exe 的完整路径（包括文件名）
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    // 去掉 exe 文件名，只保留目录路径
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0';

    // 拼接音乐文件的完整路径
    sprintf(fullpath, "%s\\%s", exePath, filename);

    char cmd[512];
    char alias[256];
    strcpy(alias, filename);
    char* dot = strrchr(alias, '.');
    if (dot) *dot = '\0';

    sprintf(cmd, "close %s", alias);
    mciSendStringA(cmd, NULL, 0, NULL);

    sprintf(cmd, "open \"%s\" alias %s", fullpath, alias);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        // 打开失败，静默返回
        return;
    }

    sprintf(cmd, "play %s repeat", alias);
    mciSendStringA(cmd, NULL, 0, NULL);
}

void StopBGM(void) {
    if (current_music[0] != '\0') {
        char cmd[512];
        sprintf(cmd, "close %s", current_music);
        mciSendStringA(cmd, NULL, 0, NULL);
        current_music[0] = '\0';
    }
}

void SetBGMVolume(int volume) {
    if (current_music[0] != '\0') {
        char cmd[512];
        // volume: 0-1000 (对应 0-100%)
        sprintf(cmd, "setaudio %s volume to %d", current_music, volume);
        mciSendStringA(cmd, NULL, 0, NULL);
    }
}