#pragma once

#define DEBOUNCE 25
#define DYNAMIC_KEYMAP_LAYER_COUNT 6// RGB / WS2812
#define RGBLIGHT_LIMIT_VAL 150
#define RGBLIGHT_SLEEP

// Encoder: Viele Encoder sind 4 Schritte pro Rastung.
// Das macht’s „normal“ statt Maschinengewehr.
#define ENCODER_RESOLUTION 4
#define DEBOUNCE 25

// Pro Micro A3 entspricht F4 in QMK/AVR
#define WS2812_DI_PIN F4
#define RGBLIGHT_LED_COUNT 6

// Optional, aber sinnvoll
#define RGBLIGHT_LIMIT_VAL 150
#define RGBLIGHT_SLEEP

#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 0