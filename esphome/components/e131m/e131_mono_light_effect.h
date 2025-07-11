#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_state.h"
#ifdef USE_NETWORK
namespace esphome {
namespace e131m {

class E131Component;
struct E131Packet;

enum E131MLightChannels { E131_MONO = 1, E131_RGB = 3 };

class E131MonoLightEffect : public light::LightEffect {
 public:
  E131MonoLightEffect(const std::string &name);

  void start() override;
  void stop() override;
  void apply() override;

  int get_data_per_universe() const;
  int get_lights_per_universe() const;
  int get_first_universe() const;
  int get_last_universe() const;
  int get_universe_count() const;

  void set_first_universe(int universe) { this->first_universe_ = universe; }
  void set_channels(E131MLightChannels channels) { this->channels_ = channels; }
  void set_e131(E131Component *e131) { this->e131_ = e131; }

 protected:
  bool process_(int universe, const E131Packet &packet);

  int first_universe_{0};
  E131MLightChannels channels_{E131_RGB};
  E131Component *e131_{nullptr};

  friend class E131Component;
};

}  // namespace e131m
}  // namespace esphome
#endif

