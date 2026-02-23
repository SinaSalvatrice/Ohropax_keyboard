#include QMK_KEYBOARD_H
#include "lib/lib8tion/lib8tion.h"  // sin8()

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
    ENC_HOLD = QK_USER,   // Encoder-Button (in Matrix!) -> halten, um Layer via Drehung zu wechseln
    TO_NAV,               // Panic: zurück auf Layer 0 (default layer)
};

/* ========= Encoder hold state ========= */
static bool enc_hold = false;

/* ========= LED: 1 LED pro Layer (Position = Key-Index 0..5) =========
   Deine gewünschte Zuordnung (Row,Col):
   L1: (0,0) grün   -> index 0
   L2: (0,1) türkis -> index 1
   L3: (0,2) blau   -> index 2
   L4: (1,0) lila   -> index 3
   L5: (1,1) rot    -> index 4
   L6: (1,2) orange -> index 5
*/
static const uint8_t layer_led_index[6] = {0, 1, 2, 3, 4, 5};

/* RGB Farben (0-255) */
typedef struct { uint8_t r, g, b; } rgb_t;
static const rgb_t layer_color[6] = {
    /* NAV  */ {  0, 255,   0 }, // grün
    /* EDIT */ {  0, 255, 180 }, // türkis
    /* MEDIA*/ {  0,  80, 255 }, // blau
    /* F    */ { 180,   0, 255 }, // lila
    /* SYM  */ { 255,   0,   0 }, // rot
    /* SYS  */ { 255, 120,   0 }  // orange
};

static void rgblight_init_silent(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(0, 0, 0);
}

static void layer_led_breathe_task(void) {
    // Phase läuft kontinuierlich -> weiches Breathing
    static uint8_t phase = 0;
    phase += 2; // Geschwindigkeit

    // sin8 liefert 0..255, wir ziehen es in einen brauchbaren Bereich
    uint8_t v = sin8(phase);              // 0..255
    v = scale8(v, 200) + 25;              // min 25, max ~225

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer > L_SYS) layer = L_NAV;

    // alles aus
    for (uint8_t i = 0; i < RGBLED_NUM; i++) {
        rgblight_setrgb_at(0, 0, 0, i);
    }

    // aktive LED setzen
    uint8_t idx = layer_led_index[layer];
    rgb_t c = layer_color[layer];

    rgblight_setrgb_at(
        scale8(c.r, v),
        scale8(c.g, v),
        scale8(c.b, v),
        idx
    );

    rgblight_set();
}

/* ========= Keymap =========
   LAYOUT_2x3:
   [ K00 K01 K02 ]
   [ K10 K11 K12 ]
*/
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  /* NAV (Layer 0) */
  [L_NAV] = LAYOUT_2x3(
    KC_UP,   KC_DOWN, KC_LEFT,
    KC_RGHT, KC_PGUP, ENC_HOLD   // <- Encoder-Button: Standard hier (1,2). Falls falsch: Taste umsetzen!
  ),

  /* EDIT (Layer 1): copy/cut/paste/redo/undo + Hold */
  [L_EDIT] = LAYOUT_2x3(
    C(KC_C), C(KC_X), C(KC_V),
    C(KC_Y), C(KC_Z), ENC_HOLD
  ),

  /* MEDIA (Layer 2) */
  [L_MEDIA] = LAYOUT_2x3(
    KC_MPLY, KC_MPRV, KC_MNXT,
    KC_VOLD, KC_VOLU, ENC_HOLD
  ),

  /* F (Layer 3) */
  [L_F] = LAYOUT_2x3(
    KC_F1, KC_F2, KC_F3,
    KC_F4, KC_F5, ENC_HOLD
  ),

  /* SYM (Layer 4) – Platzhalter sinnvoll für “coding-ish” */
  [L_SYM] = LAYOUT_2x3(
    KC_LBRC, KC_RBRC, KC_BSLS,
    KC_GRV,  KC_MINS, ENC_HOLD
  ),

  /* SYS (Layer 5): Helligkeit + Panic + Boot */
  [L_SYS] = LAYOUT_2x3(
    KC_BRID, KC_BRIU, TO_NAV,
    RGB_TOG, RGB_VAD, QK_BOOT
  )
};

/* ========= Encoder =========
   - Ohne Hold: layerabhängige Vorschläge (kannst du leicht ändern)
   - Mit Hold (ENC_HOLD gedrückt): Encoder wechselt Default-Layer 0..5
*/
bool encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer > L_SYS) layer = L_NAV;

    if (enc_hold) {
        // Default layer durchschalten
        int8_t dl = get_highest_layer(default_layer_state);
        if (dl > L_SYS) dl = L_NAV;

        if (clockwise) dl++; else dl--;
        if (dl < 0) dl = L_SYS;
        if (dl > L_SYS) dl = L_NAV;

        set_single_persistent_default_layer((uint8_t)dl);
        return false;
    }

    // Ohne Hold: Vorschläge pro Layer
    switch (layer) {
        case L_NAV:
            // scroll/pgup/pgdn ist oft nützlich:
            tap_code(clockwise ? KC_PGDN : KC_PGUP);
            break;

        case L_EDIT:
            // redo/undo auf Encoder
            tap_code16(clockwise ? C(KC_Y) : C(KC_Z));
            break;

        case L_MEDIA:
            // Volume
            tap_code(clockwise ? KC_VOLU : KC_VOLD);
            break;

        case L_F:
            // F5/F6 (z.B. refresh/build) – frei anpassbar
            tap_code(clockwise ? KC_F6 : KC_F5);
            break;

        case L_SYM:
            // Tab / Shift+Tab als “Navigation in IDE”
            tap_code16(clockwise ? KC_TAB : S(KC_TAB));
            break;

        case L_SYS:
            // RGB brightness (Value)
            tap_code(clockwise ? RGB_VAI : RGB_VAD);
            break;
    }

    return false;
}

/* ========= ENC_HOLD + TO_NAV ========= */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {

        case ENC_HOLD:
            enc_hold = record->event.pressed;
            return false;

        case TO_NAV:
            if (record->event.pressed) {
                set_single_persistent_default_layer(L_NAV);
                layer_clear();
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
    // 60-70 fps reicht, aber wir halten’s leicht:
    static uint16_t last = 0;
    uint16_t now = timer_read();
    if (timer_elapsed(last) > 15) {
        last = now;
        layer_led_breathe_task();
    }
}
