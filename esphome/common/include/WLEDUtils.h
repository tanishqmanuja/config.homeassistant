#pragma once

// https://github.com/wled/WLED/blob/93e011d4039d1a1e8c4a70503f2d79cf4ac44c3b/wled00/colors.cpp#L52

static uint16_t approximateKelvinFromRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (r == b) return 6550;

  if (r > b) {
    uint16_t scale = 0xFFFF / r;
    b = ((uint16_t)b * scale) >> 8;

    if (b < 33)  return 1900 + b       *6;
    if (b < 72)  return 2100 + (b-33)  *10;
    if (b < 101) return 2492 + (b-72)  *14;
    if (b < 132) return 2900 + (b-101) *16;
    if (b < 159) return 3398 + (b-132) *19;
    if (b < 186) return 3906 + (b-159) *22;
    if (b < 210) return 4500 + (b-186) *25;
    if (b < 230) return 5100 + (b-210) *30;
                 return 5700 + (b-230) *34;
  } else {
    uint16_t scale = 0xFFFF / b;
    r = ((uint16_t)r * scale) >> 8;

    if (r > 225) return 6600 + (254 - r) * 50;
    uint16_t k = 8080 + (225 - r) * 86;
    return (k > 10091) ? 10091 : k;
  }
}