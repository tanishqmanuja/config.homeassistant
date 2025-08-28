#include "e131.h"
#include "e131_base_light_effect.h"
#ifdef USE_NETWORK
#include "esphome/core/log.h"

namespace esphome {
namespace e131 {

static const int MAX_DATA_SIZE = (sizeof(E131Packet::values) - 1);

E131BaseLightEffect::E131BaseLightEffect(){}

int E131BaseLightEffect::get_data_per_universe() const { return get_lights_per_universe() * channels_; }

int E131BaseLightEffect::get_lights_per_universe() const { return MAX_DATA_SIZE / channels_; }

int E131BaseLightEffect::get_first_universe() const { return first_universe_; }

int E131BaseLightEffect::get_last_universe() const { return first_universe_ + get_universe_count() - 1; }

int E131BaseLightEffect::get_universe_count() const { return 1; }

void E131BaseLightEffect::start() {
  if (this->e131_) {
    this->e131_->add_effect(this);
  }
}

void E131BaseLightEffect::stop() {
  if (this->e131_) {
    this->e131_->remove_effect(this);
  }
}

}  // namespace e131
}  // namespace esphome
#endif

