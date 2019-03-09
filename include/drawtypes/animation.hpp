#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

namespace drawtypes {
  namespace details {
    template <typename>
    class base_animation_manager;
  }

  class animation : public non_copyable_mixin<animation> {
   public:
    explicit animation(unsigned int framerate_ms) : m_framerate_ms(framerate_ms) {}
    explicit animation(vector<icon_t>&& frames, unsigned int framerate_ms)
        : m_frames(move(frames))
        , m_framerate_ms(framerate_ms)
        , m_framecount(m_frames.size())
        , m_frame(m_frames.size() - 1) {}

    void add(icon_t&& frame);
    void increment();

    label_t get() const;
    unsigned int framerate() const;

    explicit operator bool() const;

   protected:
    vector<label_t> m_frames;

    unsigned int m_framerate_ms = 1000;
    size_t m_framecount = 0;
    std::atomic_size_t m_frame{0_z};
  };

  using animation_t = shared_ptr<animation>;

  animation_t load_animation(
      const config& conf, const string& section, string name = "animation", bool required = true);

  namespace details {
    template <typename Manager>
    class base_animation_manager {
     public:
      base_animation_manager() = default;
      ~base_animation_manager() {
        m_is_running = false;
        m_thread.join();
      }

     protected:
      template <typename SelectionCallback, typename PostUpdateCallback>
      void launch(SelectionCallback&& sel_callback, PostUpdateCallback&& post_callback) {
        launch_impl([this, sel = forward<SelectionCallback>(sel_callback)]() { return sel(me().get_animations()); },
            std::forward<PostUpdateCallback>(post_callback));
      }

      template <typename PostUpdateCallback>
      void launch(PostUpdateCallback&& post_callback) {
        launch_impl([this]() { return me().get_animation(); }, std::forward<PostUpdateCallback>(post_callback));
      }

     private:
      template <typename SelectionCallback, typename PostUpdateCallback>
      void launch_impl(SelectionCallback&& sel_callback, PostUpdateCallback&& post_callback) {
        m_thread = std::thread(
            [this](auto&& sel_callback, auto&& post_callback) {
              while (m_is_running) {
                auto now = chrono::system_clock::now();
                animation_t animation = sel_callback();

                if (animation) {
                  animation->increment();
                  now += chrono::milliseconds(animation->framerate());
                  post_callback();
                } else {
                  now += chrono::milliseconds(me().m_default_sleep);
                }

                std::this_thread::sleep_until(now);
              }
            },
            std::forward<SelectionCallback>(sel_callback), std::forward<PostUpdateCallback>(post_callback));
      }

      void stop() {
        m_is_running = false;
      }

     private:
      std::thread m_thread;
      std::atomic_bool m_is_running{true};

      Manager& me() {
        return *static_cast<Manager*>(this);
      }

      const Manager& me() const {
        return *static_cast<const Manager* const>(this);
      }
    };
  }  // namespace details

  class animation_manager : details::base_animation_manager<animation_manager> {
   public:
    explicit animation_manager(unsigned int default_framerate_ms, animation_t&& animation)
        : m_default_sleep{default_framerate_ms}, m_animation{animation} {}

    template <typename PostUpdateCallback>
    void launch(PostUpdateCallback&& post_callback) {
      details::base_animation_manager<animation_manager>::launch(forward<PostUpdateCallback>(post_callback));
    }

    const animation_t& get_animation() const {
      return m_animation;
    }

   private:
    friend details::base_animation_manager<animation_manager>;

    unsigned int m_default_sleep;
    animation_t m_animation;
  };

  class multi_animation_manager : details::base_animation_manager<multi_animation_manager> {
   public:
    explicit multi_animation_manager(unsigned int default_framerate_ms, vector<animation_t>&& animations)
        : details::base_animation_manager<multi_animation_manager>()
        , m_default_sleep(default_framerate_ms)
        , m_animations(move(animations)) {}

    template <typename SelectionCallback, typename PostUpdateCallback>
    void launch(SelectionCallback&& animation_selector, PostUpdateCallback&& post_callback) {
      details::base_animation_manager<multi_animation_manager>::launch(
          forward<SelectionCallback>(animation_selector), forward<PostUpdateCallback>(post_callback));
    }

    const vector<animation_t>& get_animations() const {
      return m_animations;
    }

   private:
    friend details::base_animation_manager<multi_animation_manager>;

    unsigned int m_default_sleep;
    vector<animation_t> m_animations;
  };

  using animation_manager_t = unique_ptr<animation_manager>;
  using multi_animation_manager_t = unique_ptr<multi_animation_manager>;

  template <typename... Args>
  decltype(auto) make_multi_animation_manager(Args&&... args) {
    return factory_util::unique<multi_animation_manager>(forward<Args>(args)...);
  }

  template <typename... Args>
  decltype(auto) make_animation_manager(Args&&... args) {
    return factory_util::unique<animation_manager>(forward<Args>(args)...);
  }
}  // namespace drawtypes

POLYBAR_NS_END
