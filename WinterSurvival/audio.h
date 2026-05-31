#ifndef AUDIO_H
#define AUDIO_H

// 初始化并循环播放背景音乐（文件需放在 exe 同目录或使用绝对路径）
void PlayBGM(const char* filename);

// 停止当前背景音乐
void StopBGM(void);

// 可选：调整音量 (0-1000)
void SetBGMVolume(int volume);

#endif
