#pragma once

#include <pulse/pulseaudio.h>
#include <atomic>
#include <queue>

#include "common.hpp"
#include "errors.hpp"
#include "settings.hpp"

#include "utils/math.hpp"
// fwd
struct pa_context;
struct pa_threaded_mainloop;
struct pa_cvolume;
typedef struct pa_context pa_context;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;

POLYBAR_NS
class logger;

DEFINE_ERROR(pulseaudio_error);

class pulseaudio {
  // events to add to our queue
  enum class evtype {
    NEW = 0,
    CHANGE,
    REMOVE,
    SERVER,
    /**
     * Disconnect and reconnect to the pulseaudio daemon
     */
    RECONNECT,
    /**
     * Event that does nothing.
     *
     * Can be used to make sure that update_volume is called
     */
    NOP,
  };
  using queue = std::queue<evtype>;

 public:
  explicit pulseaudio(const logger& logger, string&& sink_name, bool m_max_volume);
  ~pulseaudio() = default;

  pulseaudio(const pulseaudio& o) = delete;
  pulseaudio& operator=(const pulseaudio& o) = delete;

  const string& get_name();

  bool wait();
  int process_events();

  int get_volume();
  void set_volume(pa_volume_t percentage);
  void inc_volume(int delta_perc);
  void set_mute(bool mode);
  void toggle_mute();
  bool is_muted();
  bool is_disconnected();

 private:
  void connect();

  void update_volume();
  void throw_error(const string& msg);

  static void get_sink_volume_callback(pa_context* context, const pa_sink_info* info, int is_last, void* userdata);
  static void subscribe_callback(pa_context* context, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
  static void simple_callback(pa_context* context, int success, void* userdata);
  static void sink_info_callback(pa_context* context, const pa_sink_info* info, int eol, void* userdata);
  static void context_state_callback(pa_context* context, void* userdata);

  void wait_loop(pa_operation* op);

 private:
  struct mainloop_deleter {
    void operator()(pa_threaded_mainloop* loop);
  };

  struct context_deleter {
    void operator()(pa_context* context);
  };

  using mainloop_ptr = std::unique_ptr<pa_threaded_mainloop, mainloop_deleter>;
  using context_ptr = std::unique_ptr<pa_context, context_deleter>;

  struct mainloop_locker {
    explicit mainloop_locker(mainloop_ptr& loop) noexcept;

    ~mainloop_locker() noexcept;

    void unlock() noexcept;

   private:
    pa_threaded_mainloop* m_loop;
  };

 private:
  const logger& m_log;

  /**
   * Has context_state_callback signalled the mainloop
   *
   * The context_state_callback and connect function communicate via this variable
   */
  std::atomic_bool m_state_callback_signal{false};

  // used for temporary callback results
  int success{0};
  pa_cvolume cv;
  bool muted{false};
  std::atomic_bool m_disconnected{true};

  // default sink name
  static constexpr auto DEFAULT_SINK{"@DEFAULT_SINK@"};

  mainloop_ptr m_mainloop{nullptr};
  context_ptr m_context{nullptr};

  queue m_events;

  // specified sink name
  string spec_s_name;
  string s_name;
  uint32_t m_index{0};

  pa_volume_t m_max_volume{PA_VOLUME_UI_MAX};
};

POLYBAR_NS_END
