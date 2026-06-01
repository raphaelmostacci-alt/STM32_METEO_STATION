# Presentation outline -- STM32 Meteo Station

6 slides, ~20 minutes total including the live demo.

---

## Slide 1 -- Application

**Title:** Station meteo embarquee STM32

- Objet : mesurer en continu la **temperature**, l'**humidite relative** et la **pression atmospherique** d'une piece et les diffuser sur le PC via la liaison serie.
- Cas d'usage : capteur d'ambiance pedagogique, alerte de seuil reglable a la volee, base reutilisable pour un IoT plus complet.
- Plateforme : NUCLEO-L152RE + shield X-NUCLEO-IKS01A3.

**Visuel :** photo de la carte cablee + capture du terminal serie.

---

## Slide 2 -- Solution technique

**Architecture materielle**

| Bloc                | Composant                       | Bus / Pin            |
|---------------------|----------------------------------|----------------------|
| MCU                 | STM32L152RET6 (32 MHz)          | -                    |
| Capteur humidite    | HTS221                          | I2C1, PB8/PB9        |
| Capteur pression    | LPS22HH                         | I2C1, PB8/PB9        |
| Capteur temp        | STTS751 (fallback HTS221)       | I2C1, PB8/PB9        |
| Liaison PC          | USART2 + ST-LINK VCP            | PA2/PA3 @ 115200     |
| Tick periodique     | TIM6 (IRQ 1 Hz)                 | -                    |
| Bouton mode         | B1 (PC13), EXTI13 IRQ           | -                    |
| Seuil d'alarme      | Potentiometre, ADC1 IN0         | PA0                  |
| Indicateurs         | LD2, L0..L3                      | PA5, PB1/2/10/11     |

**Architecture logicielle**

- HAL C generee par CubeMX + BSP IKS01A3 du pack X-CUBE-MEMS1.
- Pattern **flag-driven** : les ISR ne font que lever des `volatile` flags, le `while(1)` execute les lectures I2C, l'ADC, le `printf` et les LEDs.
- Module utilisateur `weather.c` / `weather.h` separe la logique capteurs/affichage du `main.c`.

**Visuel :** schema bloc + extrait du `main loop`.

---

## Slide 3 -- Defi / probleme

- **Acquisition periodique fiable** sans bloquer le CPU : on doit echantillonner exactement chaque seconde meme si une lecture I2C est lente.
- **Deux interruptions imposees** par le cahier des charges (TIM + GPIO) : il faut les rendre coherentes (priorites, rebond du bouton, callbacks non bloquants).
- **Plusieurs capteurs sur le meme bus** I2C : il faut gerer les codes d'erreur, distinguer le capteur defaillant et continuer a publier les autres mesures.
- **Ressources tres limitees** : STM32L1 = 16 KB de SRAM, donc pas d'allocation dynamique, buffers `static`.
- **Demo lisible** en 5 minutes : il faut un retour visuel immediat (LEDs) en plus du terminal.

**Visuel :** diagramme d'interruption avec TIM6 + EXTI13 et leurs flags.

---

## Slide 4 -- Resolution

- **Cadencement** : TIM6 prescaler 31999, period 999 -> 1 Hz exact. Le callback ne fait que `g_sample_flag = 1;`.
- **Bouton** : EXTI13 declenche `HAL_GPIO_EXTI_Callback`, anti-rebond logiciel par `HAL_GetTick()` (50 ms), pose `g_mode_toggle`.
- **Pipeline capteurs** : `Weather_InitSensors()` arme les trois capteurs et leve les `*_ok` flags. `Weather_ReadSensors()` ignore un capteur en panne et marque `sample.valid = 0`.
- **Telemetrie** : `snprintf` dans un buffer statique + `HAL_UART_Transmit`. Format unique facile a parser :
  `METEO T=23.45 C RH=48.2 % P=1013.4 hPa ADC=2048 mode=LIVE`.
- **ADC** : lecture polling 12 bits a chaque tick. La valeur sert directement de seuil d'alarme (L3 s'allume si `ADC > seuil`).
- **Modes** : LIVE = mesure + impression, FROZEN = pas de print, L2 allumee.
- **LEDs status** : L0 = sample valide, L1 = erreur capteur, L2 = mode FROZEN, L3 = alarme, LD2 = battement de coeur.

**Visuel :** capture annotee du terminal pendant un cycle.

---

## Slide 5 -- Demo

Scenario joue en direct (~5 minutes) :

1. Reset, banniere + sensors ready dans le terminal.
2. Heartbeat LD2 a 1 Hz visible (preuve que le TIM6 IRQ tourne).
3. Souffler / poser le doigt sur la carte : variation visible de T et RH.
4. Tourner le potentiometre -> L3 (alarme) s'allume / s'eteint en fonction du seuil.
5. Appuyer sur B1 -> `mode -> FROZEN`, L2 allumee, plus de telemetrie. Re-appuyer -> retour LIVE.
6. (Bonus) Debrancher le shield -> `[SENSOR_ERR]`, L1 allumee. Rebrancher, reset, retour OK.

**Visuel :** photo cablage + capture du terminal pendant chaque etape.

---

## Slide 6 -- Conclusion

- Toutes les exigences du sujet sont couvertes (voir `REQUIREMENTS_CHECKLIST.md`).
- Architecture **modulaire et reproductible** : le module `weather` est reutilisable pour un autre boitier ou un autre transport (LoRa, BLE, USB CDC).
- **Pistes d'amelioration :**
  - Affichage local sur MAX7219 (SPI1) si on accepte de perdre LD2 sur PA5.
  - Lecture ADC en DMA pour ne plus poller.
  - Stockage long-terme dans l'EEPROM (4 KB libres).
  - Calcul du point de rosee, de la pression au niveau de la mer.
- **Lecons retenues** : ne jamais bloquer dans une ISR, toujours verifier les retours BSP/HAL, separer la couche capteur de la couche application.

**Visuel :** photo du montage final + QR code vers le depot Git.
