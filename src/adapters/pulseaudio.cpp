#include "adapters/pulseaudio.hpp"
#include "components/logger.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Construct pulseaudio object
 */
pulseaudio::pulseaudio(const logger& logger, string&& sink_name, bool max_volume)
    : m_log(logger), spec_s_name(sink_name) {
  m_max_volume = max_volume ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;
  connect();
}

void pulseaudio::connect() {
  // Clear eventqueue
  queue().swap(m_events);

  m_mainloop.reset(pa_threaded_mainloop_new());
  if (!m_mainloop) {
    throw pulseaudio_error("Could not create pulseaudio threaded mainloop.");
  }
  mainloop_locker guard(m_mainloop);

  pa_context* context = pa_context_new(pa_threaded_mainloop_get_api(m_mainloop.get()), "polybar");
  if (!context) {
    throw pulseaudio_error("Could not create pulseaudio context.");
  }
  m_context.reset(context);

  pa_context_set_state_callback(m_context.get(), context_state_callback, this);

  m_state_callback_signal = false;
  if (pa_context_connect(m_context.get(), nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
    throw pulseaudio_error("Could not connect pulseaudio context.");
  }

  if (pa_threaded_mainloop_start(m_mainloop.get()) < 0) {
    throw pulseaudio_error("Could not start pulseaudio mainloop.");
  }

  m_log.trace("pulseaudio: started mainloop");

  /*
   * Only wait for signal from the context state callback, if it has not
   * already signalled the mainloop since pa_context_connect was called
   */
  if (!m_state_callback_signal) {
    pa_threaded_mainloop_wait(m_mainloop.get());
  }

  if (pa_context_get_state(m_context.get()) != PA_CONTEXT_READY) {
    throw pulseaudio_error("Could not connect to pulseaudio server.");
  }

  pa_operation* op{nullptr};
  if (!spec_s_name.empty()) {
    op = pa_context_get_sink_info_by_name(m_context.get(), spec_s_name.c_str(), sink_info_callback, this);
    if (!op) {
      throw_error("pa_context_get_sink_info_by_name failed");
    }
    wait_loop(op);
  }
  if (s_name.empty()) {
    // get the sink index
    op = pa_context_get_sink_info_by_name(m_context.get(), DEFAULT_SINK, sink_info_callback, this);
    if (!op) {
      throw_error("pa_context_get_sink_info_by_name failed");
    }
    wait_loop(op);
    m_log.warn("pulseaudio: using default sink %s", s_name);
  } else {
    m_log.trace("pulseaudio: using sink %s", s_name);
  }

  auto event_types = static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER);
  op = pa_context_subscribe(m_context.get(), event_types, simple_callback, this);
  if (!op) {
    throw_error("pa_context_subscribe failed");
  }

  wait_loop(op);
  if (!success) {
    throw pulseaudio_error("Failed to subscribe to sink.");
  }
  pa_context_set_subscribe_callback(m_context.get(), subscribe_callback, this);

  // Make sure there is at least one event so that the volume is updated.
  m_events.emplace(evtype::NOP);
}

/**
 * Get sink name
 */
const string& pulseaudio::get_name() {
  mainloop_locker guard(m_mainloop);
  return s_name;
}

/**
 * Wait for events
 */
bool pulseaudio::wait() {
  mainloop_locker guard(m_mainloop);
  return !m_events.empty();
}

/**
 * Process queued pulseaudio events
 */
int pulseaudio::process_events() {
  int ret = m_events.size();
  mainloop_locker guard(m_mainloop);
  pa_operation* o{nullptr};
  // clear the queue
  while (!m_events.empty()) {
    auto ev = m_events.front();
    m_events.pop();
    switch (ev) {
      // try to get specified sink
      case evtype::NEW:
        // redundant if already using specified sink
        if (!spec_s_name.empty()) {
          o = pa_context_get_sink_info_by_name(m_context.get(), spec_s_name.c_str(), sink_info_callback, this);
          if (!o) {
            throw_error("pa_context_get_sink_info_by_name failed");
          }
          wait_loop(o);
          break;
        }
        // FALLTHRU
      case evtype::SERVER:
        // don't fallthrough only if always using default sink
        if (!spec_s_name.empty()) {
          break;
        }
        // FALLTHRU
      // get default sink
      case evtype::REMOVE:
        o = pa_context_get_sink_info_by_name(m_context.get(), DEFAULT_SINK, sink_info_callback, this);
        if (!o) {
          throw_error("pa_context_get_sink_info_by_name failed");
        }
        wait_loop(o);
        if (spec_s_name != s_name)
          m_log.warn("pulseaudio: using default sink %s", s_name);
        break;

      case evtype::RECONNECT:
        // We got disconnected from pulse
        m_log.warn("Reconnecting to PulseAudio");

        guard.unlock();
        connect();
        guard = mainloop_locker{m_mainloop};
        break;

      case evtype::NOP:
      default:
        break;
    }

    // We don't want exceptions in update_volume to crash the whole module
    try {
      update_volume();
    } catch (const pulseaudio_error& e) {
      m_log.err("pulseaudio: %s", e.what());
    }
  }

  return ret;
}

/**
 * Get volume in percentage
 */
int pulseaudio::get_volume() {
  mainloop_locker guard(m_mainloop);
  // alternatively, user pa_cvolume_avg_mask() to average selected channels
  return static_cast<int>(pa_cvolume_max(&cv) * 100.0f / PA_VOLUME_NORM + 0.5f);
}

/**
 * Set volume to given percentage
 */
void pulseaudio::set_volume(float percentage) {
  mainloop_locker guard(m_mainloop);
  pa_volume_t vol = math_util::percentage_to_value<pa_volume_t>(percentage, PA_VOLUME_MUTED, PA_VOLUME_NORM);
  pa_cvolume_scale(&cv, vol);
  pa_operation* op = pa_context_set_sink_volume_by_index(m_context.get(), m_index, &cv, simple_callback, this);
  if (!op) {
    throw_error("pa_context_set_sink_volume_by_index failed");
  }
  wait_loop(op);
  if (!success)
    throw pulseaudio_error("Failed to set sink volume.");
}

/**
 * Increment or decrement volume by given percentage (prevents accumulation of rounding errors from get_volume)
 */
void pulseaudio::inc_volume(int delta_perc) {
  mainloop_locker guard(m_mainloop);
  pa_volume_t vol = math_util::percentage_to_value<pa_volume_t>(abs(delta_perc), PA_VOLUME_NORM);
  if (delta_perc > 0) {
    pa_volume_t current = pa_cvolume_max(&cv);
    if (current + vol <= m_max_volume) {
      pa_cvolume_inc(&cv, vol);
    } else if (current < m_max_volume) {
      // avoid rounding errors and set to m_max_volume directly
      pa_cvolume_scale(&cv, m_max_volume);
    } else {
      m_log.warn("pulseaudio: maximum volume reached");
    }
  } else
    pa_cvolume_dec(&cv, vol);
  pa_operation* op = pa_context_set_sink_volume_by_index(m_context.get(), m_index, &cv, simple_callback, this);
  if (!op) {
    throw_error("pa_context_set_sink_volume_by_index failed");
  }
  wait_loop(op);
  if (!success)
    throw pulseaudio_error("Failed to set sink volume.");
}

/**
 * Set mute state
 */
void pulseaudio::set_mute(bool mode) {
  mainloop_locker guard(m_mainloop);
  pa_operation* op = pa_context_set_sink_mute_by_index(m_context.get(), m_index, mode, simple_callback, this);
  if (!op) {
    throw_error("pa_context_set_sink_mute_by_index failed");
  }
  wait_loop(op);
  if (!success)
    throw pulseaudio_error("Failed to mute sink.");
}

/**
 * Toggle mute state
 */
void pulseaudio::toggle_mute() {
  set_mute(!is_muted());
}

/**
 * Get current mute state
 */
bool pulseaudio::is_muted() {
  mainloop_locker guard(m_mainloop);
  return muted;
}

/**
 * Update local volume cache
 */
void pulseaudio::update_volume() {
  pa_operation* o = pa_context_get_sink_info_by_index(m_context.get(), m_index, get_sink_volume_callback, this);
  if (!o) {
    throw_error("pa_context_get_sink_info_by_index failed");
  }
  wait_loop(o);
}

void pulseaudio::throw_error(const string& msg) {
  throw pulseaudio_error(sstream() << msg << ": " << pa_strerror(pa_context_errno(m_context.get())));
}

/**
 * Callback when getting volume
 */
void pulseaudio::get_sink_volume_callback(pa_context*, const pa_sink_info* info, int, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  if (info) {
    This->cv = info->volume;
    This->muted = info->mute;
  }
  pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
}

/**
 * Callback when subscribing to changes
 */
void pulseaudio::subscribe_callback(pa_context*, pa_subscription_event_type_t t, uint32_t idx, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SERVER:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_CHANGE:
          This->m_events.emplace(evtype::SERVER);
          break;
      }
      break;
    case PA_SUBSCRIPTION_EVENT_SINK:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_NEW:
          This->m_events.emplace(evtype::NEW);
          break;
        case PA_SUBSCRIPTION_EVENT_CHANGE:
          if (idx == This->m_index)
            This->m_events.emplace(evtype::CHANGE);
          break;
        case PA_SUBSCRIPTION_EVENT_REMOVE:
          if (idx == This->m_index)
            This->m_events.emplace(evtype::REMOVE);
          break;
      }
      break;
  }
  pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
}

/**
 * Simple callback to check for success
 */
void pulseaudio::simple_callback(pa_context*, int success, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  This->success = success;
  pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
}

/**
 * Callback when getting sink info & existence
 */
void pulseaudio::sink_info_callback(pa_context*, const pa_sink_info* info, int eol, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  if (!eol && info) {
    This->m_index = info->index;
    This->s_name = info->name;
  }
  pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
}

/**
 * Callback when context state changes
 */
void pulseaudio::context_state_callback(pa_context* context, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
      This->m_state_callback_signal = true;
      pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
      break;
    case PA_CONTEXT_FAILED:
      This->m_state_callback_signal = true;
      pa_threaded_mainloop_signal(This->m_mainloop.get(), 0);
      This->m_events.emplace(evtype::RECONNECT);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;
  }
}

void pulseaudio::wait_loop(pa_operation* op) {
  while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait(m_mainloop.get());
  }
  pa_operation_unref(op);
}

pulseaudio::mainloop_locker::mainloop_locker(polybar::pulseaudio::mainloop_ptr& loop) noexcept : m_loop{loop.get()} {
  pa_threaded_mainloop_lock(m_loop);
}

pulseaudio::mainloop_locker::~mainloop_locker() noexcept {
  unlock();
}

void pulseaudio::mainloop_locker::unlock() noexcept {
  if (m_loop) {
    pa_threaded_mainloop_unlock(m_loop);
    m_loop = nullptr;
  }
}

void pulseaudio::context_deleter::operator()(pa_context* context) {
  if (context) {
    pa_context_disconnect(context);
    pa_context_unref(context);
  }
}

void pulseaudio::mainloop_deleter::operator()(pa_threaded_mainloop* loop) {
  if (loop) {
    pa_threaded_mainloop_stop(loop);
    pa_threaded_mainloop_free(loop);
  }
}

POLYBAR_NS_END
