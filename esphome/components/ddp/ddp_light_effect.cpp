#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_light_effect";

DDPLightEffect::DDPLightEffect(const char *name) : LightEffect(name) {}

const char *DDPLightEffect::get_name() { return LightEffect::get_name(); }

void DDPLightEffect::start() {
  // backup gamma for restoring when effect ends
  this->gamma_backup_ = this->state_->get_gamma_correct();
  this->next_packet_will_be_first_ = true;

  LightEffect::start();
  DDPLightEffectBase::start();
}

void DDPLightEffect::stop() {
  // restore gamma.
  this->state_->set_gamma_correct(this->gamma_backup_);
  this->next_packet_will_be_first_ = true;

  DDPLightEffectBase::stop();
  LightEffect::stop();
}

void DDPLightEffect::apply() {
  // if receiving DDP packets times out, reset to home assistant color.
  // apply function is not needed normally to display changes to the light
  // from Home Assistant, but it is needed to restore value on timeout.
  if (this->timeout_check()) {
    ESP_LOGD(TAG, "DDP stream for '%s->%s' timed out.", this->state_->get_name(), this->get_name());
    this->next_packet_will_be_first_ = true;

    auto call = this->state_->turn_on();

    call.set_color_mode_if_supported(this->state_->remote_values.get_color_mode());
    call.set_red_if_supported(this->state_->remote_values.get_red());
    call.set_green_if_supported(this->state_->remote_values.get_green());
    call.set_blue_if_supported(this->state_->remote_values.get_blue());
    call.set_brightness_if_supported(this->state_->remote_values.get_brightness());
    call.set_color_brightness_if_supported(this->state_->remote_values.get_color_brightness());

    call.set_white_if_supported(this->state_->remote_values.get_white());
    call.set_cold_white_if_supported(this->state_->remote_values.get_cold_white());
    call.set_warm_white_if_supported(this->state_->remote_values.get_warm_white());

    call.set_publish(false);
    call.set_save(false);

    // restore backed up gamma value
    this->state_->set_gamma_correct(this->gamma_backup_);
    call.perform();
  }
}

uint16_t DDPLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {
  // we require 3 bytes of data (r, g, b).
  if (size < (used + 3)) {
    return 0;
  }

  // disable gamma on first received packet, not just based on effect being enabled.
  if (this->next_packet_will_be_first_ && this->disable_gamma_) {
    this->state_->set_gamma_correct(0.0f);
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();

  ESP_LOGV(TAG, "Applying DDP data for '%s->%s': (%02x,%02x,%02x) size = %d, used = %d",
           this->state_->get_name().c_str(), this->get_name(), payload[used], payload[used + 1], payload[used + 2], size,
           used);

  uint8_t red = payload[used];
  uint8_t green = payload[used + 1];
  uint8_t blue = payload[used + 2];

  // In scaling modes, we adjust the incoming RGB values based on the light's brightness in Home Assistant.
  if (this->scaling_mode_ != DDP_NO_SCALING && this->scaling_mode_ != DDP_SCALE_MULTIPLY) {
    uint8_t max_val = 0;
    if (this->scaling_mode_ == DDP_SCALE_PACKET) {
      // Find the brightest color component in the entire packet
      for (int i = 10; i < size; i++) {
        if (payload[i] > max_val) {
          max_val = payload[i];
        }
      }
    } else {  // For SCALE_STRIP and SCALE_PIXEL, it's just this one pixel.
      max_val = std::max({red, green, blue});
    }

    // Scale the RGB values so that the brightest component matches the HA brightness.
    if (max_val > 0) {
      uint32_t ha_brightness_int = this->state_->remote_values.get_brightness() * 255;
      red = (uint32_t) red * ha_brightness_int / max_val;
      green = (uint32_t) green * ha_brightness_int / max_val;
      blue = (uint32_t) blue * ha_brightness_int / max_val;
    }
  } else if (this->scaling_mode_ == DDP_SCALE_MULTIPLY) {
    // Simply multiply by the HA brightness value.
    float ha_brightness = this->state_->remote_values.get_brightness();
    red = (uint32_t)red * ha_brightness;
    green = (uint32_t)green * ha_brightness;
    blue = (uint32_t)blue * ha_brightness;
  }
  // In NO_SCALING mode, we use the raw RGB values.

  auto call = this->state_->turn_on();
  float final_red = (float) red / 255.0f;
  float final_green = (float) green / 255.0f;
  float final_blue = (float) blue / 255.0f;

  call.set_color_mode_if_supported(light::ColorMode::RGB_COLD_WARM_WHITE);
  call.set_color_mode_if_supported(light::ColorMode::RGB_COLOR_TEMPERATURE);
  call.set_color_mode_if_supported(light::ColorMode::RGB_WHITE);
  call.set_color_mode_if_supported(light::ColorMode::RGB);
  call.set_red_if_supported(final_red);
  call.set_green_if_supported(final_green);
  call.set_blue_if_supported(final_blue);
  call.set_brightness_if_supported(std::max({final_red, final_green, final_blue}));
  call.set_color_brightness_if_supported(1.0f);

  // disable white channels
  call.set_white_if_supported(0.0f);
  call.set_cold_white_if_supported(0.0f);
  call.set_warm_white_if_supported(0.0f);

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
