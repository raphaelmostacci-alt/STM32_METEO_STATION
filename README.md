# Station Météo STM32

> Projet de fin de module **P23MRS** — ISEN Toulon — Promotion 2026  
> Équipe : **Baptiste GREHL · Mathias MOHA · Raphael MOSTACCI**

Station météorologique embarquée qui mesure la température, l'humidité et la pression atmosphérique en temps réel, et les restitue sur PC via liaison série.

| | |
|---|---|
| **Carte** | NUCLEO-L152RE (STM32L152RET6 · Cortex-M3 · 32 MHz) |
| **Shield** | X-NUCLEO-IKS01A3 |
| **Toolchain** | STM32CubeIDE + STM32Cube FW_L1 V1.10.5 + X-CUBE-MEMS1 v11.1.0 |
| **Langage** | C (HAL) |

---

## Ce que fait le projet

Toutes les secondes, **TIM6** déclenche une interruption qui :

1. lit température, humidité et pression via **I2C1** depuis le shield IKS01A3,
2. publie une ligne de télémétrie sur **USART2** à 115 200 bauds, ex. :
   ```
   METEO Ville=Marseille T=22.45 C RH=47.80 % P=1015.2 hPa
   ```
3. met à jour les **LED de statut**.

Au **démarrage** (et à chaque appui sur le bouton bleu **B1**), le programme demande de saisir une ville dans le terminal. La télémétrie est suspendue pendant la saisie et reprend dès la validation (touche Entrée).

## Périphériques utilisés

| Périphérique | Broches | Rôle | Mode |
|---|---|---|---|
| USART2 | PA2 TX / PA3 RX | `printf` 115 200 baud 8N1 via ST-LINK | Polling TX · **IRQ RX** |
| I2C1 | PB8 SCL / PB9 SDA | Capteurs IKS01A3 | Polling |
| TIM6 | — | Tick périodique 1 Hz | **IRQ** ✅ |
| EXTI13 | PC13 | Bouton B1 → re-saisie de ville | **IRQ** ✅ |
| GPIO out | PA5 (LD2) | Heartbeat (clignote à 1 Hz) | |
| GPIO out | PB1 (L0) | Mesure valide | |
| GPIO out | PB2 (L1) | Erreur capteur | |
| GPIO out | PB10 (L2) | Pause active (saisie ville) | |
| GPIO out | PB11 (L3) | — (réservé) | |

> ✅ **2 périphériques en mode interruption minimum requis : TIM6 + EXTI13 (+ USART2 RX = 3 au total)**

## Capteurs du shield utilisés

| Capteur | Grandeur | Bus |
|---|---|---|
| **HTS221** | Humidité relative (%) + Température (°C) | I2C1 |
| **LPS22HH** | Pression atmosphérique (hPa) | I2C1 |
| **STTS751** | Température de précision (°C) | I2C1 |

> ✅ **2 capteurs minimum requis : 3 utilisés**

## Structure du dépôt

```
STM32_METEO_STATION/
├── STM32_METEO_STATION.ioc          ← configuration CubeMX (source de vérité)
├── README.md
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32l1xx_it.h
│   │   └── weather.h                ← API du module applicatif
│   └── Src/
│       ├── main.c                   ← boucle principale + init périphériques
│       ├── stm32l1xx_it.c           ← callbacks TIM6 + EXTI13
│       └── weather.c                ← lecture capteurs, LED, printf UART
├── Drivers/                         ← HAL STM32L1 + BSP IKS01A3 (généré CubeMX)
├── X-CUBE-MEMS1/                    ← pilotes capteurs ST (généré CubeMX)
└── docs/
    ├── oral_script.md               ← script de présentation orale
    └── generate_pptx.py             ← script de génération du PowerPoint
```

---

## Prérequis

- **STM32CubeMX** (standalone) — pour régénérer les drivers si nécessaire
- **STM32CubeIDE** — pour compiler et flasher
- **Terminal série** : PuTTY, TeraTerm ou CoolTerm — **115 200 bauds · 8N1**
- Carte **NUCLEO-L152RE** + shield **X-NUCLEO-IKS01A3** emboîté
- Câble **USB-A / micro-USB**

---

## Mise en route rapide

### 1 · Cloner le dépôt

```bash
git clone https://github.com/<votre-compte>/STM32_METEO_STATION.git
```

### 2 · Régénérer le code HAL avec CubeMX

Les drivers HAL et le BSP IKS01A3 sont volumineux mais nécessaires à la compilation.

1. Lancer **STM32CubeMX**.
2. `File → Load Project...` → sélectionner `STM32_METEO_STATION.ioc`.
3. Si demandé, installer **STM32Cube FW_L1 V1.10.5** et **X-CUBE-MEMS1 v11.1.0** via `Help → Manage embedded software packages`.
4. Dans l'onglet **Project Manager** : vérifier `Toolchain / IDE = STM32CubeIDE` et `Keep User Code when re-generating` coché.
5. Cliquer **GENERATE CODE**.

> ⚠️ Si CubeMX écrase vos fichiers utilisateur, restaurer depuis le dépôt :
> `Core/Src/main.c`, `weather.c`, `stm32l1xx_it.c` et `Core/Inc/main.h`, `weather.h`, `stm32l1xx_it.h`.

### 3 · Importer dans STM32CubeIDE

1. Lancer **STM32CubeIDE**.
2. `File → Open Projects from File System...` → sélectionner le dossier `STM32_METEO_STATION` → **Finish**.

### 4 · Compiler et flasher

1. `Project → Build All` (`Ctrl+B`).
2. Brancher la Nucleo (ST-LINK USB).
3. Clic droit sur le projet → `Run As → STM32 C/C++ Application`.

---

## Utilisation

1. Ouvrir le terminal série sur le **port COM** de la Nucleo (Gestionnaire de périphériques), **115 200 bauds · 8N1**.
2. Appuyer sur le bouton **reset** (noir) de la Nucleo. La bannière de boot s'affiche :
   ```
   === STM32 METEO STATION boot ===
   Board: NUCLEO-L152RE + X-NUCLEO-IKS01A3
   Capteurs HTS221 + LPS22HH + STTS751 prets.
   Entrez la ville :
   ```
3. Saisir un nom de ville et valider avec **Entrée**. La télémétrie démarre :
   ```
   Ville selectionnee : Marseille
   METEO Ville=Marseille T=22.45 C RH=47.80 % P=1015.2 hPa
   METEO Ville=Marseille T=22.47 C RH=47.92 % P=1015.2 hPa
   ```
4. Appuyer sur le **bouton bleu B1** pour changer de ville à tout moment.

### LED de statut

| LED | Broche | État |
|---|---|---|
| LD2 (verte) | PA5 | Clignote à 1 Hz — heartbeat |
| L0 | PB1 | Allumée = mesure valide |
| L1 | PB2 | Allumée = erreur capteur |
| L2 | PB10 | Allumée = saisie ville en cours |

---

## Configuration CubeMX (récréer depuis zéro)

<details>
<summary>Cliquer pour afficher</summary>

- **Carte :** Nucleo-L152RE
- **RCC :** HSI interne
- **SYS :** Debug = Serial Wire, Timebase = SysTick
- **USART2 :** Asynchrone, 115 200, 8N1. PA2 = TX, PA3 = RX
- **I2C1 :** Fast mode 400 kHz. PB8 = SCL, PB9 = SDA
- **TIM6 :** Activé. Prescaler `31999`, Period `999`. IRQ globale activée dans NVIC
- **GPIO sorties** (Push-Pull, no pull, low speed) : PA5 (`LD2`), PB1 (`L0`), PB2 (`L1`), PB10 (`L2`), PB11 (`L3`)
- **GPIO EXTI :** PC13 — Rising edge, no pull. `EXTI line[15:10]` activée dans NVIC (priorité 1)
- **Software Pack :** `STMicroelectronics.X-CUBE-MEMS1 v11.1.0` → `Board Extension IKS01A3` (bus = I2C1)
- **Clock :** HSI 16 MHz → PLL (×6 ÷3) → SYSCLK = 32 MHz

</details>
  - Firmware package = `STM32Cube FW_L1 V1.10.5`.
  - `Code Generator -> Generate peripheral initialization as a pair of '.c/.h' files per peripheral` = unchecked (single `main.c`).
  - `Code Generator -> Keep User Code when re-generating` = **checked**.

Then `Project -> Generate Code`.

## Build and flash

In STM32CubeIDE:

1. `Project -> Build All` (`Ctrl+B`).
2. Connect the Nucleo over USB. ST-LINK should enumerate as a USB CDC + ST-LINK device.
3. Right-click the project -> `Run As -> STM32 C/C++ Application` (or click the Run icon).
4. The ST-LINK programmer flashes and resets the board.

## Terminal

- Port: ST-LINK Virtual COM (Windows: `COMx` shown in Device Manager).
- Baud rate: **115200**.
- Settings: **8 data bits, no parity, 1 stop bit, no flow control**.
- Tools: TeraTerm (shipped here as `teraterm-4.103.exe`), PuTTY, `screen`, or the STM32CubeIDE built-in terminal.

Expected at startup:

```
=== STM32 METEO STATION boot ===
Board: NUCLEO-L152RE + X-NUCLEO-IKS01A3
UART2 115200 8N1, TIM6 sampling 1 Hz, EXTI13 mode toggle.
Sensors HTS221 + LPS22HH + STTS751 ready.
METEO T=23.45 C RH=48.2 % P=1013.4 hPa ADC=2048 mode=LIVE
METEO T=23.46 C RH=48.1 % P=1013.4 hPa ADC=2050 mode=LIVE
...
```

## Demo scenario (5 minutes)

1. Power the board, open the serial terminal. Show the `=== STM32 METEO STATION boot ===` banner and 3 sensors confirmed.
2. Show LD2 blinking at 1 Hz (heartbeat from TIM6 interrupt).
3. Breathe on the shield — humidity rises a few percent, the UART line reflects it.
4. Touch the STTS751/HTS221 with a fingertip — temperature climbs ~1 °C in a few seconds.
5. Turn the potentiometer below the current ADC reading -> **L3 (PB11) lights up** (alarm).
6. Press the **blue button (B1, PC13)** — `[BTN ] mode -> FROZEN`. UART stops emitting METEO lines, **L2 (PB10) lights up**. Press again to return to LIVE.
7. Unplug the shield mid-run (optional) — `[SENSOR_ERR]` appears, L1 lights up.

## Architecture decisions

- Sensors and UART are **never** touched from inside an ISR.
- `HAL_TIM_PeriodElapsedCallback` sets `g_sample_flag = 1` and returns. The main loop performs the I2C reads and the printf.
- `HAL_GPIO_EXTI_Callback` debounces with `HAL_GetTick()` (50 ms guard) and sets `g_mode_toggle`. The main loop applies the new mode.
- All shared flags are `volatile`.
- `snprintf` is used for the telemetry line; the buffer is `static` to keep stack pressure low (SRAM = 16 KB).
- BSP return codes are checked.

## Known limitations

- ADC read is polling, not DMA — fine at 1 Hz, would need rework for fast sampling.
- No persistent storage of measurements (EEPROM available but unused).
- STTS751 may not exist on every IKS01A3 variant; the firmware falls back to HTS221 temperature when the BSP macro `IKS01A3_STTS751_0` is missing.
- Pressure value is in hPa as returned by the BSP; no sea-level correction.
- Optional MAX7219 7-segment display is not wired in this build (would conflict with LD2 on PA5 — see `BOARD.md`).

## Course requirement coverage

See [docs/REQUIREMENTS_CHECKLIST.md](docs/REQUIREMENTS_CHECKLIST.md) and [docs/PRESENTATION_OUTLINE.md](docs/PRESENTATION_OUTLINE.md).
