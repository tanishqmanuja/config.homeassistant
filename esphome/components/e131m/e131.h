#pragma once
#include "esphome/core/defines.h"
#ifdef USE_NETWORK
#include "esphome/components/socket/socket.h"
#include "esphome/core/component.h"

#include <ESPAsyncE131.h>

#include <cinttypes>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace esphome {
namespace e131m {

class E131MonoLightEffect;

enum E131ListenMethod {
  E131_UNICAST = ::E131_UNICAST,
  E131_MULTICAST = ::E131_MULTICAST,
};

const int E131_MAX_PROPERTY_VALUES_COUNT = 513;

struct E131Packet {
  uint16_t count;
  const uint8_t *values;
};

class E131Component : public esphome::Component {
 public:
  E131Component();
  ~E131Component();

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void add_effect(E131MonoLightEffect *light_effect);
  void remove_effect(E131MonoLightEffect *light_effect);

  void set_method(E131ListenMethod listen_method) { this->listen_method_ = listen_method; }

 protected:
  bool process_(int universe, const E131Packet &packet);

  E131ListenMethod listen_method_{E131_MULTICAST};
  ESPAsyncE131 async_e131_;
  std::set<E131MonoLightEffect *> light_effects_;
};

}  // namespace e131
}  // namespace esphome
#endif

