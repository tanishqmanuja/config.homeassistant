name: "QT01 Entities"
unique_id: "qt01_entities"
variables:
  entities: &entity_list
    - light.tanishq_room_switcher_ch_d
    - light.tanishq_room_switcher_ch_c
    - light.tanishq_room_switcher_ch_b
    - light.cct_lightbulb
    - light.rgbcw_lightbulb
    - light.tanishq_room_desklight
    - light.dimled01_led_strip
    - light.dimled05_led_strip

state: "{{ entities | length }}"

attributes:
  list: "{{ entities }}"
  status: >
    {% set ns = namespace(out='') %}
    {% for e in entities %}
      {% set obj = expand(e) %}
      {% if obj | length == 0 %}
        {% set ns.out = ns.out ~ 'N' %}
      {% else %}
        {% set s = obj[0].state %}
        {% if s == 'on' %}
          {% set ns.out = ns.out ~ 'I' %}
        {% elif s == 'off' %}
          {% set ns.out = ns.out ~ 'O' %}
        {% elif s == 'unavailable' %}
          {% set ns.out = ns.out ~ 'U' %}
        {% else %}
          {% set ns.out = ns.out ~ 'N' %}
        {% endif %}
      {% endif %}
    {% endfor %}
    {{ ns.out }}
  brightness_list: >
    {% set ns = namespace(out=[]) %}
    {% for e in entities %}
      {% set obj = expand(e) %}
      {% if obj | length == 0 %}
        {% set ns.out = ns.out + [-1] %}
      {% else %}
        {% set attrs = obj[0].attributes %}
        {% if 'brightness' in attrs %}
          {% set b = attrs.brightness %}
          {% if b is none %}
            {% set ns.out = ns.out + [0] %}
          {% else %}
            {% set ns.out = ns.out + [b] %}
          {% endif %}
        {% else %}
          {% set ns.out = ns.out + [-1] %}
        {% endif %}
      {% endif %}
    {% endfor %}
    {{ ns.out | join(',') }}