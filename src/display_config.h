#pragma once

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include "esp_lcd_panel_vendor.h"

/*
	Configuration basée sur la fiche technique fournie (JC4827B043N) et
	les timings typiques du driver ILI6485 pour 480x272 en RGB 24-bit.

	IMPORTANT : ce fichier fournit des constantes et des repères. Les
	numéros de GPIO ci-dessous sont des placeholders d'exemple. Tu dois
	remplacer EXAMPLE_PIN_NUM_* par les GPIO réels auxquels tu as connecté
	les broches du connecteur FFC (voir README_FR.md pour la correspondance
	FFC-pin -> signal).

	Polarités et timings :
	- PCLK (pixel clock) typique : 9 MHz
	- HSYNC/VSYNC : active low (polarity négative)
	- DE (DEN) : active high
	- DISP : active high (permet d'allumer l'affichage)
*/

// Résolution
#define EXAMPLE_LCD_H_RES              480
#define EXAMPLE_LCD_V_RES              272

// Fréquence pixel clock recommandée (d'après ta fiche)
// Réduite à 4 MHz pour économiser CPU/RAM
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (4 * 1000 * 1000) // 4 MHz (réduit de 8 MHz)

// Niveau d'activation du rétroéclairage (à remplacer si nécessaire)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL 0

// Broche de rétroéclairage (GPIO de l'ESP32 connecté au driver LEDA)
// Sur la board Sunton ESP32-S3 4827S043C : GPIO2 (TFT_BL)
#define EXAMPLE_PIN_NUM_BK_LIGHT       2

// === Signaux de synchronisation (remplacer par les GPIO réels) ===
// Les valeurs ci-dessous sont des exemples : adapte selon ton câblage.
// Signaux de synchronisation (mapping Sunton ESP32-S3 4827S043C)
#define EXAMPLE_PIN_NUM_HSYNC         39 // HSYNC (FFC pin 32)
#define EXAMPLE_PIN_NUM_VSYNC         41 // VSYNC (FFC pin 33)
#define EXAMPLE_PIN_NUM_DE            40 // DEN / DE (FFC pin 34) (partagé avec DISP on this board)
#define EXAMPLE_PIN_NUM_PCLK          42 // PCLK / CLK (FFC pin 30)

// === Broches de données (R0..R7, G0..G7, B0..B7) ===
// Remplace EXAMPLE_PIN_NUM_DATAi par le GPIO connecté à la broche FFC correspondante.
// Broches de données (RGB565 mapping validé pour Sunton board)
// Ordre interne : DATA0..DATA15 -> B0,B1,B2,B3,B4, G0..G5, R0..R4
// (conçu pour correspondre au panel 16-bit RGB565 wiring)
#define EXAMPLE_PIN_NUM_DATA0         8   // B0  (FFC pin 21)
#define EXAMPLE_PIN_NUM_DATA1         3   // B1  (FFC pin 22)
#define EXAMPLE_PIN_NUM_DATA2         46  // B2  (FFC pin 23)
#define EXAMPLE_PIN_NUM_DATA3         9   // B3  (FFC pin 24)
#define EXAMPLE_PIN_NUM_DATA4         1   // B4  (FFC pin 25)

#define EXAMPLE_PIN_NUM_DATA5         5   // G0  (FFC pin 13)
#define EXAMPLE_PIN_NUM_DATA6         6   // G1  (FFC pin 14)
#define EXAMPLE_PIN_NUM_DATA7         7   // G2  (FFC pin 15)
#define EXAMPLE_PIN_NUM_DATA8         15  // G3  (FFC pin 16)
#define EXAMPLE_PIN_NUM_DATA9         16  // G4  (FFC pin 17)
#define EXAMPLE_PIN_NUM_DATA10        4   // G5  (FFC pin 18)

#define EXAMPLE_PIN_NUM_DATA11        45  // R0  (FFC pin 5)
#define EXAMPLE_PIN_NUM_DATA12        48  // R1  (FFC pin 6)
#define EXAMPLE_PIN_NUM_DATA13        47  // R2  (FFC pin 7)
#define EXAMPLE_PIN_NUM_DATA14        21  // R3  (FFC pin 8)
#define EXAMPLE_PIN_NUM_DATA15        14  // R4  (FFC pin 9)

// Pin DISP (display on/off) si disponible (connecter à la broche 31 FFC)
#define EXAMPLE_LCD_DISP_ON_OFF_PIN   40

// Timings (valeurs typiques extraites / dérivées du PDF et ILI6485)
// Tous les timings s'expriment en nombre de cycles PCLK (DCLK) pour l'horizontal
// et en lignes pour le vertical.
#define EXAMPLE_LCD_HSYNC_BACK_PORCH   43
#define EXAMPLE_LCD_HSYNC_FRONT_PORCH  8
#define EXAMPLE_LCD_HSYNC_PULSE_WIDTH  4

#define EXAMPLE_LCD_VSYNC_BACK_PORCH   12
#define EXAMPLE_LCD_VSYNC_FRONT_PORCH  8
#define EXAMPLE_LCD_VSYNC_PULSE_WIDTH  4

// Polarités (configurer dans esp_lcd timing flags si nécessaire)
// HSYNC/VSYNC : active low (false) ; DE : active high (true) ; PCLK : latch on falling edge
// L'API esp_lcd_rgb_panel_config_t::timings.flags a des champs à configurer dans main.c
