#include "e131.h"
#ifdef USE_NETWORK
#include "e131_mono_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace e131m {

static const char *const TAG = "e131m";

E131Component::E131Component() {}

E131Component::~E131Component() {
  this->async_e131_.end();
}

void E131Component::setup() {}

void E131Component::loop() {
  static e131_packet_t p;
  static E131Packet packet;
  int packets_processed = 0;
  while (!this->async_e131_.isEmpty() && packets_processed++ < 5) {
    this->async_e131_.pull(&p);
    
    const int universe = htons(p.universe);
    const int property_count = htons(p.property_value_count) - 1;
    
    packet.count = (property_count > E131_MAX_PROPERTY_VALUES_COUNT)
    ? E131_MAX_PROPERTY_VALUES_COUNT
    : property_count;

    packet.values = p.property_values + 1;
    
    if (!this->process_(universe, packet)) {
      ESP_LOGV(TAG, "Ignored packet for universe %d of size %d.", universe, packet.count);
    }
  }
}
  
void E131Component::add_effect(E131MonoLightEffect *light_effect) {
    if (light_effects_.count(light_effect)) {
      return;
    }
    
    ESP_LOGI(TAG, "Starting UDP listening.");
    if (!this->async_e131_.begin(static_cast<e131_listen_t>(this->listen_method_),light_effect->get_first_universe(), light_effect->get_universe_count())) {
      ESP_LOGE(TAG, "Unable to start UDP listening.");
    mark_failed();
    return;
  }

  ESP_LOGD(TAG, "Registering '%s' for universes %d-%d.", light_effect->get_name().c_str(),
           light_effect->get_first_universe(), light_effect->get_last_universe());

  light_effects_.insert(light_effect);
}

void E131Component::remove_effect(E131MonoLightEffect *light_effect) {
  if (!light_effects_.count(light_effect)) {
    return;
  }

  ESP_LOGD(TAG, "Unregistering '%s' for universes %d-%d.", light_effect->get_name().c_str(),
           light_effect->get_first_universe(), light_effect->get_last_universe());

  light_effects_.erase(light_effect);

  if (this->light_effects_.empty()) {
    ESP_LOGI(TAG, "Stopping UDP listening.");
    this->async_e131_.end();
  }
}

bool E131Component::process_(int universe, const E131Packet &packet) {
  bool handled = false;

  ESP_LOGV(TAG, "Received E1.31 packet for %d universe, with %d bytes", universe, packet.count);

  for (auto *light_effect : light_effects_) {
    handled |= light_effect->process_(universe, packet);
  }

  return handled;
}

}  // namespace e131m
}  // namespace esphome
#endif

