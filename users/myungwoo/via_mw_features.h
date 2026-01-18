// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include QMK_KEYBOARD_H

// VIA에서 "Any"로 넣을 수 있는 커스텀 키코드(공통):
// - A 토글(가상 홀드): 0x7E40 (QK_USER_0)
// - 오토클릭(홀드 중 50ms 간격 좌클릭): 0x7E41 (QK_USER_1)
enum via_mw_custom_keycodes {
    VIA_MW_A_TOG     = QK_USER_0,
    VIA_MW_AUTOCLICK = QK_USER_1,
};

// 기존 keymap.c(및 README)에서 쓰던 이름을 그대로 유지하기 위한 호환 매크로
#ifndef A_TOG
#    define A_TOG VIA_MW_A_TOG
#endif
#ifndef AUTOCLICK
#    define AUTOCLICK VIA_MW_AUTOCLICK
#endif

// 공용 처리 훅: keymap.c의 process_record_user/matrix_scan_user/shutdown_user에서 호출
bool via_mw_process_record_user(uint16_t keycode, keyrecord_t *record);
void via_mw_matrix_scan_user(void);
void via_mw_shutdown_user(void);


