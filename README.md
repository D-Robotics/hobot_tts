English| [简体中文](./README_cn.md)

# Hobot TTS

**hobot_tts** provides the function to convert text into speech for audio playback. It subscribes to text messages, then calls the TTS software interface to convert the text into PCM data, and finally uses the ALSA interface for playback.

## Environment Setup

To run hobot_tts, it is necessary to confirm that the audio device is set up correctly. Refer to the RDK User Manual [Audio Adapter](https://developer.horizon.cc/documents_rdk/hardware_development/rdk_x3/audio_board) section for specific setup methods.

You can use the following command to check if the settings are correct:

```bash
root@ubuntu:~# ls /dev/snd/
by-path  controlC0  pcmC0D0c  pcmC0D1p  timer
```

If an audio device such as `pcmC0D1p` appears, it means the settings are correct.

## How to Run

To run for the first time, download and extract the model file. The detailed commands are as follows:

```bash
wget http://archive.d-robotics.cc/tros/tts-model/tts_model.tar.gz
sudo tar -xf tts_model.tar.gz -C /opt/tros/${TROS_DISTRO}/lib/hobot_tts/
```

Start the program:

```bash
source /opt/tros/setup.bash

# Suppress debug printing information
export GLOG_minloglevel=1

ros2 run hobot_tts hobot_tts
```

After successful execution, the program subscribes to the topic "/tts_text" (message type is std_msgs/msg/String) and converts it into speech signal for playback.

Note: When loading the audio driver, if the new audio device is not `pcmC0D1p`, for example `pcmC1D1p`, you need to use the `playback_device` parameter to specify the playback audio device.

## Parameter List

| Parameter Name  | Explanation              | Type         | Required | Default Value |
| --------------- | ------------------------ | ------------ | -------- | ------------- |
| topic_sub       | Subscribed text topic    | std::string  | No       | "/tts_text"   |
| playback_device | Audio playback device     | std::string  | No       | "hw:0,1"      |