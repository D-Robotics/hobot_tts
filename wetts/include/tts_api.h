// Copyright 2024 D-Robotics Inc. All Rights Reserved.
// Author: zhendong.peng@d-robotics.cc (Zhendong Peng)

#ifndef MODEL_TTS_API_H_
#define MODEL_TTS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

enum en_rec_err_code {
  ERRCODE_TTS_SUCC = 0x10000,           // 正常
  ERRCODE_TTS_INVALID_INPUT = 0x10001,  // 输入参数无效
  ERRCODE_TTS_LONG_INPUT = 0x10002,     // 输入文本过长
  ERRCODE_TTS_INIT_MODEL = 0x10003,     // 初始化模型失败
  ERRCODE_TTS_MODEL_FILE = 0x10004,  // 模型文件及配置文件路径不存在
  ERRCODE_TTS_INVALID_OUTPUT = 0x10005,  // 输出无效
};

struct audio_info {
  int sample_rate;
  int bit_depth;
  int num_channels;
  int max_dur_ms;  // 合成音频的最大时长，单位: ms
  int max_len;  // 用于申请内存存储返回的采样点，单位是字节
};

// 初始化语音合成模块（包括前端和后端模型）
// @param model_path: 模型相关路径
// @param top_dir: 模型和配置文件所在的文件夹路径
// @param file: 配置文件
// @param errcode: 返回错误码
// @return void*: 返回 tts 指针
void* wetts_init(const char* top_dir, const char* file, int* errcode)
    __attribute__((visibility("default")));

// 释放资源模块
// @param speaker: wetts_init 初始化返回的指针
void wetts_free(void* tts) __attribute__((visibility("default")));

// 获取 tts 模型对应音频的采样率
// @param tts: wetts_init 初始化返回的指针
// @return struct audio_info: 返回合成音频的信息
struct audio_info wetts_audio_info(void* tts)
    __attribute__((visibility("default")));

// 后端语音合成
// @param tts: wetts_init 初始化返回的指针
// @param text: 待合成的文字序列
// @param speaker_id: 说话人的 id
// @param pcm_data: 返回合成的 pcm 数据，需要通过 reinterpret_cast 转为 float
// @param pcm_size: 返回 float pcm 的长度
// @return errcode: 返回错误码
int wetts_synthesis(void* tts, const char* text, const int speaker_id,
                    char* pcm_data, int* pcm_size)
    __attribute__((visibility("default")));

#ifdef __cplusplus
}
#endif

#endif  // MODEL_TTS_API_H_
