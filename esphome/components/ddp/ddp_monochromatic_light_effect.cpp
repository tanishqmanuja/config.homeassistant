#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_monochromatic_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_monochromatic_light_effect";

namespace {
  constexpr float kInv255 = 1.0f / 255.0f;
}

DDPMonochromaticLightEffect::DDPMonochromaticLightEffect(const std::string &name) : LightEffect(name) {}

const std::string &DDPMonochromaticLightEffect::get_name() { return LightEffect::get_name(); }

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
  if ( this->timeout_check() ) {
    ESP_LOGD(TAG,"DDP stream for '%s->%s' timed out.", this->state_->get_name().c_str(), this->get_name().c_str());
    this->next_packet_will_be_first_ = true;

    auto call = this->state_->turn_on();

    call.set_brightness_if_supported(this->state_->remote_values.get_brightness());

    call.set_publish(false);
    call.set_save(false);

    // restore backed up gamma value
    this->state_->set_gamma_correct(this->gamma_backup_);
    call.perform();
   }

}

uint16_t DDPMonochromaticLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {
  // at least for now, we require 1 bytes of data (r, g, b).
  if ( size < (used + 3) ) { return 0; }

  // disable gamma on first received packet, not just based on effect being enabled.
  // that way home assistant light can still be used as normal when DDP packets are not
  // being received but effect is still enabled.
  // gamma will be enabled again when effect disabled or on timeout.
  if ( this->next_packet_will_be_first_ && this->disable_gamma_ ) {
    this->state_->set_gamma_correct(0.0f);
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();


  float red   = (float)payload[used] * kInv255;
  float green = (float)payload[used + 1] * kInv255;
  float blue  = (float)payload[used + 2] * kInv255;

  float multiplier = this->state_->remote_values.get_brightness();
  float max_val = 0.0f;

  if (this->scaling_mode_ == DDP_SCALE_PIXEL) {
    max_val = std::max({ red, green, blue });
  }

  if (max_val != 0.0f) {
    multiplier /= max_val;
  }

  if (this->scaling_mode_ != DDP_NO_SCALING) {
    red *= multiplier;
    green *= multiplier;
    blue *= multiplier;
  }

  float brightness = std::max({ red, green, blue });

  auto call = this->state_->turn_on();
  call.set_brightness_if_supported(brightness);
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
