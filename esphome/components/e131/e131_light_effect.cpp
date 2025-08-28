#include "e131.h"
#include "e131_light_effect.h"
#ifdef USE_NETWORK
#include "esphome/core/log.h"

namespace esphome {
namespace e131 {

static const char *const TAG = "e131_light_effect";
static const int MAX_DATA_SIZE = (sizeof(E131Packet::values) - 1);

E131LightEffect::E131LightEffect(const std::string &name) : LightEffect(name) {}

const std::string &E131LightEffect::get_name() { return LightEffect::get_name(); }

int E131LightEffect::get_data_per_universe() const { return get_lights_per_universe() * channels_; }

int E131LightEffect::get_lights_per_universe() const { return MAX_DATA_SIZE / channels_; }

int E131LightEffect::get_first_universe() const { return first_universe_; }

int E131LightEffect::get_last_universe() const { return first_universe_ + get_universe_count() - 1; }

int E131LightEffect::get_universe_count() const { return 1; }

void E131LightEffect::start() {
  LightEffect::start();
  E131BaseLightEffect::start();

  if (this->e131_) {
    this->e131_->add_effect(this);
  }
}

void E131LightEffect::stop() {
  if (this->e131_) {
    this->e131_->remove_effect(this);
  }

  E131BaseLightEffect::stop();
  LightEffect::stop();
}

void E131LightEffect::apply() {
  // ignore, it is run by `E131Component::update()`
}

bool E131LightEffect::process_(int universe, const E131Packet &packet) {
  // check if this is our universe and data are valid
  if (universe < first_universe_ || universe > get_last_universe())
    return false;

  auto *input_data = packet.values + 1;

  ESP_LOGV(TAG, "Applying data for '%s' on %d universe.", get_name().c_str(), universe);

  auto call = this->state_->turn_on();

  uint8_t raw_mono = input_data[0];
  uint8_t raw_red = input_data[0];
  uint8_t raw_green = channels_>= E131_RGB ? input_data[1] : 0;
  uint8_t raw_blue = channels_ >= E131_RGB ? input_data[2] : 0;
  uint8_t raw_white = channels_ >= E131_RGBW ? input_data[3] : 0;

  float mono = (float)raw_mono / 255.0f;

  switch (channels_) {
    case E131_MONO:
      call.set_brightness_if_supported(mono);
      break;

    case E131_RGB: {
      float red = (float)raw_red / 255.0f;
      float green = (float)raw_green / 255.0f;
      float blue = (float)raw_blue / 255.0f;
      mono = std::max({red, green, blue}) / 255.0f;
      call.set_red_if_supported(red);
      call.set_green_if_supported(green);
      call.set_blue_if_supported(blue);
      call.set_brightness_if_supported(mono);
      break;
    }

    case E131_RGBW: {
      // Shouldn't happen anyways
      call.set_brightness_if_supported(mono);
      break;
    }
  }

  // don't tell HA every change
  call.set_transition_length(0);
  call.set_publish(false);
  call.set_save(false);
  call.perform();

  return true;
}

}  // namespace e131
}  // namespace esphome
#endif
