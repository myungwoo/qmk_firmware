#include QMK_KEYBOARD_H
#include "timer.h"

// VIA에서 "Any"로 넣을 수 있는 커스텀 키코드:
// - A 토글(가상 홀드): 0x7E40 (QK_USER_0)
// - 오토클릭(홀드 중 50ms 간격 좌클릭): 0x7E41 (QK_USER_1)
enum custom_keycodes {
    A_TOG = QK_USER_0,
    AUTOCLICK = QK_USER_1,
};

static bool a_latched            = false; // 토글로 A가 눌린 상태인지
static bool a_phys_suppressed_dn = false; // 물리 A down 이벤트를 우리가 막았는지(대응 release도 막기 위함)
static bool a_phys_down          = false; // 물리 A가 현재 눌린 상태인지
static bool a_keep_latch_on_up   = false; // 물리 A를 누른 상태에서 토글 ON 되었을 때, 해당 물리 up을 무시(홀드 유지)

// 오토클릭 상태(블로킹 없이 matrix_scan_user에서 타이머로 처리)
static bool     autoclick_held       = false;
static bool     autoclick_btn_down   = false;
static uint16_t autoclick_last_press = 0;
static uint16_t autoclick_down_since = 0;

#ifndef AUTOCLICK_INTERVAL_MS
#    define AUTOCLICK_INTERVAL_MS 50
#endif

// “클릭”을 만들기 위한 버튼 다운 유지 시간(너무 짧으면 OS/USB 폴링에 따라 누락될 수 있음)
#ifndef AUTOCLICK_PULSE_MS
#    define AUTOCLICK_PULSE_MS 5
#endif

static void a_tog_reset_state(void) {
    if (a_latched) {
        unregister_code(KC_A);
    }
    a_latched            = false;
    a_phys_suppressed_dn = false;
    a_phys_down          = false;
    a_keep_latch_on_up   = false;
}

static void autoclick_reset_state(void) {
    autoclick_held = false;
    if (autoclick_btn_down) {
        unregister_code16(MS_BTN1);
    }
    autoclick_btn_down   = false;
    autoclick_last_press = 0;
    autoclick_down_since = 0;
}

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
                    // 토글 OFF 시점의 상태 조합에 따라 스턱/불일치를 방지한다.
                    if (a_phys_suppressed_dn && a_phys_down) {
                        // (토글 ON 상태에서 물리 A down이 억제된 상태) 토글을 끄더라도 물리 A는 계속 눌린 상태일 수 있음.
                        // 토글 홀드는 해제하되 "현재 물리 A down" 상태를 호스트에 반영한다.
                        unregister_code(KC_A);
                        a_latched = false;

                        a_phys_suppressed_dn = false; // 이후 물리 A up은 정상적으로 통과
                        register_code(KC_A);          // 물리 A가 내려가 있는 상태를 호스트에 반영
                    } else if (a_phys_suppressed_dn) {
                        // 토글 홀드가 유일한 A 홀드이므로 반드시 해제
                        unregister_code(KC_A);
                        a_latched = false;
                    } else if (a_phys_down) {
                        // 물리 A가 이미(호스트에) 눌린 상태에서 토글을 끄는 경우:
                        // 토글로 걸어둔 register를 풀되, 물리 A는 계속 눌린 상태이므로 결과적으로 A는 계속 DOWN이어야 한다.
                        unregister_code(KC_A);
                        a_latched          = false;
                        a_keep_latch_on_up = false; // 물리 A up을 통과시켜 정상 해제되도록
                    } else {
                        unregister_code(KC_A);
                        a_latched = false;
                    }
                }
            }
            return false;

        case AUTOCLICK:
            if (record->event.pressed) {
                autoclick_held = true;
                // 즉시 1번 클릭이 필요하면 아래 2줄 주석 해제
                // autoclick_last_press = timer_read() - AUTOCLICK_INTERVAL_MS;
                // (첫 press는 matrix_scan_user에서 처리)
            } else {
                autoclick_reset_state();
            }
            return false;

        // 물리 A키의 동작을 가로채서, "토글로 A가 눌린 상태일 때"의 예외 규칙을 구현
        case KC_A:
            if (record->event.pressed) {
                a_phys_down = true;
                if (a_latched) {
                    // 이미 A가 토글로 눌려있다면, 물리 A down은 변화 없이 무시
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

void matrix_scan_user(void) {
    if (!autoclick_held) {
        return;
    }

    // 50ms 간격으로 “press → 짧게 유지 → release” 펄스를 생성
    if (!autoclick_btn_down) {
        if (timer_elapsed(autoclick_last_press) >= AUTOCLICK_INTERVAL_MS) {
            register_code16(MS_BTN1);
            autoclick_btn_down   = true;
            autoclick_down_since = timer_read();
            autoclick_last_press = autoclick_down_since;
        }
    } else {
        if (timer_elapsed(autoclick_down_since) >= AUTOCLICK_PULSE_MS) {
            unregister_code16(MS_BTN1);
            autoclick_btn_down = false;
        }
    }
}

// 가드: 부트로더 점프/리셋/슬립 등으로 clear_keyboard()가 발생해도
// 내부 상태(a_latched 등)가 남아있으면 이후 동작이 꼬일 수 있으므로 리셋한다.
bool shutdown_user(bool jump_to_bootloader) {
    autoclick_reset_state();
    a_tog_reset_state();
    return true;
}

// VIA 키맵은 보통 4 레이어를 사용합니다(원하면 VIA에서 변경 가능).
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  XXXXXXX, KC_BSPC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,
        KC_LSFT, XXXXXXX, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, XXXXXXX,
        KC_LCTL, KC_LGUI, KC_LALT,                   KC_SPC,  KC_SPC,  KC_SPC,           KC_RALT, XXXXXXX, XXXXXXX, MO(1),   KC_RCTL
    ),

    [1] = LAYOUT(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______, A_TOG,
        _______, UG_TOGG, UG_NEXT, UG_HUEU, UG_HUED, UG_SATU, UG_SATD, UG_VALU, UG_VALD, _______, _______, _______, _______, QK_BOOT,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, BL_DOWN, BL_TOGG, BL_UP,   BL_STEP, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______, _______, _______,          _______, _______, _______, _______, _______
    ),

    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______, _______, _______,          _______, _______, _______, _______, _______
    ),

    [3] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______, _______, _______,          _______, _______, _______, _______, _______
    ),
};


