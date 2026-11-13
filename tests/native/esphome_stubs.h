// Minimal host-side entity/UART doubles. The production Yamaha .cpp is compiled unchanged.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <memory>
#include <cstring>
#include <string>
#include <vector>
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGI(...) ((void) 0)
#define ESP_LOGW(...) ::esphome::test_log_warning(__VA_ARGS__)
#define ESP_LOGE(...) ((void) 0)
#define ESP_LOGCONFIG(...) ((void) 0)
#define USE_API
#define TEMPLATABLE_VALUE(type, name) struct { template<typename... A> type value(A...){ return {}; } } name##_;

namespace esphome {
// The last warning, formatted as the logger would print it.
inline std::string test_last_warning;
__attribute__((format(printf, 2, 3))) inline void test_log_warning(const char *, const char *format, ...) {
  va_list args;
  va_start(args, format);
  va_list measure;
  va_copy(measure, args);
  const int size = std::vsnprintf(nullptr, 0, format, measure);
  va_end(measure);
  test_last_warning.assign(size > 0 ? static_cast<size_t>(size) : 0, '\0');
  if (size > 0)
    std::vsnprintf(&test_last_warning[0], test_last_warning.size() + 1, format, args);
  va_end(args);
}

struct TestEntity {
  bool has_state() const { return has_state_; }
  void set_has_state(bool value) { has_state_ = value; }
  unsigned publishes{};
 protected:
  bool has_state_{};
};
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
namespace switch_ { struct Switch : TestEntity { bool state{}; virtual void write_state(bool) = 0; void publish_state(bool v) { state = v; has_state_ = true; ++publishes; } }; }
namespace number { struct Number : TestEntity { float state{}; virtual void control(float) = 0; void publish_state(float v) { state = v; has_state_ = true; ++publishes; } }; }
namespace button { struct Button { virtual void press_action() = 0; }; }
namespace sensor { struct Sensor : TestEntity { float state{}; void publish_state(float v) { state = v; has_state_ = true; ++publishes; } }; }
namespace binary_sensor { struct BinarySensor : TestEntity { bool state{}; void publish_state(bool v) { state = v; has_state_ = true; ++publishes; } }; }
namespace text_sensor { struct TextSensor : TestEntity { std::string state; void publish_state(const std::string &v) { state = v; has_state_ = true; ++publishes; } }; }
namespace select {
struct Select : TestEntity {
  struct Traits {
    std::vector<const char *> options;
    void set_options(const std::vector<const char *> &v) { options = v; }
    const auto &get_options() const { return options; }
  } traits;
  size_t active_index_{};
  virtual void control(const std::string &) = 0;
  bool has_option(const std::string &v) const {
    return std::any_of(traits.options.begin(), traits.options.end(), [&](const char *x) { return v == x; });
  }
  std::string current_option() const { return has_state() ? traits.options.at(active_index_) : ""; }
  void publish_state(const std::string &v) {
    for (size_t index = 0; index < traits.options.size(); ++index) {
      if (v == traits.options[index]) { active_index_ = index; has_state_ = true; ++publishes; return; }
    }
  }
};
}
namespace api {
struct DisconnectRequest {};
struct TestClient {
  unsigned requests{};
  bool send_ok{true};
  bool failed{};
  bool is_connection_setup() const { return true; }
  bool send_message(DisconnectRequest) { ++requests; return send_ok; }
  void on_fatal_error() { failed = true; }
};
struct APIServer {
  std::vector<std::unique_ptr<TestClient>> clients;
  auto &active_clients() { return clients; }
};
inline APIServer *global_api_server = nullptr;
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
