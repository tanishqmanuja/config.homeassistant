sensor: binary_sensor.tanishq_room_entry_presence
entity: light.dimled03_led_strip
sensor_type: duration
delay: 3
service_data:
  brightness_pct: 50
service_data_on: 
  transition: 1
service_data_off: 
  transition: 3