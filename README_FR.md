README - Configuration écran (FR)

Contexte
--------
Ce projet utilise un écran TFT 4.3" (JC4827B043N) en mode RGB parallèle 24 bits (R0..R7, G0..G7, B0..B7) piloté par le contrôleur ILI6485.

Résumé important extrait du PDF "4.3” 480X272-TN bare screen specs.pdf" + ILI6485
--------------------------------------------------------------------------------
- Connecteur : FFC 40 broches (broche 1 en haut vue côté composant).
- Résolution : 480 x 272
- Interface : RGB parallèle 24 bits (8 bits / couleur)
- PCLK typique : 9 MHz (entre 8 et 12 MHz)
- HSYNC/VSYNC : polarity = active low (négatif)
- DE (DEN) : active high
- DISP : active high (contrôle écran allumé / éteint)
- LEDA/LEDK : rétroéclairage, utiliser un driver constant-current (Vf total typ ~15V pour la chaîne LED)

Correspondance FFC (extrait du PDF)
----------------------------------
FFC pin -> Signal (vue composant)
1  -> LEDK (cathode rétroéclairage)
2  -> LEDA (anode rétroéclairage)
3  -> GND
4  -> VCC (3.3V)
5  -> R0
6  -> R1
7  -> R2
8  -> R3
9  -> R4
10 -> R5
11 -> R6
12 -> R7
13 -> G0
14 -> G1
15 -> G2
16 -> G3
17 -> G4
18 -> G5
19 -> G6
20 -> G7
21 -> B0
22 -> B1
23 -> B2
24 -> B3
25 -> B4
26 -> B5
27 -> B6
28 -> B7
29 -> GND
30 -> CLK (PCLK)
31 -> DISP
32 -> HSYNC
33 -> VSYNC
34 -> DEN (DE)
35 -> NC
36 -> GND
37 -> XR (NC)
38 -> YD (NC)
39 -> XL (NC)
40 -> YU (NC)

Ce que j'ai modifié dans le dépôt
-------------------------------
- `src/display_config.h` : fichier de configuration des timings et placeholders pour les GPIO, avec commentaires en français et valeurs de timing typiques (PCLK=9MHz, porches, polarités).

Étapes suivantes recommandées
-----------------------------
1. Confirme quel GPIO ESP32-S3 est câblé à chaque signal FFC (R0..R7, G0..G7, B0..B7, HSYNC, VSYNC, DE, PCLK, DISP, LEDA/LEDK). Si tu utilises un adaptateur, indique sa correspondance.
2. Mettre à jour `src/display_config.h` : remplacer les `-1` par les numéros GPIO réels.
3. Compiler/téléverser la version LVGL minimale (le code utilise l'API esp_lcd pour RGB). Surveille le port série (115200) pour messages d'initialisation.
4. Si écran blanc : vérifier la tension LED (rétroéclairage), que VCC 3.3V est stable, et que DISP est mis à HIGH après power-on et reset (attendre 120 ms après VCC selon PDF).

Notes de debugging
------------------
- Si tu as encore des "Guru Meditation Errors" : partage le log série complet. Indique aussi l'assignation GPIO que tu as utilisée. Les erreurs StoreProhibited sont souvent causées par des indicateurs mémoire/PSRAM mal configurés ou par des accès à des pointeurs non initialisés.
- Si tu veux, je peux appliquer une carte de mapping exemple (ex : FFC pin 5 -> GPIO 16, etc.) pour te fournir un `display_config.h` prêt à l'emploi — mais il faut confirmer le câblage physique.


Si tu confirmes ton câblage FFC->GPIO, je mets à jour les fichiers et je lance une compilation de test.
