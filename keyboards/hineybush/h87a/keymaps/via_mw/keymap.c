#include QMK_KEYBOARD_H

// VIA에서 "Any"로 넣을 수 있는 커스텀 키코드:
// - A 토글(가상 홀드): 0x7E40 (QK_USER_0)
enum custom_keycodes {
    A_TOG = QK_USER_0,
};

static bool a_latched            = false; // 가상으로 A가 눌린 상태인지
static bool a_phys_suppressed_dn = false; // 물리 A down 이벤트를 우리가 막았는지(대응 release도 막기 위함)
static bool a_phys_down          = false; // 물리 A가 현재 눌린 상태인지
static bool a_keep_latch_on_up   = false; // 물리 A를 누른 상태에서 토글 ON 되었을 때, 해당 물리 up을 무시(홀드 유지)

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case A_TOG:
            if (record->event.pressed) {
                if (!a_latched) {
                    register_code(KC_A);
                    a_latched = true;
                    // 물리 A를 이미 누르고 있는 상태에서 토글로 A를 켠 경우,
                    // 이후 물리 A를 떼더라도 토글 홀드가 유지되도록 up 이벤트를 한 번 막는다.
                    if (a_phys_down) {
                        a_keep_latch_on_up = true;
                    }
                } else {
                    // 토글 OFF:
                    // - 물리 A down이 "억제된" 상태라면(=a_phys_suppressed_dn), 토글이 유일한 A 홀드이므로 반드시 해제
                    // - 물리 A가 실제로(호스트에) 눌린 상태라면, 토글을 끄더라도 물리 A가 유지되도록 키업을 기다림
                    if (a_phys_suppressed_dn && a_phys_down) {
                        // 토글 홀드를 끄는 순간에도 물리 A는 계속 누르고 있는 상태일 수 있음.
                        // 이 경우, 토글 홀드는 해제하되 "현재 물리 A down" 상태가 호스트에 반영되도록 press를 재생성한다.
                        unregister_code(KC_A);
                        a_latched = false;

                        a_phys_suppressed_dn = false; // 이후 물리 A up은 정상적으로 통과
                        register_code(KC_A);          // 물리 A가 내려가 있는 상태를 호스트에 반영
                    } else if (a_phys_suppressed_dn) {
                        unregister_code(KC_A);
                        a_latched = false;
                    } else if (a_phys_down) {
                        a_latched          = false;
                        a_keep_latch_on_up = false; // 물리 A up을 통과시켜 정상 해제되도록
                    } else {
                        unregister_code(KC_A);
                        a_latched = false;
                    }
                }
            }
            return false;

        // 물리 A키의 동작을 가로채서, "토글로 A가 눌린 상태일 때"의 예외 규칙을 구현
        case KC_A:
            if (record->event.pressed) {
                a_phys_down = true;
                if (a_latched) {
                    // 이미 A가 가상으로 눌려있다면, 물리 A down은 변화 없이 무시
                    a_phys_suppressed_dn = true;
                    return false;
                }
            } else {
                a_phys_down = false;
                if (a_keep_latch_on_up) {
                    // (물리 A를 누른 상태에서 토글 ON 된 케이스) 물리 A up을 무시해서 홀드 유지
                    a_keep_latch_on_up = false;
                    return false;
                }
                if (a_phys_suppressed_dn) {
                    // (가상 홀드 중) 물리 A를 눌렀다 뗀 순간 = A를 뗀 것과 같게 + 토글 상태 해제
                    a_phys_suppressed_dn = false;
                    if (a_latched) {
                        unregister_code(KC_A);
                        a_latched = false;
                    }
                    return false;
                }
            }
            return true;
    }
    return true;
}

// VIA 키맵은 보통 4 레이어를 사용합니다(원하면 VIA에서 변경 가능).
// h87a는 다양한 레이아웃을 지원하므로, 기본 키맵과 동일하게 LAYOUT_all을 사용합니다.
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_all(
        KC_ESC,           KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,    KC_F10,     KC_F11,     KC_F12,                  KC_PSCR, KC_SCRL, KC_PAUS,
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,     KC_MINS,    KC_EQL,     KC_BSPC,   KC_BSPC,      KC_INS,  KC_HOME, KC_PGUP,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,     KC_LBRC,    KC_RBRC,    KC_BSLS,                 KC_DEL,  KC_END,  KC_PGDN,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,  KC_QUOT,    KC_NUHS,    KC_ENT,
        KC_LSFT, KC_NUBS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,   KC_SLSH,    KC_RSFT,    KC_TRNS,                          KC_UP,
        KC_LCTL, KC_LGUI, KC_LALT,                   KC_SPC,                                      KC_RALT,  MO(1),      KC_RGUI,      KC_RCTL,               KC_LEFT, KC_DOWN, KC_RGHT
    ),

    [1] = LAYOUT_all(
        _______,          _______, _______, _______, _______, UG_TOGG, UG_NEXT, UG_HUED, UG_HUEU,          UG_SATD, UG_SATU, UG_VALD, UG_VALU,                        BL_TOGG, BL_DOWN, A_TOG,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, KC_VOLU,
        _______, _______, _______, _______, QK_BOOT, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 KC_MPLY, KC_MNXT, KC_VOLD,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______,                                     _______, _______, _______, _______,                          _______, _______, _______
    ),

    [2] = LAYOUT_all(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______,                                     _______, _______, _______, _______,                          _______, _______, _______
    ),

    [3] = LAYOUT_all(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                 _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______,                                     _______, _______, _______, _______,                          _______, _______, _______
    )
};


