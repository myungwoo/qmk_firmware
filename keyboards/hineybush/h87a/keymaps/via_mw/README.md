# h87a - via_mw 키맵 (VIA + 매크로 딜레이 + A 토글)

이 폴더는 `hineybush/h87a` 기판에서 다음을 위해 만든 커스텀 키맵입니다.

- **VIA 사용**: 플래시 후에도 VIA에서 키맵/매크로를 수정할 수 있도록 `VIA_ENABLE = yes`로 빌드합니다.
- **VIA 매크로 딜레이 사용**: VIA에서 `"Upgrade Firmware to use delays"`가 뜨는 경우, 최신 QMK/VIA 프로토콜 기반 펌웨어로 해결되는 경우가 많습니다.
- **A 토글(가상 홀드) 기능**: 특정 키를 누르면 A가 눌린 상태로 유지되고, 다시 누르면 해제됩니다.

---

## 빌드 타겟

```bash
make hineybush/h87a:via_mw
```

빌드 결과물은 보통 QMK 루트에 `.hex`로 생성됩니다(예: `hineybush_h87a_via_mw.hex`).

---

## 설치(플래시) 방법 (atmega32u4 / atmel-dfu)

h87a는 `atmel-dfu` 부트로더를 사용합니다. 일반적으로 다음 중 하나로 플래시합니다.

- **QMK Toolbox**(권장): DFU 모드 진입 후 `.hex` 선택 → Flash
- **dfu-programmer**(커맨드라인):

```bash
dfu-programmer atmega32u4 erase --force
dfu-programmer atmega32u4 flash path/to/your.hex
dfu-programmer atmega32u4 reset
```

### DFU(부트로더) 모드 진입
- 가장 쉬운 방법: VIA에서 어떤 키에 **Reset/Bootloader(QK_BOOT)**를 배치한 뒤 눌러서 진입
- 또는 PCB의 RESET 버튼/패드(보드마다 위치가 다름)를 사용

---

## VIA에서 사용하기

### 1) A 토글(가상 홀드) 키 배치
이 키맵에는 커스텀 키코드 **`A_TOG`**가 있습니다.

- VIA에서 원하는 키 선택 → **Any** → 아래 값을 입력
  - **`0x7E40`** (QK_USER_0)

동작:
- `A_TOG` 1회: **A 홀드(눌림 유지)**
- `A_TOG` 다시 1회: **A 해제**
- **물리 A를 누르고 있는 도중에 `A_TOG`를 눌러 홀드를 켜면**, 이후 물리 A를 떼더라도 **A 홀드가 유지**됩니다.
- A 홀드가 켜진 상태에서 **물리 A를 누르면 변화 없음**
- 그 상태에서 **물리 A를 떼는 순간**: **A 홀드 해제(= A 뗀 것과 같게)**, 이후 `A_TOG`로 다시 홀드 가능

### 2) (편의) 기본 레이어에 넣어둔 위치
이 키맵에서는 기본적으로 **레이어 1에서 Pause 자리에 `A_TOG`**를 넣었습니다.
`MO(1)`(Fn)로 레이어 1에 들어간 뒤 Pause 위치를 누르면 토글됩니다.

---

## 파일 구성
- `rules.mk`
  - `VIA_ENABLE = yes`
  - `LTO_ENABLE = yes`
- `keymap.c`
  - `A_TOG (0x7E40)` 및 동작 로직


