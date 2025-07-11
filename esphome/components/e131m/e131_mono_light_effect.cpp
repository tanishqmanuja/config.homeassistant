#include "e131_mono_light_effect.h"
#include "e131.h"
#ifdef USE_NETWORK
#include "esphome/core/log.h"

namespace esphome {
namespace e131m {

static const char *const TAG = "e131m_light_effect";
static const int MAX_DATA_SIZE = (sizeof(E131Packet::values) - 1);

E131MonoLightEffect::E131MonoLightEffect(const std::string &name) : LightEffect(name) {}

int E131MonoLightEffect::get_data_per_universe() const { return get_lights_per_universe() * channels_; }

int E131MonoLightEffect::get_lights_per_universe() const { return MAX_DATA_SIZE / channels_; }

int E131MonoLightEffect::get_first_universe() const { return first_universe_; }

int E131MonoLightEffect::get_last_universe() const { return first_universe_ + get_universe_count() - 1; }

int E131MonoLightEffect::get_universe_count() const { return 1; }

void E131MonoLightEffect::start() {
  LightEffect::start();

  if (this->e131_) {
    this->e131_->add_effect(this);
  }
}

void E131MonoLightEffect::stop() {
  if (this->e131_) {
    this->e131_->remove_effect(this);
  }

  LightEffect::stop();
}

void E131MonoLightEffect::apply() {
  // ignore, it is run by `E131Component::update()`
}

bool E131MonoLightEffect::process_(int universe, const E131Packet &packet) {
  if (packet.count < channels_) {
    ESP_LOGW(TAG, "Packet too small: expected at least %d bytes, got %d", channels_, packet.count);
    return false;
  }
    
  ESP_LOGV(TAG, "Applying data for '%s' on %d universe.", get_name().c_str(), universe);
    
  const uint8_t *data = packet.values;
  auto call = this->state_->turn_on();

  float brightness = 0.0f;

  if (channels_ == E131_MONO) {
    brightness = data[0] / 255.0f;
  } else if (channels_ == E131_RGB) {
    brightness = std::max({ data[0], data[1], data[2] }) / 255.0f;
  }

  call.set_brightness(brightness);

  // don't tell HA every change
  call.set_transition_length(0);
  call.set_publish(false);
  call.set_save(false);
  call.perform();

  return true;
}

}  // namespace e131m
}  // namespace esphome
#endif

