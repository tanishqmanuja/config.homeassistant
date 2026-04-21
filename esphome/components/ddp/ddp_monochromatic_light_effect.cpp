#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_monochromatic_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_monochromatic_light_effect";

DDPMonochromaticLightEffect::DDPMonochromaticLightEffect(const char *name) : LightEffect(name) {}

const char *DDPMonochromaticLightEffect::get_name() { return LightEffect::get_name(); }

void DDPMonochromaticLightEffect::start() {
  // backup gamma for restoring when effect ends
  this->gamma_backup_ = this->state_->get_gamma_correct();
  this->next_packet_will_be_first_ = true;

  LightEffect::start();
  DDPLightEffectBase::start();
}

void DDPMonochromaticLightEffect::stop() {
  // restore gamma.
  this->state_->set_gamma_correct(this->gamma_backup_);
  this->next_packet_will_be_first_ = true;

  DDPLightEffectBase::stop();
  LightEffect::stop();
}

void DDPMonochromaticLightEffect::apply() {
  // if receiving DDP packets times out, reset to home assistant color.
  // apply function is not needed normally to display changes to the light
  // from Home Assistant, but it is needed to restore value on timeout.
  if (this->timeout_check()) {
    ESP_LOGD(TAG, "DDP stream for '%s->%s' timed out.", this->state_->get_name(), this->get_name());
    this->next_packet_will_be_first_ = true;

    auto call = this->state_->turn_on();

    if (this->blank_on_idle_) {
      call.set_brightness_if_supported(0.0f);
    } else {
      call.set_brightness_if_supported(this->state_->remote_values.get_brightness());
    }

    call.set_publish(false);
    call.set_save(false);

    // restore backed up gamma value
    this->state_->set_gamma_correct(this->gamma_backup_);
    call.perform();
  }
}

uint16_t DDPMonochromaticLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {
  // we require 3 bytes of data (r, g, b) to determine brightness.
  if (size < (used + 3)) {
    return 0;
  }

  // disable gamma on first received packet, not just based on effect being enabled.
  // that way home assistant light can still be used as normal when DDP packets are not
  // being received but effect is still enabled.
  // gamma will be enabled again when effect disabled or on timeout.
  if (this->next_packet_will_be_first_ && this->disable_gamma_) {
    this->state_->set_gamma_correct(0.0f);
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();

  uint8_t red = payload[used];
  uint8_t green = payload[used + 1];
  uint8_t blue = payload[used + 2];

  uint8_t ddp_brightness = std::max({red, green, blue});
  uint8_t final_brightness_int = ddp_brightness;

  if (this->scaling_mode_ == DDP_SCALE_PIXEL) {
    if (ddp_brightness > 0) {
      uint32_t ha_brightness_int = this->state_->remote_values.get_brightness() * 255;
      final_brightness_int = (uint32_t) ddp_brightness * ha_brightness_int / ddp_brightness;
    }
  } else if (this->scaling_mode_ == DDP_SCALE_MULTIPLY) {
    final_brightness_int = (uint32_t) ddp_brightness * this->state_->remote_values.get_brightness();
  }
  // In NO_SCALING mode, we just use the raw ddp_brightness value.

  float brightness_float = (float) final_brightness_int / 255.0f;
  if (this->scaling_mode_ == DDP_NO_SCALING) {
    brightness_float = (float) ddp_brightness / 255.0f;
  }

  auto call = this->state_->turn_on();
  call.set_brightness_if_supported(brightness_float);
  call.set_transition_length_if_supported(0);
  call.set_publish(false);
  call.set_save(false);

  call.perform();

  // manually calling loop otherwise we just go straight into processing the next DDP
  // packet without executing the light loop to display the just-processed packet.
  // Not totally sure why or if there is a better way to fix, but this works.
  this->state_->loop();

  return 3;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
