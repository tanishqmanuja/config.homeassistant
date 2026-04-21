#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static constexpr int DDP_PORT = 4048;
static constexpr int DDP_HEADER_SIZE = 10;
static constexpr int DDP_MIN_PACKET_SIZE = 13;  // Header (10) + 1 pixel (3)
static constexpr int DDP_HEADER_OFFSET_DATA_OFFSET = 4;
static constexpr int DDP_MAX_PAYLOAD_SIZE = 1500;

DDPComponent::DDPComponent() {}
DDPComponent::~DDPComponent() {}
void DDPComponent::setup() {}

void DDPComponent::loop() {
  if (!this->udp_ || this->payload_buffer_.empty()) {
    return;
  }

  while (uint16_t packet_size = this->udp_->parsePacket()) {
    if (packet_size > this->payload_buffer_.size()) {
      ESP_LOGW(TAG, "Received DDP packet too large (%u bytes), skipping", packet_size);
      // Either skip or resize the buffer here if you want to support jumbo frames
      continue;
    }

    if (this->udp_->read(this->payload_buffer_.data(), packet_size) != packet_size) {
      ESP_LOGW(TAG, "Failed to read complete DDP packet");
      continue;
    }

    if (!this->process_(this->payload_buffer_.data(), packet_size)) {
      ESP_LOGW(TAG, "Failed to process DDP packet");
      continue;
    }
  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {
  if (std::find(this->light_effects_.begin(), this->light_effects_.end(), light_effect) != this->light_effects_.end()) {
    return;
  }

  // only the first effect added needs to start udp listening
  // but we still need to add the effect to the vector so it can be applied.
  if (this->light_effects_.empty()) {
    if (!this->udp_) {
      this->udp_ = make_unique<WiFiUDP>();
    }

    ESP_LOGD(TAG, "Starting UDP listening for DDP.");
    if (!this->udp_->begin(DDP_PORT)) {
      ESP_LOGE(TAG, "Cannot bind DDP to port %d.", DDP_PORT);
      mark_failed();
      return;
    }

    this->payload_buffer_.resize(DDP_MAX_PAYLOAD_SIZE);
  }

  this->light_effects_.push_back(light_effect);
}

void DDPComponent::remove_effect(DDPLightEffectBase *light_effect) {
  auto it = std::find(this->light_effects_.begin(), this->light_effects_.end(), light_effect);
  if (it == this->light_effects_.end()) {
    return;
  }

  this->light_effects_.erase(it);

  // if no more effects left, stop udp listening
  if (this->light_effects_.empty() && this->udp_) {
    ESP_LOGD(TAG, "Stopping UDP listening for DDP.");
    this->udp_->stop();
    std::vector<uint8_t>().swap(this->payload_buffer_);
  }
}

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {
  // size under 10 means we don't even receive a valid header.
  // size under 13 means we don't have enough for even 1 pixel.
  if (size < DDP_MIN_PACKET_SIZE) {
    ESP_LOGE(TAG, "Invalid DDP packet received, too short (size=%d)", size);
    return false;
  }

  // ignore packet if data offset != [00 00 00 00].  This likely means the device is receiving a DDP packet not meant
  // for it. There may be a better way to handle this header field.  One user was receiving packets with non-zero data
  // offset that were screwing up the light effect (flickering).
  if (payload[DDP_HEADER_OFFSET_DATA_OFFSET] || payload[DDP_HEADER_OFFSET_DATA_OFFSET + 1] ||
      payload[DDP_HEADER_OFFSET_DATA_OFFSET + 2] || payload[DDP_HEADER_OFFSET_DATA_OFFSET + 3]) {
    ESP_LOGE(TAG, "Ignoring DDP Packet with non-zero data offset.");
    return false;
  }

  ESP_LOGV(TAG, "DDP packet received (size=%d): - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x [%02x %02x %02x]",
           size, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7],
           payload[8], payload[9], payload[10], payload[11], payload[12]);

  // first 10 bytes are the header, so consider them used from the get-go
  // if timecode field is used, takes up an additional 4 bytes of header.
  // this component does not handle the timecode field.  If there is a situation
  // where the timecode field is included and cannot be removed, this may need
  // modified to handle the timecode field.  So far, neither WLED nor xLights
  // follow the header spec in general, and neither sends a timecode field.
  uint16_t used = DDP_HEADER_SIZE;

  // run through all registered effects, each takes required data per their size starting at packet address determined
  // by used.
  for (auto *light_effect : this->light_effects_) {
    if (used >= size) {
      return false;
    }
    uint16_t new_used = light_effect->process_(&payload[0], size, used);
    if (new_used == 0) {
      return false;
    } else {
      used += new_used;
    }
  }

  return true;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
