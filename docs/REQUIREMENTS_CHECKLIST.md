# Course requirements -- coverage checklist

Source: `Projet STM32.pptx` and `CLAUDE.md`. Status legend:

- [x] satisfied by firmware in this repo (verified by reading the code)
- [~] satisfied by firmware but requires hardware validation on the bench
- [ ] not implemented

## Peripherals

| # | Requirement                                                  | Status | Where                                                                                  |
|---|---------------------------------------------------------------|--------|----------------------------------------------------------------------------------------|
| 1 | GPIO with at least one LED                                    | [x]    | `MX_GPIO_Init()` configures LD2 + L0..L3; `Weather_UpdateLeds()` drives them.          |
| 2 | GPIO with at least one button                                 | [x]    | PC13 / B1 configured as EXTI rising in `MX_GPIO_Init()`.                               |
| 3 | TIMER                                                          | [x]    | TIM6 base timer, `MX_TIM6_Init()`.                                                     |
| 4 | ADC                                                            | [x]    | ADC1 IN0 on PA0, `MX_ADC_Init()` + `Weather_ReadAdc()`.                                |
| 5 | UART with printf                                              | [x]    | USART2 115200 8N1, `__io_putchar` retargets printf in `main.c`.                        |
| 6 | SPI and/or I2C                                                | [x]    | I2C1 fast mode on PB8/PB9 (`MX_I2C1_Init()`), used by IKS01A3 BSP.                     |

## Interrupts (at least two peripherals in interrupt mode)

| # | Interrupt                                                     | Status | Where                                                                                  |
|---|---------------------------------------------------------------|--------|----------------------------------------------------------------------------------------|
| 1 | TIMER interrupt -- TIM6 1 Hz sampling tick                    | [x]    | `HAL_TIM_Base_Start_IT(&htim6)` + `HAL_TIM_PeriodElapsedCallback` in `main.c`.        |
| 2 | GPIO EXTI interrupt -- PC13 mode toggle                       | [x]    | `EXTI15_10_IRQHandler` -> `HAL_GPIO_EXTI_Callback` with 50 ms software debounce.       |

## Sensors (at least two from the shield)

| # | Sensor              | Status | Where                                                                                  |
|---|---------------------|--------|----------------------------------------------------------------------------------------|
| 1 | HTS221 (humidity)   | [x]    | `Weather_InitSensors()` initialises and enables `ENV_HUMIDITY` on `IKS01A3_HTS221_0`.  |
| 2 | LPS22HH (pressure)  | [x]    | `Weather_InitSensors()` initialises and enables `ENV_PRESSURE` on `IKS01A3_LPS22HH_0`. |
| 3 | STTS751 (temperature, optional fallback HTS221 temp) | [x] | `Weather_InitSensors()` initialises `ENV_TEMPERATURE` on `IKS01A3_STTS751_0` when available. |

## Coding rules (from `CLAUDE.md`)

| Rule                                                          | Status | Notes                                                                          |
|---------------------------------------------------------------|--------|--------------------------------------------------------------------------------|
| CubeMX-generated code untouched, user code in USER CODE blocks | [x]    | All custom logic lives in `USER CODE` blocks of `main.c` + `weather.c/.h`.    |
| ISR callbacks only set volatile flags                          | [x]    | `g_sample_flag` and `g_mode_toggle` are `volatile uint8_t`, set in callbacks. |
| Button debounce via `HAL_GetTick()`, ~50 ms                    | [x]    | `BUTTON_DEBOUNCE_MS = 50`, applied in `HAL_GPIO_EXTI_Callback`.                |
| `snprintf` for UART formatting                                 | [x]    | `Weather_PrintUart()` uses a static `snprintf` buffer.                         |
| No heap, small static buffers                                  | [x]    | Heap = 0x200, only 96-byte static UART buffer.                                 |
| `float` for env values                                         | [x]    | `WeatherSample` stores temperature/humidity/pressure as `float`.               |
| Check HAL / BSP return values                                  | [x]    | `Weather_InitSensors`, `Weather_ReadSensors`, `Weather_ReadAdc` all check.     |
| Professional UART strings                                      | [x]    | No joke/debug strings.                                                         |

## Deliverables

| Deliverable                                                   | Status | Where                                                                                  |
|---------------------------------------------------------------|--------|----------------------------------------------------------------------------------------|
| Working physical mockup                                       | [~]    | Needs hardware on the bench to validate.                                               |
| Source code in repo with usable README                        | [x]    | `STM32_METEO_STATION/README.md`.                                                       |
| 5-6 slide presentation outline                                | [x]    | `docs/PRESENTATION_OUTLINE.md` (6 slides).                                             |
| <= 20 minute demo plan                                         | [x]    | README "Demo scenario" + slide 5.                                                      |

## Items still requiring bench validation

These need the physical board + shield to confirm:

- [ ] Boot banner appears on the terminal.
- [ ] HTS221 + LPS22HH + STTS751 all init OK (no `[SENSOR_ERR]`).
- [ ] LD2 toggles at exactly 1 Hz.
- [ ] Temperature, humidity, pressure values are plausible at room conditions.
- [ ] Potentiometer changes the threshold; L3 turns on when crossed.
- [ ] Button B1 toggles LIVE / FROZEN and L2 follows.
- [ ] No double-trigger on the button (debounce holds).
- [ ] Removing the shield mid-run produces `[SENSOR_ERR]` and L1 turns on.

## Optional / not implemented

- [ ] SPI MAX7219 7-segment display. Would conflict with LD2 on PA5 -- see `BOARD.md` section "PA5 conflict". If used, drop the heartbeat LED and reassign sample-valid indicator to L0 only.
- [ ] UART RX command parser (e.g. on-the-fly threshold setting).
- [ ] DMA-based ADC streaming.
- [ ] EEPROM logging of min/max.
