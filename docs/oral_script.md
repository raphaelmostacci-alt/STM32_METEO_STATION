# Script oral — Station Météo STM32
**Baptiste · Mathias · Raphael — 20 min max**

---

## Répartition

| Qui | Quoi | Durée |
|---|---|---|
| **Baptiste** | Slide 1 — Application | ~3 min |
| **Baptiste** | Slide 2 — Solution technique | ~4 min |
| **Mathias** | Slide 3 — Défis | ~4 min |
| **Mathias** | Slide 4 — Résolution | ~4 min |
| **Raphael** | Slide 5 — Démo | ~4 min |
| **Raphael** | Slide 6 — Conclusion | ~1 min |

---

---

# BAPTISTE — Slide 1 · Application

Bonjour, nous c'est Baptiste, Mathias et Raphael, en promo 2026 à l'ISEN.

Notre projet c'est une **station météo embarquée sur STM32**.

L'idée c'est simple : on branche le shield IKS01A3 sur la Nucleo-L152RE,
et chaque seconde ça mesure la température, l'humidité et la pression,
et ça les envoie en direct sur un PC via le port série.

On a aussi une interaction : au démarrage on tape une ville dans le terminal,
et avec le bouton bleu on peut en changer à tout moment.

---

# BAPTISTE — Slide 2 · Solution technique

Pour faire ça, on a utilisé ces périphériques :

- **I2C** pour lire les trois capteurs du shield — HTS221, LPS22HH, STTS751
- **UART** pour afficher les mesures sur le PC à 115 200 bauds
- **TIM6** en interruption — c'est lui qui cadence à 1 Hz
- **EXTI13** en interruption — c'est le bouton bleu pour changer de ville
- **GPIO** pour les LEDs de statut

Côté code, on a tout mis dans un module séparé `weather.c` pour garder le `main.c` propre.
L'archi est entièrement événementielle : pas de `HAL_Delay` dans la boucle principale.

---

# MATHIAS — Slide 3 · Défis

On a eu quatre problèmes principaux.

**1 — Cadencer à 1 Hz proprement**
Le réflexe c'est `HAL_Delay(1000)` mais ça bloque tout.
On ne peut rien faire d'autre pendant ce temps-là.
Il fallait un vrai timer hardware.

**2 — Faire coexister plusieurs interruptions**
Le sujet imposait 2 IRQ minimum, on en a 3 : TIM6, EXTI13, USART2 RX.
Elles ne doivent pas se bloquer entre elles.

**3 — Gérer les erreurs capteurs**
Trois capteurs sur le même bus I2C.
Si l'un plante à l'init, il faut quand même continuer avec les autres.

**4 — Saisie UART sans bloquer les mesures**
Pendant que l'utilisateur tape la ville, TIM6 continue à tirer.
Si on laisse passer les mesures, le terminal devient illisible.

---

# MATHIAS — Slide 4 · Résolution

Le principe qu'on a adopté c'est le **pattern flag-driven**.

Les interruptions ne font qu'une chose : lever un flag.
C'est la boucle principale qui lit ce flag et fait le travail.

```
TIM6 IRQ       →  g_sample_flag = 1
EXTI13 IRQ     →  g_ask_city = 1
USART2 RX IRQ  →  g_city_ready = 1  (quand Entrée reçu)
```

**Anti-rebond bouton :**
Dans le callback EXTI13, on compare `HAL_GetTick()` au dernier tick enregistré.
Si c'est moins de 50 ms, on ignore — c'est un rebond.

**Pause pendant la saisie :**
On a un flag `g_paused`. Quand il est à 1, la boucle ignore les ticks TIM6.
Dès que la ville est validée, `g_paused` repasse à 0 et c'est reparti.

**Robustesse capteurs :**
`Weather_InitSensors()` essaie chaque capteur séparément.
Si l'un échoue, il est juste marqué indisponible, les autres continuent.

---

# RAPHAEL — Slide 5 · Démo

*[Ouvrir le terminal série — bonne vitesse COM, 115 200 bauds 8N1]*

Je vais vous faire la démo en 5 étapes.

**1 — Reset**
*[Appuyer sur le bouton reset noir]*
La bannière de boot s'affiche, les 3 capteurs sont OK.
Le terminal demande : `Entrez la ville :`

**2 — Saisie de la ville**
*[Taper `Marseille` + Entrée]*
Les mesures démarrent immédiatement.

**3 — Mesure live**
*[Laisser tourner]*
Une ligne par seconde, la LED LD2 clignote en rythme — c'est le heartbeat du TIM6.

**4 — Variation**
*[Souffler sur le shield]*
L'humidité et la température remontent. La pression reste stable.

**5 — Changer de ville**
*[Appuyer sur B1]*
La télémétrie s'arrête, on ressaisit une ville.
*[Taper `Lyon` + Entrée]*
Ça repart avec `Ville=Lyon`.

---

# RAPHAEL — Slide 6 · Conclusion

En résumé, on a couvert tout ce que demandait le sujet :

- GPIO, TIMER, UART, I2C ✅
- 2 IRQ minimum → on en a **3** ✅
- 2 capteurs minimum → on en a **3** ✅
- L'ADC a été retiré pour simplifier et fiabiliser — c'est un choix assumé.

Le code est sur GitHub, le README explique comment rebuilder et utiliser.

Merci, on est dispo pour vos questions.

---

---

## Memo questions jury

| Question | Qui répond |
|---|---|
| Pourquoi pas l'ADC ? | Raphael |
| C'est quoi `volatile` ? | Mathias — *"évite que le compilo optimise et ignore la variable modifiée en IRQ"* |
| Comment marche le debounce ? | Mathias — *"on compare HAL_GetTick(), si < 50 ms on ignore"* |
| Comment TIM6 est configuré ? | Baptiste — *"prescaler 31999, period 999, à 32 MHz ça donne 1 Hz"* |
| Pourquoi 3 IRQ et pas 2 ? | Mathias — *"UART RX en IRQ évite de rater des caractères pendant les mesures"* |

## Checklist le jour J

- [ ] Carte flashée avec le dernier build
- [ ] Shield bien emboîté
- [ ] Câble USB branché
- [ ] Terminal série ouvert sur le bon COM — **115 200 · 8N1**
- [ ] Savoir retrouver le COM port (Gestionnaire de périphériques)
