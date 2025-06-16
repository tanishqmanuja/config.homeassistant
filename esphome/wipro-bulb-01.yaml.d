# Wipro Garnet RGBCCT Bulb

substitutions:
  IP: 192.168.0.75
  DEVICE_NAME: wiprobulb01
  FRIENDLY_NAME: Wipro Bulb 01

packages:
  base: !include ./common/base.yaml
  board: !include ./common/board/bk7231n.yaml

api:
  encryption: !remove
  reboot_timeout: 9min

mdns:
web_server:

text_sensor:
  - platform: libretiny
    version:
      name: LibreTiny Version

output:
  - platform: libretiny_pwm
    id: output_red
    pin: P8
  - platform: libretiny_pwm
    id: output_green
    pin: P24
  - platform: libretiny_pwm
    id: output_blue
    pin: P26
  - platform: libretiny_pwm
    id: output_cold
    pin: P7
  - platform: libretiny_pwm
    id: output_warm
    pin: P6

light:
  - platform: rgbww
    id: light_rgbww
    name: Light
    color_interlock: true
    cold_white_color_temperature: 6500 K
    warm_white_color_temperature: 2700 K
    red: output_red
    green: output_green
    blue: output_blue
    cold_white: output_cold
    warm_white: output_warm
    restore_mode: RESTORE_DEFAULT_ON