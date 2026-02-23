#include QMK_KEYBOARD_H
#include "lib/lib8tion/lib8tion.h"  // sin8()
#include "timer.h"

/* ========= Layer ========= */
enum layers {
    L_NAV = 0,
    L_EDIT,
    L_MEDIA,
    L_F,
    L_SYM,
    L_SYS,
};

/* ========= Custom Keycodes ========= */
enum custom_keycodes {
    ENC_HOLD = QK_USER,   // halten + Encoder drehen => Layer wechseln
    TO_NAV,               // Panic: zurück auf Layer 0
    LED_MODE_TOG,         // SYS: Status-LED <-> Alle LEDs breathen
    RGB_TOG_U,            // rgblight toggle (robust, statt RGB_TOG)
    RGB_VAI_U,            // val up (robust, statt RGB_VAI)
    RGB_VAD_U,            // val down (robust, statt RGB_VAD)
};

/* ========= Encoder hold state ========= */
static bool enc_hold = false;

/* ========= LED mode ========= */
static bool led_all_mode = false; // false=Status-LED, true=alle LEDs breathen

/* ========= LED: 1 LED pro Layer (Index 0..5) ========= */
static const uint8_t layer_led_index[6] = {0, 1, 2, 3, 4, 5};

/* RGB Farben (0-255) — QMK hat rgb_t schon, NICHT neu typedefen */
static const rgb_t layer_color[6] = {
    /* NAV  */ { .r =   0, .g = 255, .b =   0 }, // grün
    /* EDIT */ { .r =   0, .g = 255, .b = 180 }, // türkis
    /* MEDIA*/ { .r =   0, .g =  80, .b = 255 }, // blau
    /* F    */ { .r = 180, .g =   0, .b = 255 }, // lila
    /* SYM  */ { .r = 255, .g =   0, .b =   0 }, // rot
    /* SYS  */ { .r = 255, .g = 120, .b =   0 }  // orange
};

static void rgblight_init_silent(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(0, 0, 0);
    rgblight_set();
}

static void layer_led_breathe_task(void) {
    static uint8_t phase = 0;
    phase += 2;

    uint8_t v = sin8(phase);
    v = scale8(v, 200) + 25;

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer > L_SYS) layer = L_NAV;

    rgb_t c = layer_color[layer];

    // alles aus
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        rgblight_setrgb_at(0, 0, 0, i);
    }

    if (led_all_mode) {
        // alle LEDs in Layerfarbe breathen
        for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
            rgblight_setrgb_at(
                scale8(c.r, v),
                scale8(c.g, v),
                scale8(c.b, v),
                i
            );
        }
    } else {
        // nur Status-LED
        uint8_t idx = layer_led_index[layer];
        rgblight_setrgb_at(
            scale8(c.r, v),
            scale8(c.g, v),
            scale8(c.b, v),
            idx
        );
    }

    rgblight_set();
}

/* ========= Keymap ========= */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  [L_NAV] = LAYOUT_2x3(
    KC_UP,   KC_DOWN, KC_LEFT,
    KC_RGHT, KC_PGUP, ENC_HOLD
  ),

  [L_EDIT] = LAYOUT_2x3(
    C(KC_C), C(KC_X), C(KC_V),
    C(KC_Y), C(KC_Z), ENC_HOLD
  ),

  [L_MEDIA] = LAYOUT_2x3(
    KC_MPLY, KC_MPRV, KC_MNXT,
    KC_VOLD, KC_VOLU, ENC_HOLD
  ),

  [L_F] = LAYOUT_2x3(
    KC_F1, KC_F2, KC_F3,
    KC_F4, KC_F5, ENC_HOLD
  ),

  [L_SYM] = LAYOUT_2x3(
    KC_LBRC, KC_RBRC, KC_BSLS,
    KC_GRV,  KC_MINS, ENC_HOLD
  ),

  /* SYS: Brightness + LED-Mode + Panic + Boot */
  [L_SYS] = LAYOUT_2x3(
    RGB_VAD_U, RGB_VAI_U, LED_MODE_TOG,
    RGB_TOG_U, TO_NAV,    QK_BOOT
  )
};

/* ========= Encoder ========= */
bool encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer > L_SYS) layer = L_NAV;

    if (enc_hold) {
        uint8_t cur = get_highest_layer(layer_state);
        if (cur > L_SYS) cur = L_NAV;

        uint8_t next = clockwise ? (cur + 1) : (cur + 5);
        next %= 6;

        layer_move(next);   // <- DAS ist der Layerwechsel
        return false;
    }

    switch (layer) {
        case L_NAV:   tap_code(clockwise ? KC_PGDN : KC_PGUP); break;
        case L_EDIT:  tap_code16(clockwise ? C(KC_Y) : C(KC_Z)); break;
        case L_MEDIA: tap_code(clockwise ? KC_VOLU : KC_VOLD); break;
        case L_F:     tap_code(clockwise ? KC_F6 : KC_F5); break;
        case L_SYM:   tap_code16(clockwise ? KC_TAB : S(KC_TAB)); break;
        case L_SYS:
            // in SYS: Val ändern
            if (clockwise) rgblight_increase_val_noeeprom();
            else           rgblight_decrease_val_noeeprom();
            break;
    }

    return false;
}

/* ========= Custom Keycodes ========= */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {

        case ENC_HOLD:
            enc_hold = record->event.pressed;
            return false;

        case TO_NAV:
            if (record->event.pressed) {
                layer_move(L_NAV);
            }
            return false;
            
        case LED_MODE_TOG:
            if (record->event.pressed) {
                led_all_mode = !led_all_mode;
            }
            return false;

        case RGB_TOG_U:
            if (record->event.pressed) {
                rgblight_toggle_noeeprom();
            }
            return false;

        case RGB_VAI_U:
            if (record->event.pressed) {
                rgblight_increase_val_noeeprom();
            }
            return false;

        case RGB_VAD_U:
            if (record->event.pressed) {
                rgblight_decrease_val_noeeprom();
            }
            return false;
    }
    return true;
}

/* ========= RGB init + breathe task ========= */
void keyboard_post_init_user(void) {
    rgblight_init_silent();
}

void matrix_scan_user(void) {
    static uint16_t last = 0;
    uint16_t now = timer_read();
    if (timer_elapsed(last) > 15) {
        last = now;
        layer_led_breathe_task();
    }
}