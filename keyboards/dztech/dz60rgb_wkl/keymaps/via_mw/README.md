# DZ60RGB WKL V2 (Mass Storage) - via_mw 키맵

이 폴더는 **DZ60RGB WKL V2 계열**에서 다음을 해결하기 위한 커스텀 키맵입니다.

- **VIA 매크로 딜레이 사용**: VIA에서 보이던 `"Upgrade Firmware to use delays"` 상황을 해결하기 위해, 최신 QMK/VIA 프로토콜 기반으로 빌드합니다.
- **A 토글(가상 홀드) 키 추가**: 특정 키를 누르면 A가 눌린 상태(홀드)로 유지되고, 다시 누르면 해제되도록 구현했습니다.

> 중요: 플래시 후에도 **VIA 사용 가능**하도록 `VIA_ENABLE = yes`로 빌드합니다.

---

## 보드/부트로더 확인 (v2_1 = KBDFANS 드라이브)

이 키맵은 사용자가 확인한 것처럼, 부트로더 진입 시 **`KBDFANS`라는 USB 드라이브가 뜨고 `FLASH.BIN` 파일이 보이는 타입**(Mass Storage 부트로더) 기준으로 안내합니다.

- ESC 키를 누른 상태로 USB 연결 → **`KBDFANS (예: D:\)` 드라이브가 나타남**
- 드라이브 안에 **`FLASH.BIN`** 존재

이 경우 QMK 타겟은 보통:

- `dztech/dz60rgb_wkl/v2_1`

---

## 빌드 방법 (Windows)

### 준비물
- QMK 소스(이 저장소)
- Windows에서 빌드하려면 **QMK MSYS** 설치가 필요합니다.
  - 문서: `https://docs.qmk.fm/#/newbs_getting_started?id=windows-10`

### 빌드 명령
QMK MSYS에서:

```bash
cd /c/Users/myungwoo/MW/QMK
make dztech/dz60rgb_wkl/v2_1:via_mw
```

### 빌드 결과물 위치
성공하면 보통 QMK 루트에 결과물이 생성됩니다:

- `dztech_dz60rgb_wkl_v2_1_via_mw.bin`

---

## 설치(플래시) 방법: KBDFANS 드라이브에 `FLASH.BIN` 덮어쓰기

1. ESC 키를 누른 상태로 USB 연결 → **`KBDFANS` 드라이브가 뜨는지 확인**
2. `dztech_dz60rgb_wkl_v2_1_via_mw.bin` 파일을 **드라이브의 `FLASH.BIN`으로 덮어쓰기**
   - Windows는 대소문자를 보통 구분하지 않지만, 혼동 방지를 위해 파일명을 **정확히 `FLASH.BIN`**으로 맞추는 것을 권장합니다.
3. 복사 완료 후 잠시 기다리면(또는 안전하게 제거) 키보드가 재부팅되며 새 펌웨어로 동작합니다.

> 주의: 복사 중 케이블을 뽑지 마세요.

---

## VIA에서 사용하기

### 1) A 토글(가상 홀드) 키 배치
이 키맵에는 커스텀 키코드 **`A_TOG`**가 있습니다.

- VIA에서 원하는 키 선택 → **Any** → 아래 값을 입력
  - **`0x7E40`** (QK_USER_0)

동작:
- `A_TOG` 1회: **A가 눌린 상태로 유지(홀드)**
- `A_TOG` 다시 1회: **A 해제**
- **물리 A를 누르고 있는 도중에 `A_TOG`를 눌러 홀드를 켜면**, 이후 물리 A를 떼더라도 **A 홀드가 유지**됩니다.
- A 홀드가 켜진 상태에서 **물리 A 키를 누르면 변화 없음**
- 그 상태에서 **물리 A 키를 떼는 순간**: **A 홀드가 해제(= A 뗀 것과 같게)** 되고, 다음에 `A_TOG`를 다시 누르면 다시 홀드가 켜집니다.

### 2) AUTOCLICK(홀드 오토클릭) 키 배치
이 키맵에는 커스텀 키코드 **`AUTOCLICK`**도 있습니다.

- VIA에서 원하는 키 선택 → **Any** → 아래 값을 입력
  - **`0x7E41`** (QK_USER_1)

동작:
- `AUTOCLICK`을 **누르고 있는 동안** 마우스 **좌클릭을 50ms 간격**으로 반복합니다.
- 구현이 `wait_ms()` 블로킹이 아니라 **타이머 기반**이라, 오토클릭을 누른 상태에서도 **ESC 등 다른 키 입력이 정상 동작**합니다.

튜닝(펌웨어 코드 수정 시):
- `AUTOCLICK_INTERVAL_MS` (기본 50ms)
- `AUTOCLICK_PULSE_MS` (기본 5ms)

### 2) VIA 매크로 딜레이
새 펌웨어로 플래시한 뒤 VIA에서 매크로 편집 시 딜레이가 더 이상 막히지 않아야 합니다.

---

## 파일 구성
- `rules.mk`
  - `VIA_ENABLE = yes` (VIA 사용)
  - `LTO_ENABLE = yes` (용량 최적화)
  - `MOUSEKEY_ENABLE = yes` (오토클릭/마우스 버튼 키코드용)
- `keymap.c`
  - `A_TOG (0x7E40)`, `AUTOCLICK (0x7E41)` 커스텀 키코드 및 동작 구현
