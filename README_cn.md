[English](./README.md) | 简体中文

# Hobot TTS

**hobot_tts**提供将文本转化为语音播放功能，订阅文本消息，然后调用TTS软件接口，将文本转化为PCM数据，最后调用ALSA接口播放。

## 环境搭建

运行hobot_tts，需要确认音频设备设置正确，具体设置方法参考RDK用户手册[音频转接板](https://developer.horizon.cc/documents_rdk/hardware_development/rdk_x3/audio_board)章节。

可使用如下命令检查是否设置正确：

```bash
root@ubuntu:~# ls /dev/snd/
by-path  controlC0  pcmC0D0c  pcmC0D1p  timer
```

如果出现例如`pcmC0D1p`音频设备则表示设置正确。

## 运行方式

首次运行需要下载模型文件解压，详细命令如下：

```bash
wget http://archive.d-robotics.cc/tts-model/tts_model.tar.gz
sudo tar -xf tts_model.tar.gz -C /opt/tros/${TROS_DISTRO}/lib/hobot_tts/
```

启动程序：

```bash
source /opt/tros/setup.bash

# 屏蔽调式打印信息
export GLOG_minloglevel=1

ros2 run hobot_tts hobot_tts
```

运行成功后，程序订阅topic "/tts_text"（消息类型为std_msgs/msg/String），然后转化为语音信号播放。

注意：加载音频驱动时，若新增音频设备不是`pcmC0D1p`，例如`pcmC1D1p`，则需要使用参数`playback_device`指定播放音频设备。

## 参数列表

| 参数名          | 解释             | 类型        | 是否必须 | 默认值      |
| --------------- | ---------------- | ----------- | -------- | ----------- |
| topic_sub       | 订阅的文本topic  | std::string | 否       | "/tts_text" |
| playback_device | 语音播放音频设备 | std::string | 否       | "hw:0,1"    |
