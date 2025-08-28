#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "e131_base_light_effect.h"
#ifdef USE_NETWORK
namespace esphome {
namespace e131 {

class E131Component;
struct E131Packet;

class E131LightEffect : public E131BaseLightEffect, public light::LightEffect {
 public:
  E131LightEffect(const std::string &name);

  const std::string &get_name() override;

  void start() override;
  void stop() override;
  void apply() override;

  int get_data_per_universe() const override;
  int get_lights_per_universe() const override;
  int get_first_universe() const override;
  int get_last_universe() const override;
  int get_universe_count() const override;

 protected:
  bool process_(int universe, const E131Packet &packet) override;

  friend class E131Component;
};

}  // namespace e131
}  // namespace esphome
#endif
