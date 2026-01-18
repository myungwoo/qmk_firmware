// SPDX-License-Identifier: GPL-2.0-or-later

#include "via_mw_features.h"
#include "timer.h"

// ---------------------------
// A_TOG (가상 A 홀드 토글)
// ---------------------------
static bool a_latched            = false; // 가상으로 A가 눌린 상태인지
static bool a_phys_suppressed_dn = false; // 물리 A down 이벤트를 우리가 막았는지(대응 release도 막기 위함)
static bool a_phys_down          = false; // 물리 A가 현재 눌린 상태인지
static bool a_keep_latch_on_up   = false; // 물리 A를 누른 상태에서 토글 ON 되었을 때, 해당 물리 up을 무시(홀드 유지)

static void a_tog_reset_state(void) {
    if (a_latched) {
        unregister_code(KC_A);
    }
    a_latched            = false;
    a_phys_suppressed_dn = false;
    a_phys_down          = false;
    a_keep_latch_on_up   = false;
}

// ---------------------------
// AUTOCLICK (홀드 중 좌클릭 반복)
// ---------------------------
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

static void autoclick_reset_state(void) {
    autoclick_held = false;
    if (autoclick_btn_down) {
        unregister_code16(MS_BTN1);
    }
    autoclick_btn_down   = false;
    autoclick_last_press = 0;
    autoclick_down_since = 0;
}

bool via_mw_process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case VIA_MW_A_TOG:
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

        case VIA_MW_AUTOCLICK:
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

void via_mw_matrix_scan_user(void) {
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

void via_mw_shutdown_user(void) {
    autoclick_reset_state();
    a_tog_reset_state();
}


