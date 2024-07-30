// Copyright (c) 2024，D-Robotics.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hobot_tts/hobot_tts.h"

#include <alsa/asoundlib.h>

#include <fstream>
#include <iostream>

namespace hobot_tts {

HobotTTSNode::HobotTTSNode(rclcpp::Node::SharedPtr& nh) : nh_(nh) {
  nh_->declare_parameter<std::string>("playback_device", playback_device_name_);
  nh_->get_parameter<std::string>("playback_device", playback_device_name_);

  speaker_device_ = alsa_device_allocate();
  if (!speaker_device_) {
    RCLCPP_ERROR(nh_->get_logger(), "alloc speaker device error!");
    throw std::runtime_error("HobotTTSNode allocate alsa device failed");
  }
  speaker_device_->name = const_cast<char*>(playback_device_name_.c_str());
  ;
  speaker_device_->format = SND_PCM_FORMAT_S16;
  speaker_device_->direct = SND_PCM_STREAM_PLAYBACK;
  speaker_device_->rate = 16000;
  speaker_device_->channels = 2;
  speaker_device_->buffer_time = 0;  // use default buffer time
  speaker_device_->nperiods = 4;
  speaker_device_->period_size = 512;  // 1 period including 1024 frames

  auto ret = alsa_device_init(speaker_device_);
  if (ret < 0) {
    if (speaker_device_) free(speaker_device_);
    RCLCPP_ERROR(nh_->get_logger(), "alsa_device_init speaker failed. ret = %d",
                 ret);
  }

  nh_->declare_parameter<std::string>("topic_sub", topic_subscription_name_);
  nh_->get_parameter<std::string>("topic_sub", topic_subscription_name_);
  text_subscription_ = nh_->create_subscription<std_msgs::msg::String>(
      topic_subscription_name_, 10,
      std::bind(&HobotTTSNode::MessageCallback, this, std::placeholders::_1));

  processing_thread_ = std::thread(&HobotTTSNode::ProcessMessages, this);
  playback_thread_ = std::thread(&HobotTTSNode::PlaybackMessages, this);

  int err_code = 0;
  std::string tros_distro
      = std::string(std::getenv("TROS_DISTRO")? std::getenv("TROS_DISTRO") : "");
  tts_ =
      wetts_init(std::string("/opt/tros/" + tros_distro + "/lib/hobot_tts/tts_model").c_str(),
      "tts.flags", &err_code);
  struct audio_info info = wetts_audio_info(tts_);
  pcm_data_ = new char[info.max_len];

  RCLCPP_INFO_STREAM(nh_->get_logger(), "Sample rate: " << info.sample_rate);
  RCLCPP_INFO_STREAM(nh_->get_logger(), "Bit depth: " << info.bit_depth);
  RCLCPP_INFO_STREAM(nh_->get_logger(),
                     "Num of channels: " << info.num_channels);
  RCLCPP_INFO_STREAM(nh_->get_logger(),
                     "Max seconds of audio: " << info.max_dur_ms / 1000);
}

HobotTTSNode::~HobotTTSNode() {
  StopPlayback();

  if (pcm_data_) {
    delete[] pcm_data_;
  }

  if (tts_) {
    wetts_free(tts_);
  }

  if (speaker_device_) {
    alsa_device_deinit(speaker_device_);
    alsa_device_free(speaker_device_);
    speaker_device_ = nullptr;
  }
}

void HobotTTSNode::MessageCallback(const std_msgs::msg::String::SharedPtr msg) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (message_queue_.size() >= kMaxMessageQueueSize) {
    // Discard the oldest message if the queue size exceeds the limit
    message_queue_.pop();
  }
  message_queue_.push(msg);
  cv_.notify_one();
}

int HobotTTSNode::ConvertToPCM(const std::string& msg,
                               std::unique_ptr<float[]>& pcm_data,
                               int& pcm_size) {
  auto err_code = wetts_synthesis(tts_, msg.c_str(), 1, pcm_data_, &pcm_size);
  if (err_code != ERRCODE_TTS_SUCC) {
    RCLCPP_ERROR_STREAM(nh_->get_logger(),
                        "Synthesis failed with error code: " << err_code);
    return -1;
  }

  pcm_data.reset(new float[pcm_size]);
  memcpy(pcm_data.get(), pcm_data_, pcm_size * sizeof(float));

  return 0;
}

void HobotTTSNode::ProcessMessages() {
  auto splitString = [](std::string& input,
                        std::vector<std::string>& segments) {
    std::string segment;

    size_t startPos = 0;
    size_t index = 0;

    auto isChinesePunctuation = [](const std::string& str, size_t index) {
      return (str[index] == '\xEF' && str[index + 1] == '\xBC' &&
              (str[index + 2] == '\x8C' || str[index + 2] == '\x9F' ||
               str[index + 2] == '\x9A' || str[index + 2] == '\x81')) ||
             (str[index] == '\xE3' && str[index + 1] == '\x80' &&
              (str[index + 2] == '\x82' || str[index + 2] == '\x81'));
    };

    while (index < input.length()) {
      if (isChinesePunctuation(input, index)) {
        segment = input.substr(startPos, index - startPos);
        if (!segment.empty()) {
          segments.push_back(segment);
          segment.clear();
        }
        startPos = index + 3;
      }
      if (ispunct(input[index])) {
        segment = input.substr(startPos, index - startPos);
        if (!segment.empty()) {
          segments.push_back(segment);
          segment.clear();
        }
        startPos = index + 1;
      }
      if (isspace(input[index])) {
        input[index] = ' ';
      }
      if (isupper(input[index])) {
        input[index] = tolower(input[index]);
      }
      ++index;
    }

    segment = input.substr(startPos);
    if (!segment.empty()) {
      segments.push_back(segment);
    }
  };

  while (rclcpp::ok()) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock,
             [this] { return !message_queue_.empty() || stop_playback_; });

    if (stop_playback_) {
      break;
    }

    while (!message_queue_.empty()) {
      auto message = message_queue_.front();
      message_queue_.pop();

      std::vector<std::string> message_vector;
      splitString(message->data, message_vector);
      for (auto msg : message_vector) {
        std::unique_ptr<float[]> pcm_data;
        int pcm_size;
        auto ret = ConvertToPCM(msg, pcm_data, pcm_size);
        if (!ret) {
          std::lock_guard<std::mutex> playback_lock(playback_mutex_);
          if (playback_queue_.size() >= kMaxPlaybackQueueSize) {
            // Discard the oldest PCM data if the queue size exceeds the limit
            playback_queue_.pop();
          }
          playback_queue_.push(std::make_pair(std::move(pcm_data), pcm_size));
          cv_playback_.notify_one();
        }
      }
    }
  }
}

void HobotTTSNode::PlaybackMessages() {
  while (rclcpp::ok()) {
    std::unique_lock<std::mutex> lock(playback_mutex_);
    cv_playback_.wait(
        lock, [this] { return !playback_queue_.empty() || stop_playback_; });

    if (stop_playback_ && playback_queue_.empty()) {
      break;
    }

    while (!playback_queue_.empty()) {
      auto pcm_data = std::move(playback_queue_.front().first);
      auto pcm_size = playback_queue_.front().second;
      playback_queue_.pop();

      std::vector<int16_t> pcm_int16;
      auto pcm_float = pcm_data.get();
      for (int i = 0; i < pcm_size; i++) {
        pcm_int16.push_back(*pcm_float);
        pcm_int16.push_back(*pcm_float);
        pcm_float++;
      }

      if (speaker_device_) {
        snd_pcm_sframes_t frames = snd_pcm_bytes_to_frames(
            speaker_device_->handle, pcm_int16.size() * sizeof(int16_t));
        snd_pcm_prepare(speaker_device_->handle);  // 耗时0.1ms
        alsa_device_write(speaker_device_, pcm_int16.data(), frames);
        snd_pcm_drop(speaker_device_->handle);  // 耗时1ms
      }
    }
  }
}

void HobotTTSNode::StopPlayback() {
  if (!stop_playback_) {
    stop_playback_ = true;
    cv_.notify_one();
    cv_playback_.notify_one();
    if (processing_thread_.joinable()) {
      processing_thread_.join();
    }
    if (playback_thread_.joinable()) {
      playback_thread_.join();
    }
  }
}

}  // namespace hobot_tts
