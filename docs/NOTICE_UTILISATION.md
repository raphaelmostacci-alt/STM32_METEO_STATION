# Notice d'utilisation — STM32 Meteo Station

Station meteo embarquee sur **NUCLEO-L152RE** + shield **X-NUCLEO-IKS01A3**.
Mesure en continu **temperature**, **humidite** et **pression** et publie les valeurs sur le PC via la liaison serie ST-LINK.

---

## 1. Materiel necessaire

- Carte **NUCLEO-L152RE** (STM32L152RET6).
- Shield **X-NUCLEO-IKS01A3** correctement enfiche sur les connecteurs Arduino de la Nucleo.
- Cable **USB type A vers mini-USB** (cote ST-LINK de la Nucleo, pas le port USER).
- PC sous Windows / Linux / macOS avec :
  - **STM32CubeIDE** (1.13 ou plus recent),
  - les drivers **ST-LINK** (installes automatiquement avec CubeIDE sur Windows),
  - un emulateur de terminal : **TeraTerm** (fourni dans `teraterm-4.103.exe`), PuTTY, le terminal integre de CubeIDE, ou `screen` / `minicom` sous Linux.

---

## 2. Cablage et verifications

1. Mettre la Nucleo **hors tension** (debrancher l'USB).
2. Enficher le shield IKS01A3 sur les connecteurs Arduino de la Nucleo en respectant l'orientation (les broches `+5V`, `+3V3`, `GND` doivent coincider).
3. Verifier qu'aucun cavalier `JP6` n'a ete deplace sur la Nucleo (`IDD` doit etre ferme pour alimenter le MCU).
4. Brancher le cable USB cote **ST-LINK** (port marque `CN1` sur la Nucleo).
5. La LED rouge `LD3` (alimentation) doit s'allumer fixe. La LED `LD1` (com ST-LINK) clignote rouge/vert pendant l'enumeration USB.

> **Important :** le shield IKS01A3 utilise I2C1 sur PB8/PB9 ; ces broches sont deja routees par le shield, aucun fil supplementaire n'est necessaire.

---

## 3. Premiere mise en route (compilation et flash)

> **Important** : sur cette machine, **STM32CubeMX** et **STM32CubeIDE** sont deux applications **separees**.
> - **CubeMX** = generateur de code (lit le `.ioc`, ecrit les drivers HAL + BSP + fichiers de projet Eclipse).
> - **CubeIDE** = environnement de developpement (compile, flashe, debug).
>
> Le workflow est : on genere d'abord dans CubeMX, **puis** on importe le projet genere dans CubeIDE.

### 3.1 Generer le code source avec STM32CubeMX (standalone)

1. Lancer **STM32CubeMX** (icone separee, pas celle de CubeIDE).
2. `File → Load Project...` et selectionner
   `C:\Users\bapti\Documents\STM32\STM32_METEO_STATION\STM32_METEO_STATION.ioc`.
3. A la premiere ouverture, CubeMX peut demander d'installer :
   - **STM32Cube FW_L1 V1.10.5** ou plus recent (firmware package STM32L1),
   - **X-CUBE-MEMS1 v11.1.0** (pack capteurs IKS01A3).

   Aller dans `Help → Manage embedded software packages`, cocher les paquets manquants et **Install Now**. Patienter quelques minutes.
4. Une fois le `.ioc` charge, ouvrir l'onglet **`Project Manager`** (en haut) :
   - **Project Name** : `STM32_METEO_STATION` (deja rempli).
   - **Project Location** : `C:\Users\bapti\Documents\STM32` (le parent du dossier deja existant).
   - **Application Structure** : `Default`.
   - **Toolchain / IDE** : **`STM32CubeIDE`** (important : pas Makefile, pas EWARM).
   - Cocher **`Generate Under Root`**.
   - Onglet **Code Generator** : cocher **`Keep User Code when re-generating`** (deja regle par le `.ioc`).
5. Onglet **`Pinout & Configuration`** → categorie **`Software Packs`** → verifier que `STMicroelectronics.X-CUBE-MEMS1` est coche avec **`Board Extension IKS01A3`** active, instance BSP sur **I2C1**.
6. Cliquer sur **`GENERATE CODE`** (gros bouton en haut a droite).
7. CubeMX ecrit dans le dossier `STM32_METEO_STATION/` :
   - `.project` et `.cproject` (les fichiers de projet Eclipse/CubeIDE),
   - `Drivers/STM32L1xx_HAL_Driver/` et `Drivers/CMSIS/`,
   - `Middlewares/ST/STM32_BSP_IKS01A3/`,
   - `Core/Src/stm32l1xx_hal_msp.c`,
   - `STM32L152RETX_FLASH.ld` (script editeur de liens),
   - `startup_stm32l152retx.s` (fichier de demarrage).

   Le code utilisateur deja present (`main.c`, `weather.c`, `weather.h`, `stm32l1xx_it.c`) est conserve grace a `KeepUserCode=true`.
8. Quand CubeMX affiche `The Code is successfully generated`, fermer la boite de dialogue (ne pas cliquer "Open Project" si proposee, on va le faire depuis CubeIDE).

### 3.2 Importer le projet dans STM32CubeIDE

1. Lancer **STM32CubeIDE** (l'autre application).
2. Choisir ou creer un **workspace** (ex. `C:\Users\bapti\STM32CubeIDE\workspace_meteo`). N'importe quel emplacement convient, **sauf** a l'interieur du dossier projet lui-meme.
3. `File → Open Projects from File System...`
4. Cliquer sur `Directory...` et selectionner le dossier
   `C:\Users\bapti\Documents\STM32\STM32_METEO_STATION`.
5. CubeIDE detecte le `.project` genere a l'etape 3.1. Cocher la ligne correspondante.
6. Cliquer **Finish**. Le projet apparait dans le `Project Explorer` avec **l'icone bleue C** et le marteau **Build** devient actif.

> Si CubeIDE ne voit pas le projet (la ligne reste vide) : c'est que `.project` n'a pas ete genere. Retourner dans CubeMX, verifier que **Toolchain / IDE = STM32CubeIDE** dans le Project Manager et refaire `GENERATE CODE`.

### 3.3 Compiler

1. Selectionner `STM32_METEO_STATION` dans le `Project Explorer`.
2. `Project → Build All` (raccourci `Ctrl+B`) ou clic sur l'icone marteau.
3. La console doit afficher `Build Finished. 0 errors`. Quelques warnings BSP sont normaux.
4. Si la compilation echoue avec `iks01a3_env_sensors.h: No such file or directory` : retourner dans **CubeMX**, verifier que `Board Extension IKS01A3` est coche, refaire `GENERATE CODE`, puis dans CubeIDE faire `Project → Clean...` puis `Build All`.

### 3.4 Flasher la carte

1. S'assurer que la Nucleo est branchee (port ST-LINK, voir section 2).
2. Clic droit sur le projet → `Run As → STM32 C/C++ Application`.
3. A la premiere execution, CubeIDE demande de creer une `Debug Configuration` : accepter les valeurs par defaut → **OK**.
4. CubeIDE efface la Flash, programme le binaire puis reset le MCU.
5. La carte demarre immediatement apres le flash.

### 3.5 Re-generer apres un changement de `.ioc`

Si tu modifies la configuration des peripheriques :

1. Editer le `.ioc` **dans CubeMX standalone**.
2. `Project → Generate Code` (ou `GENERATE CODE`).
3. Revenir dans CubeIDE : **clic droit sur le projet → Refresh** (`F5`).
4. **Build All**.

Les blocs `/* USER CODE BEGIN ... */ ... /* USER CODE END ... */` sont preserves.

---

## 4. Connexion au terminal serie

1. Ouvrir le **Gestionnaire de peripheriques** Windows (`Win+X → Gestionnaire de peripheriques`) ou `ls /dev/ttyACM*` sous Linux.
2. Reperer le port serie `STMicroelectronics STLink Virtual COM Port (COMx)`.
3. Lancer **TeraTerm** (ou PuTTY) :
   - **Port** : le COMx reperer ci-dessus.
   - **Baud rate** : `115200`.
   - **Data bits** : `8`.
   - **Parity** : `None`.
   - **Stop bits** : `1`.
   - **Flow control** : `None`.
4. Cliquer sur **OK**. Le terminal est pret.

### 4.1 Sous TeraTerm

`Setup → Serial port...` puis renseigner les parametres ci-dessus.
Pour rendre le reglage permanent : `Setup → Save setup → teraterm.ini`.

### 4.2 Sous PuTTY

`Connection type : Serial`, `Serial line : COMx`, `Speed : 115200`, puis **Open**.

### 4.3 Dans STM32CubeIDE

`Window → Show View → Terminal`, cliquer sur l'icone `Open a Terminal`, choisir `Serial Terminal`, renseigner les memes parametres.

---

## 5. Sequence de demarrage attendue

Apres avoir flashe et ouvert le terminal, appuyer sur le **bouton noir RESET** (B2) de la Nucleo. Le terminal doit afficher :

```
=== STM32 METEO STATION boot ===
Board: NUCLEO-L152RE + X-NUCLEO-IKS01A3
UART2 115200 8N1, TIM6 sampling 1 Hz, EXTI13 mode toggle.
Sensors HTS221 + LPS22HH + STTS751 ready.
METEO T=23.45 C RH=48.2 % P=1013.4 hPa ADC=2048 mode=LIVE
METEO T=23.46 C RH=48.1 % P=1013.4 hPa ADC=2050 mode=LIVE
...
```

La LED verte **LD2** doit clignoter a **1 Hz** (battement de coeur).

> Si la banniere apparait mais affiche `[WARN] One or more IKS01A3 sensors failed to init.`, verifier que le shield est correctement enfiche et que les broches PB8/PB9 ne sont pas detournees par un cavalier.

---

## 6. Utilisation au quotidien

### 6.1 Mode LIVE (par defaut)

- Une mesure est prise toutes les **1 seconde** sur les trois capteurs.
- Chaque mesure est imprimee sur le terminal au format :
  ```
  METEO T=<temp> C RH=<humid> % P=<press> hPa ADC=<adc> mode=LIVE
  ```
- La LED verte LD2 toggle a chaque mesure (heartbeat).
- La LED **L0** (PB1) est allumee si la derniere mesure est valide.
- La LED **L1** (PB2) s'allume des qu'un capteur renvoie une erreur.

### 6.2 Mode FROZEN

- Appuyer sur le **bouton bleu B1** (PC13) de la Nucleo.
- Le terminal affiche `[BTN ] mode -> FROZEN`.
- La LED **L2** (PB10) s'allume.
- Les capteurs ne sont **plus interroges** et la telemetrie est suspendue.
- L'ADC continue d'etre lu (le seuil reste reactif) et le heartbeat LD2 continue.
- Re-appuyer sur B1 pour revenir en mode LIVE.

### 6.3 Reglage du seuil d'alarme

- La valeur du **potentiometre** (broche PA0, lue par l'ADC1) sert directement de seuil.
- Plage : 0 a 4095 (12 bits).
- Tant que `ADC > seuil`, la LED **L3** (PB11) est allumee.
- Tourner le potentiometre permet de declencher l'alarme a la demande pour la demo.

### 6.4 Signification de chaque LED

| LED | Broche | Etat allume = ...                            |
|-----|--------|----------------------------------------------|
| LD2 | PA5    | Heartbeat (toggle a 1 Hz)                    |
| L0  | PB1    | Derniere mesure valide                       |
| L1  | PB2    | Erreur capteur detectee                      |
| L2  | PB10   | Mode FROZEN actif                            |
| L3  | PB11   | Alarme : valeur ADC depasse le seuil         |

---

## 7. Scenario de demonstration (5 minutes)

1. **Reset** la carte. Verifier la banniere et `Sensors HTS221 + LPS22HH + STTS751 ready.`.
2. Montrer le **heartbeat LD2** a 1 Hz.
3. **Souffler** doucement sur le shield : humidite et temperature montent.
4. **Poser le doigt** sur le capteur STTS751 : temperature monte de 1 °C en quelques secondes.
5. **Tourner le potentiometre** sous la valeur ADC affichee → **L3** s'allume.
6. Appuyer sur **B1** → passage en FROZEN, **L2** s'allume, plus de lignes METEO.
7. Re-appuyer sur **B1** → retour en LIVE.
8. *(Optionnel)* Debrancher le shield brievement → `[SENSOR_ERR]` apparait, **L1** s'allume. Rebrancher et reset.

---

## 8. Depannage

| Symptome                                              | Cause probable                              | Solution                                                                                                  |
|-------------------------------------------------------|----------------------------------------------|------------------------------------------------------------------------------------------------------------|
| Aucun port COM dans le Gestionnaire de peripheriques | Drivers ST-LINK absents                     | Reinstaller CubeIDE (drivers inclus) ou telecharger `ST-LINK USB driver` sur st.com.                       |
| Le terminal reste vide apres reset                   | Mauvais baud rate                           | Verifier que le terminal est en **115200 8N1**, pas en 9600.                                              |
| Terminal affiche des caracteres bizarres             | Mauvaise vitesse / parite                   | Verifier 115200 8N1 sans parite ni flow control.                                                          |
| `[WARN] sensors failed to init.`                      | Shield mal enfiche / I2C non routee         | Repositionner le shield, verifier les pins PB8/PB9, refaire `Generate Code` avec X-CUBE-MEMS1 actif.      |
| LD2 ne clignote pas                                   | TIM6 non demarre ou IRQ non activee         | Verifier que `HAL_TIM_Base_Start_IT(&htim6)` reussit et que `TIM6_IRQHandler` est present dans `it.c`.    |
| Le bouton B1 declenche plusieurs fois par appui      | Anti-rebond logiciel insuffisant            | Augmenter `BUTTON_DEBOUNCE_MS` (50 ms par defaut) dans `main.c`.                                          |
| Compilation : `iks01a3_env_sensors.h` introuvable    | Pack X-CUBE-MEMS1 non installe              | Installer le pack dans CubeMX, cocher `Board Extension IKS01A3`, refaire `Generate Code`.                 |
| Icone du projet generique (dossier) au lieu de bleu C, marteau **Build grise** | Le code n'a pas encore ete genere par CubeMX, donc pas de `.project`/`.cproject` | Ouvrir le `.ioc` dans **CubeMX standalone**, verifier `Toolchain/IDE = STM32CubeIDE`, faire `GENERATE CODE`, puis dans CubeIDE `File → Open Projects from File System...` (voir section 3.1 et 3.2). |
| `.project` introuvable dans le dossier apres `Generate Code` | Mauvais toolchain selectionne dans CubeMX | Dans CubeMX, onglet `Project Manager`, mettre `Toolchain / IDE = STM32CubeIDE` puis refaire `GENERATE CODE`. |
| L3 allumee en permanence                              | Potentiometre tourne au max                 | Tourner le potentiometre vers le minimum pour baisser le seuil.                                            |
| Flash echoue : "Target not detected"                 | Cable USB defectueux / cote USER branche    | Brancher le cable cote **ST-LINK** (CN1), pas cote USER (CN15).                                           |

---

## 9. Arret et redemarrage

- **Arret normal** : debrancher l'USB. La Nucleo s'eteint, le terminal indique la perte de connexion.
- **Redemarrage** : appuyer sur le **bouton noir RESET** (B2) en haut de la Nucleo. Le firmware redemarre, la banniere s'affiche a nouveau.
- **Re-flashage** : relancer `Run As → STM32 C/C++ Application` dans CubeIDE.

---

## 10. Pour aller plus loin

- Voir [README.md](../README.md) pour la configuration CubeMX detaillee.
- Voir [docs/PRESENTATION_OUTLINE.md](PRESENTATION_OUTLINE.md) pour le plan de presentation.
- Voir [docs/REQUIREMENTS_CHECKLIST.md](REQUIREMENTS_CHECKLIST.md) pour la couverture du cahier des charges.
- Voir `BOARD.md` (a la racine du depot) pour le mapping complet des broches.

---

*Document redige le 2026-05-27. Projet pedagogique P23MRS, ISEN.*
