// Minimal host-side entity/UART doubles. The production Yamaha .cpp is compiled unchanged.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGI(...) ((void) 0)
#define ESP_LOGW(...) ((void) 0)
#define ESP_LOGE(...) ((void) 0)
#define ESP_LOGCONFIG(...) ((void) 0)
#define TEMPLATABLE_VALUE(type, name) struct { template<typename... A> type value(A...){ return {}; } } name##_;

namespace esphome {
inline uint32_t test_now = 1000;
inline uint32_t millis() { return test_now; }
template<typename T> T clamp(T v, T lo, T hi) { return std::clamp(v, lo, hi); }
template<typename T> struct FixedVector : std::vector<T> { void init(size_t n) { this->reserve(n); } };
template<typename T> struct Parented { T *parent_{}; };
template<typename... T> struct Action { virtual void play(const T &...) = 0; };
namespace setup_priority { inline constexpr float DATA = 600; }
struct PollingComponent {
  virtual ~PollingComponent() = default;
  virtual void setup() {} virtual void update() {} virtual void loop() {}
  virtual void dump_config() {} virtual float get_setup_priority() const { return 0; }
  uint32_t get_update_interval() const { return 30000; }
};
namespace uart {
struct UARTDevice {
  std::vector<std::vector<uint8_t>> sent;
  int available() { return 0; } int read() { return -1; }
  void write_array(const uint8_t *p, size_t n) { sent.emplace_back(p, p + n); }
};
}
namespace switch_ { struct Switch { bool state{}; virtual void write_state(bool) = 0; void publish_state(bool v) { state = v; } }; }
namespace number { struct Number { float state{}; virtual void control(float) = 0; void publish_state(float v) { state = v; } }; }
namespace button { struct Button { virtual void press_action() = 0; }; }
namespace sensor { struct Sensor { float state{}; void publish_state(float v) { state = v; } }; }
namespace binary_sensor { struct BinarySensor { bool state{}; void publish_state(bool v) { state = v; } }; }
namespace text_sensor { struct TextSensor { std::string state; void publish_state(const std::string &v) { state = v; } }; }
namespace select {
struct Select {
  struct Traits {
    std::vector<const char *> options;
    void set_options(const std::vector<const char *> &v) { options = v; }
  } traits;
  std::string state;
  virtual void control(const std::string &) = 0;
  bool has_option(const std::string &v) const {
    return std::any_of(traits.options.begin(), traits.options.end(), [&](const char *x) { return v == x; });
  }
  void publish_state(const std::string &v) { state = v; }
};
}
namespace media_player {
enum MediaPlayerEntityFeature { BROWSE_MEDIA=1, STOP=2, MEDIA_ANNOUNCE=4, VOLUME_SET=8, VOLUME_STEP=16, VOLUME_MUTE=32, PREVIOUS_TRACK=64, NEXT_TRACK=128, PLAY=256 };
enum MediaPlayerState { MEDIA_PLAYER_STATE_IDLE, MEDIA_PLAYER_STATE_OFF };
enum MediaPlayerCommand { MEDIA_PLAYER_COMMAND_TURN_ON, MEDIA_PLAYER_COMMAND_TURN_OFF, MEDIA_PLAYER_COMMAND_TOGGLE, MEDIA_PLAYER_COMMAND_MUTE, MEDIA_PLAYER_COMMAND_UNMUTE, MEDIA_PLAYER_COMMAND_VOLUME_UP, MEDIA_PLAYER_COMMAND_VOLUME_DOWN, MEDIA_PLAYER_COMMAND_PLAY, MEDIA_PLAYER_COMMAND_NEXT, MEDIA_PLAYER_COMMAND_PREVIOUS };
struct MediaPlayerTraits { void clear_feature_flags(int) {} void add_feature_flags(int) {} void set_supports_turn_off_on(bool) {} };
struct MediaPlayerCall {
  std::optional<MediaPlayerCommand> command;
  std::optional<float> volume;
  std::optional<std::string> media_url;
  const auto &get_command() const { return command; }
  const auto &get_volume() const { return volume; }
  const auto &get_media_url() const { return media_url; }
};
struct MediaPlayer {
  MediaPlayerState state{}; float volume{};
  virtual MediaPlayerTraits get_traits() = 0;
  virtual bool is_muted() const { return false; }
  virtual void control(const MediaPlayerCall &) = 0;
  void publish_state() {}
};
}
}
