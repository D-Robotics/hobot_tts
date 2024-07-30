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

#ifndef HOBOT_TTS_INCLUDE_H_
#define HOBOT_TTS_INCLUDE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "tts_api.h"
#include "utils/alsa_device.h"

namespace hobot_tts {

class HobotTTSNode {
 public:
  HobotTTSNode(rclcpp::Node::SharedPtr& nh);
  ~HobotTTSNode();

  void OnGetText(const std_msgs::msg::String::SharedPtr msg);

 private:
  rclcpp::Node::SharedPtr nh_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr text_subscription_;
  std::string topic_subscription_name_ = "/tts_text";
  std::string playback_device_name_ = "hw:0,1";

  void MessageCallback(const std_msgs::msg::String::SharedPtr msg);

  void ProcessMessages();

  void PlaybackMessages();

  void StopPlayback();

  int ConvertToPCM(const std::string& msg, std::unique_ptr<float[]>& pcm_data,
                   int& pcm_size);

  std::queue<std_msgs::msg::String::SharedPtr> message_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;

  std::queue<std::pair<std::unique_ptr<float[]>, int>> playback_queue_;
  std::mutex playback_mutex_;
  std::condition_variable cv_playback_;

  std::atomic<bool> stop_playback_{false};
  std::thread processing_thread_;
  std::thread playback_thread_;

  static constexpr size_t kMaxMessageQueueSize = 10;
  static constexpr size_t kMaxPlaybackQueueSize = 5;

  void* tts_ = nullptr;
  alsa_device_t* speaker_device_ = nullptr;
  char* pcm_data_ = nullptr;
};

}  // namespace hobot_tts

#endif
