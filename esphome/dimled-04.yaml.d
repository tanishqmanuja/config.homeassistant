substitutions:
  ip: 192.168.29.85
  device_name: dimled04
  friendly_name: DimLED 04
  default_brightness: 50%

packages:
  dimled.mk2: !include ./dimled/mk2.base.yaml
  mods.boot-brightness: !include ./dimled/mods/boot-brightness.yaml
  mods.wifi-indicator: !include ./dimled/mods/wifi-indicator.yaml