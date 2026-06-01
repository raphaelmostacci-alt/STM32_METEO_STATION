# Script oral — Station Météo STM32

**P23MRS · ISEN Toulon · 1er juin 2026**

| Personne | Slides | Durée |
|---|---|---|
| Baptiste GREHL | 01 Application + 02 Solution | ~7 min |
| Mathias MOHA | 03 Défis + 04 Résolution | ~8 min |
| Raphael MOSTACCI | 05 Démo + 06 Conclusion | ~5 min |

> ⚠️ **Note :** L'ADC a été retiré du projet final pour simplifier et fiabiliser l'ensemble. Il est mentionné comme tel en slide 6.

---

## Baptiste GREHL — Slide 01/06 · Application (~3 min)

*[Avancer sur slide 1 — STATION METEO STM32]*

Bonjour à tous. Je m'appelle Baptiste GREHL, je suis avec Mathias MOHA et Raphael MOSTACCI. Nous sommes en promotion 2026 à l'ISEN Toulon, et nous allons vous présenter notre projet de fin de module P23MRS : **la Station Météo STM32**.

L'idée de départ est simple : mesurer, en temps réel, la température, l'humidité et la pression atmosphérique d'une pièce, et les exposer en direct sur un PC via un terminal série.

Matériel, en deux mots : une carte **Nucleo-L152RE** — c'est un STM32L1, Cortex-M3 cadencé à 32 MHz — surplombée d'un shield **X-NUCLEO-IKS01A3** qui embarque plusieurs capteurs environnementaux de chez ST.

Le contexte du projet imposait d'utiliser au moins deux périphériques en mode interruption et au moins deux capteurs du shield. On a respecté ces deux contraintes, et même un peu plus — on y revient dans les slides suivantes.

On a 20 minutes au total, dont une démonstration en direct. On va donc aller droit au but.

*[Transition → slide 2]*

---

## Baptiste GREHL — Slide 02/06 · Solution (~4 min)

*[Avancer sur slide 2 — Architecture matérielle et logicielle]*

Pour construire cette station, on a fait deux choix structurants.

### Côté matériel

On a trois capteurs sur le bus **I2C1** :
- **HTS221** — humidité + température
- **LPS22HH** — pression atmosphérique
- **STTS751** — température de précision complémentaire

Tous les trois communiquent via I2C1 sur les broches PB8 et PB9.

Pour la communication avec le PC, on utilise **USART2** à 115 200 bauds, branché sur le convertisseur ST-LINK de la Nucleo. C'est le classique `printf` retargété vers `HAL_UART_Transmit`.

Le bouton bleu **B1** de la Nucleo — PC13 — sert à relancer une saisie de ville. Il est géré par interruption EXTI13.

### Côté logiciel

On a généré le projet avec STM32CubeIDE et CubeMX, ce qui nous a fourni le HAL et le **BSP IKS01A3 v11.1**. On n'a pas eu à écrire les drivers bas niveau des capteurs.

Notre logique applicative repose sur **trois interruptions** :
- **TIM6** en mode interruption, configuré pour générer un tick à exactement 1 Hz
- **EXTI13** sur le bouton B1, avec anti-rebond logiciel à 50 ms
- **USART2 RX** en interruption, pour lire la saisie de la ville caractère par caractère sans bloquer la boucle principale

Tout le code métier lié aux capteurs, aux LED et au printf est encapsulé dans un module séparé : **`weather.c` / `weather.h`**. Ça rend le `main.c` beaucoup plus lisible.

En bas de slide : 3 capteurs · 3 IRQ actives · 1 Hz · 115 200 bauds UART.

*[Transition → slide 3 — passer la parole à Mathias]*

---

## Mathias MOHA — Slide 03/06 · Défis (~4 min)

*[Avancer sur slide 3 — Quatre problèmes à régler]*

Merci Baptiste. Moi c'est Mathias MOHA. Derrière ces choix techniques se cachaient quatre problèmes concrets à résoudre.

### 1 · Acquisition périodique

Le réflexe du débutant, c'est un `HAL_Delay(1000)`. Mais ça bloque le CPU : pendant une seconde, il ne fait rien d'autre. Et une lecture I2C prend quelques millisecondes — si on les additionne sur plusieurs capteurs, on dérive. Il faut un **cadencement matériel propre**.

### 2 · Coexistence des IRQ

Le sujet imposait deux périphériques en interruption minimum. On en a utilisé **trois** : TIM6, EXTI13 et USART2 RX. Il faut que ces trois interruptions ne se marchent pas dessus, que les callbacks restent non bloquants, et que le bouton ne génère pas de faux déclenchements à cause de rebonds mécaniques.

### 3 · Erreurs multi-capteurs

Trois capteurs sur le même bus I2C. Si l'un d'eux ne répond pas à l'initialisation — par exemple un mauvais contact — on ne peut pas juste bloquer et afficher une erreur. Il faut **continuer à publier les mesures des capteurs qui fonctionnent** et signaler l'erreur via une LED dédiée.

### 4 · Interactivité UART

On voulait que l'utilisateur puisse saisir une ville et la changer en cours de route avec B1. Pendant la saisie, le TIM6 continue à tirer des ticks à 1 Hz. Si on laisse la mesure tourner, les sorties se mélangent dans le terminal. Il faut **suspendre la télémétrie proprement** sans sortir de la boucle principale.

*[Transition → slide 4]*

---

## Mathias MOHA — Slide 04/06 · Résolution (~4 min)

*[Avancer sur slide 4 — Pattern flag-driven et séparation des couches]*

Le schéma au centre du slide résume le pattern qu'on a adopté.

```
TIM6 IRQ    ─┐
EXTI13 IRQ  ─┼─>  flag volatile  ─>  while(1)  ─>  UART / LED / Heartbeat
USART2 RX   ─┘
```

Les ISR **ne font qu'une chose** : elles lèvent un flag `volatile uint8_t` et rendent la main en moins d'une microseconde. C'est la boucle principale qui surveille ces flags et fait le vrai travail.

On a quatre flags :

| Flag | Levé par | Rôle |
|---|---|---|
| `g_sample_flag` | TIM6 toutes les secondes | Déclencher une mesure |
| `g_ask_city` | EXTI13 (bouton B1) | Relancer la saisie de ville |
| `g_city_ready` | Callback UART RX (`\n` reçu) | Ville validée, reprendre |
| `g_paused` | Main loop pendant saisie | Suspendre la télémétrie |

### Anti-rebond

On utilise `HAL_GetTick()`. Dans le callback EXTI13, on compare le tick courant au dernier tick enregistré. Si la différence est **< 50 ms**, on ignore — c'est un rebond.

### Pause interactive

Quand `g_paused == 1`, le `while(1)` vide les ticks TIM6 sans rien publier. Dès la validation de la ville, `g_paused` repasse à 0 et la télémétrie reprend immédiatement.

### Robustesse capteurs

`Weather_InitSensors()` tente chaque capteur indépendamment. Si `BSP_ERROR_NONE` n'est pas retourné, le flag `s_hum_ok` / `s_press_ok` / `s_temp_ok` reste à zéro. `Weather_ReadSensors()` saute ce capteur et la LED L1 s'allume.

On a donc un code entièrement événementiel, **sans aucun `HAL_Delay` dans la boucle principale**.

*[Transition → slide 5 — passer la parole à Raphael]*

---

## Raphael MOSTACCI — Slide 05/06 · Démo (~4 min)

*[Avancer sur slide 5 — Démonstration en direct]*  
*[Avoir le terminal série ouvert sur le bon COM à 115 200 bauds]*

Merci Mathias. Je suis Raphael MOSTACCI et je vais vous faire la démonstration en cinq étapes.

### Étape 1 — Reset

*[Appuyer sur le bouton reset noir de la Nucleo]*

La bannière de boot apparaît :
```
=== STM32 METEO STATION boot ===
Board: NUCLEO-L152RE + X-NUCLEO-IKS01A3
Capteurs HTS221 + LPS22HH + STTS751 prets.
Entrez la ville :
```
Les trois capteurs sont initialisés, pas d'erreur. Le terminal nous demande immédiatement la ville.

### Étape 2 — Saisie de la ville

*[Taper `Marseille` dans le terminal, puis Entrée]*

Dès qu'on valide, `g_city_ready` passe à 1, `g_paused` revient à 0. On voit `Ville selectionnee : Marseille` et la télémétrie démarre.

### Étape 3 — Mesure live

*[Laisser tourner quelques secondes]*

Une ligne par seconde, exactement. La LED LD2 clignote au même rythme — c'est le heartbeat du TIM6. Temperature, humidité, pression défilent.

### Étape 4 — Variation

*[Souffler sur le shield ou approcher la main]*

Je souffle sur le shield. L'humidité remonte, la température aussi légèrement. Le HTS221 réagit en quelques secondes. La pression reste stable — c'est le LPS22HH, elle ne varie pas au souffle.

### Étape 5 — Changer de ville

*[Appuyer sur le bouton bleu B1]*

EXTI13 se déclenche, `g_ask_city` passe à 1, `g_paused` à 1. La télémétrie s'arrête net.

*[Taper `Lyon`, puis Entrée]*

Je tape `Lyon`, je valide. La mesure repart immédiatement avec `Ville=Lyon`.

*[Transition → slide 6]*

---

## Raphael MOSTACCI — Slide 06/06 · Conclusion (~3 min)

*[Avancer sur slide 6 — Bilan et perspectives]*

En guise de bilan, voici les exigences du sujet et ce qu'on a couvert.

| Exigence | Couverture |
|---|---|
| GPIO | LEDs LD2, L0, L1, L2, L3 + bouton B1 ✅ |
| TIMER | TIM6 en mode interruption ✅ |
| ADC | Retiré — optionnel, simplifié pour fiabiliser ℹ️ |
| UART | `printf` retargeté + saisie interactive de la ville ✅ |
| I2C | 3 capteurs IKS01A3 : HTS221, LPS22HH, STTS751 ✅ |
| ≥ 2 IRQ | 3 IRQ actives — TIM6 + EXTI13 + USART2 RX ✅ |
| ≥ 2 capteurs | 3 capteurs utilisés ✅ |

Sur l'ADC : on l'avait prévu pour lire un potentiomètre comme seuil d'alarme. On l'a retiré en cours de projet pour avoir un code propre et stable sur les fonctions essentielles. Ce n'est pas une régression.

### Pistes d'amélioration

- Affichage local sur MAX7219 via SPI, pour se passer du PC
- Lecture ADC en DMA + journalisation en EEPROM interne
- Calculs dérivés : point de rosée, pression ramenée au niveau de la mer
- Transport sans fil : LoRa ou BLE

---

Le projet est fonctionnel, il couvre toutes les exigences essentielles du sujet et vient d'être démontré en live. Le code source est disponible avec un README complet.

**Merci pour votre attention. On est disponibles pour vos questions.**

*[Rester sur le slide 6 pendant les questions]*

---

## Conseils pour le jour J

### Timing

| Phase | Heure | Durée |
|---|---|---|
| Baptiste — slides 1 + 2 | 0:00 → 7:00 | 7 min |
| Mathias — slides 3 + 4 | 7:00 → 15:00 | 8 min |
| Raphael — slides 5 + 6 | 15:00 → 20:00 | 5 min |

> Si la démo prend du retard, couper l'**étape 4** (souffle sur le shield).

### Anticipation des questions jury

| Question probable | Qui répond |
|---|---|
| "Pourquoi pas l'ADC ?" | Raphael (déjà abordé slide 6) |
| "Comment fonctionne le debounce ?" | Mathias |
| "Pourquoi `volatile` ?" | Mathias |
| "Quels capteurs du shield ?" | Baptiste |
| "Comment vous avez configuré TIM6 ?" | Baptiste (prescaler 31999, period 999 @ 32 MHz) |
| "Pourquoi trois IRQ ?" | Mathias |

### Matériel à avoir le jour J

- [ ] Carte Nucleo-L152RE flashée avec le dernier build
- [ ] Shield X-NUCLEO-IKS01A3 emboîté correctement
- [ ] Câble USB-A / micro-USB
- [ ] PC avec terminal série (PuTTY / TeraTerm / CoolTerm) — **115 200 bauds · 8N1**
- [ ] Savoir retrouver le bon COM port rapidement (Gestionnaire de périphériques)
