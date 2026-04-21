#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_addressable_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_addressable_light_effect";

DDPAddressableLightEffect::DDPAddressableLightEffect(const char *name) : AddressableLightEffect(name) {}

const char *DDPAddressableLightEffect::get_name() { return AddressableLightEffect::get_name(); }

void DDPAddressableLightEffect::start() {
  // backup gamma for restoring when effect ends
  this->gamma_backup_ = this->state_->get_gamma_correct();
  this->next_packet_will_be_first_ = true;

  AddressableLightEffect::start();
  DDPLightEffectBase::start();

  // not automatically active just because enabled
  this->get_addressable_()->set_effect_active(false);
}

void DDPAddressableLightEffect::stop() {
  // restore backed up gamma value and recalculate gamma table.
  this->state_->set_gamma_correct(this->gamma_backup_);
  this->get_addressable_()->setup_state(this->state_);
  this->next_packet_will_be_first_ = true;

  DDPLightEffectBase::stop();
  AddressableLightEffect::stop();
}

void DDPAddressableLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
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

    // restore backed up gamma value and recalculate gamma table.
    this->state_->set_gamma_correct(this->gamma_backup_);
    it.setup_state(this->state_);

    // effect no longer active
    it.set_effect_active(false);

    call.perform();
  }
}

uint16_t DDPAddressableLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {
  // On the first packet of a new stream, optionally disable gamma correction.
  // This allows Home Assistant to control the light normally (with gamma) until a DDP stream starts.
  if (this->next_packet_will_be_first_ && this->disable_gamma_) {
    this->state_->set_gamma_correct(0.0f);
    this->get_addressable_()->setup_state(this->state_);
  }
  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();
  auto *it = this->get_addressable_();
  it->set_effect_active(true);  // Mark effect active now that we have a packet.
#ifdef USE_ESP32
  const uint16_t leds_in_segment = it->size();
  const uint16_t bytes_in_payload = size - used;
  const uint16_t pixels_to_process = std::min((int) leds_in_segment, (int) (bytes_in_payload / 3));
#else
  const uint16_t leds_in_segment = it->size();
  const uint16_t bytes_in_payload = size - used;
  const uint16_t pixels_to_process = min(leds_in_segment, (uint16_t)(bytes_in_payload / 3));
#endif
  const uint16_t bytes_to_process = pixels_to_process * 3;
  if (pixels_to_process < 1) {
    return 0;
  }
  ESP_LOGV(TAG, "Applying DDP data for '%s' (size: %d - used: %d - pixels: %d)", this->get_name(), size, used,
           pixels_to_process);
  // Multiplier is a fixed-point 8.8 number (256 = 1.0).
  uint16_t multiplier = 256;
  // The scaling mode determines how the DDP RGB values are adjusted based on the light's brightness setting.
  switch (this->scaling_mode_) {
    case DDP_SCALE_PACKET:
      // Scale all pixels in this segment by the brightest color component in the entire DDP packet.
      // This ensures consistent brightness across all segments of a logical display.
      multiplier = this->calculate_multiplier_(payload, 10, size);
      this->set_max_brightness_();
      break;
    case DDP_SCALE_STRIP:
      // Scale all pixels in this segment by the brightest color component within this segment.
      multiplier = this->calculate_multiplier_(payload, used, used + bytes_to_process);
      this->set_max_brightness_();
      break;
    case DDP_NO_SCALING:
    case DDP_SCALE_PIXEL:
      // DDP_NO_SCALING: Use raw RGB values from the packet, ignoring the light's brightness setting.
      // DDP_SCALE_PIXEL: Brightness is calculated per-pixel inside the loop.
      // For both modes, we set the component brightness to 100% to ensure DDP values are not scaled down by ESPHome.
      this->set_max_brightness_();
      break;
    case DDP_SCALE_MULTIPLY:
      // Default ESPHome behavior: RGB values are multiplied by the light's brightness setting.
      // No multiplier is needed here, and brightness is not forced to 100%.
      break;
  }
  for (uint16_t i = 0; i < pixels_to_process; i++) {
    uint16_t payload_index = used + (i * 3);
    uint8_t red = payload[payload_index];
    uint8_t green = payload[payload_index + 1];
    uint8_t blue = payload[payload_index + 2];
    if (this->scaling_mode_ == DDP_SCALE_PIXEL) {
// Scale this pixel by its own brightest color component.
#ifdef USE_ESP32
      uint8_t max_val = std::max({red, green, blue});
#else
      uint8_t max_val = max(red, max(green, blue));
#endif
      multiplier = this->get_fixed_point_multiplier_(max_val);
    }
    // Apply scaling if required for the current mode.
    if (this->scaling_mode_ == DDP_SCALE_PACKET || this->scaling_mode_ == DDP_SCALE_STRIP ||
        this->scaling_mode_ == DDP_SCALE_PIXEL) {
      if (multiplier != 256) {
        red = (uint32_t(red) * multiplier) >> 8;
        green = (uint32_t(green) * multiplier) >> 8;
        blue = (uint32_t(blue) * multiplier) >> 8;
      }
    }
    (*it)[i].set_rgb(red, green, blue);
  }
  it->schedule_show();
  return bytes_to_process;
}

uint16_t DDPAddressableLightEffect::calculate_multiplier_(const uint8_t *payload, uint16_t start, uint16_t end) {
  uint8_t max_val = 0;
  // look for highest value in packet or strip
  for (uint16_t i = start; i < end; i++) {
    if (payload[i] > max_val) {
      max_val = payload[i];
    }
  }
  return this->get_fixed_point_multiplier_(max_val);
}

uint16_t DDPAddressableLightEffect::get_fixed_point_multiplier_(uint8_t max_val) {
  if (max_val == 0) {
    return 256;  // Fixed point representation of 1.0
  } else {
    // remote_values.get_brightness() is a float from 0.0 to 1.0
    // We want to calculate: (brightness * 255) / max_val
    // To use integer math, we scale brightness to 0-255
    // Then to create a fixed-point 8.8 number, we multiply by 256
    uint32_t brightness_int = this->state_->remote_values.get_brightness() * 255.0f;
    return (brightness_int * 256) / max_val;
  }
}

void DDPAddressableLightEffect::set_max_brightness_() {
  this->state_->current_values.set_brightness(1.0f);
  this->get_addressable_()->update_state(this->state_);
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
