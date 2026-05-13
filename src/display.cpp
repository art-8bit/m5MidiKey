#include "display.h"
#include "app_state.h"
#include "config.h"
#include "types.h"
#include "ble_midi_handler.h"    // ← Для получения доступа к BLE_PWR_TABLE
#include "keyboard_map.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  МЕТКИ ПУНКТОВ МЕНЮ
// ═══════════════════════════════════════════════════════════════════════════
static constexpr const char* MENU_LABELS[static_cast<int>(MenuItemID::COUNT)] = {
    "Octave",
    "Velocity",
    "BLE Power",
    "MIDI Channel",
    "Keyboard Source",
    "Matrix Keyboard",
    "\x10 Play Mode \x11",
};

// ═══════════════════════════════════════════════════════════════════════════
//  СТАТУС БАР
// ═══════════════════════════════════════════════════════════════════════════
void drawStatusBar() {
    g_canvas.fillRect(0, 0, SCREEN_W, STATUS_H, C_STATUS_BG);
    g_canvas.drawFastHLine(0, STATUS_H - 1, SCREEN_W, C_ACCENT);

    g_canvas.setTextSize(1);

    // Название приложения
    g_canvas.setTextColor(C_ACCENT, C_STATUS_BG);
    g_canvas.setCursor(5, 7);
    g_canvas.print("Cardputer MIDI");

    // Октава
    g_canvas.setTextColor(C_YELLOW, C_STATUS_BG);
    g_canvas.setCursor(SCREEN_W / 2 - 22, 7);
    g_canvas.printf("Oct %+d", g_octave);

    // Батарея
    int batX = SCREEN_W - 62;
    if (g_cached_bat < 0) {
        g_canvas.setTextColor(C_GRAY, C_STATUS_BG);
        g_canvas.setCursor(batX, 7);
        g_canvas.print("BAT:--");
    } else {
        uint16_t batColor = (g_cached_bat > 30) ? C_GREEN
                          : (g_cached_bat > 15) ? C_YELLOW
                          :                      C_RED;
        g_canvas.setTextColor(batColor, C_STATUS_BG);
        g_canvas.setCursor(batX, 7);
        g_canvas.printf("BAT:%d%%", g_cached_bat);
    }

    // Статус BLE (кружок)
    int cx = SCREEN_W - 7;
    int cy = STATUS_H / 2 - 1;
    if (g_bleConnected) {
        g_canvas.fillCircle(cx, cy, 4, C_GREEN);
        g_canvas.setTextColor(C_BG, C_GREEN);
        g_canvas.setCursor(cx - 2, cy - 3);
        g_canvas.print("B");
    } else {
        g_canvas.drawCircle(cx, cy, 4, C_GRAY);
        g_canvas.fillCircle(cx, cy, 3, C_STATUS_BG);
        g_canvas.setTextColor(C_GRAY, C_STATUS_BG);
        g_canvas.setCursor(cx - 2, cy - 3);
        g_canvas.print("B");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  МЕНЮ
// ═══════════════════════════════════════════════════════════════════════════
void drawMenu() {
    g_canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, C_BG);

    static constexpr int ITEM_H  = 16;
    static constexpr int MENU_W  = 218;
    static constexpr int MENU_X  = (SCREEN_W - MENU_W) / 2;
    static constexpr int START_Y = CONTENT_Y + 2;
    static constexpr int VAL_X   = MENU_X + 128;

    g_canvas.setTextSize(1);

    int menu_count = static_cast<int>(MenuItemID::COUNT);
    for (int i = 0; i < menu_count; i++) {
        int y = START_Y + i * ITEM_H;
        bool sel = (i == g_menu_sel);

        // Фон выбранного пункта
        if (sel) {
            g_canvas.fillRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 4, C_SEL_BG);
            g_canvas.drawRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 4, C_ACCENT);
            g_canvas.fillTriangle(
                MENU_X - 10, y + ITEM_H/2 - 2,
                MENU_X - 4,  y + ITEM_H/2,
                MENU_X - 10, y + ITEM_H/2 + 3,
                C_ACCENT
            );
        }

        // Текст пункта
        g_canvas.setTextColor(sel ? C_ACCENT : C_GRAY_LT);
        g_canvas.setCursor(MENU_X + 4, y + 3);
        if (i == static_cast<int>(MenuItemID::PLAY)) {
            g_canvas.setTextColor(sel ? C_ACCENT : C_ACCENT2);
        }
        g_canvas.print(MENU_LABELS[i]);

        // Значение параметра
        if (i != static_cast<int>(MenuItemID::PLAY)) {
            g_canvas.setTextColor(sel ? C_WHITE : C_YELLOW);
            g_canvas.setCursor(VAL_X, y + 3);
            
            MenuItemID item_id = static_cast<MenuItemID>(i);
            switch (item_id) {
                case MenuItemID::OCTAVE:
                    g_canvas.printf("<%+d>", g_octave);
                    break;
                case MenuItemID::VELOCITY:
                    g_canvas.printf("<%3d>", g_velocity);
                    break;
                case MenuItemID::BLE_POWER:
                    g_canvas.printf("<%s>", BLE_PWR_TABLE[g_ble_pwr].label);
                    break;
                case MenuItemID::MIDI_CHANNEL:
                    g_canvas.printf("<%2d>", g_midi_ch);
                    break;
                case MenuItemID::KBD_SOURCE:
                    g_canvas.setTextColor(sel ? C_WHITE : C_ACCENT2);
                    if (g_kbd_source == 0)
                        g_canvas.printf("<Built-in>");
                    else if (g_kbd_source == 1)
                        g_canvas.printf("<Matrix>");
                    else
                        g_canvas.printf("<Both>");
                    break;
                case MenuItemID::MATRIX_ENABLED:
                    g_canvas.setTextColor(sel ? C_WHITE : C_GREEN);
                    g_canvas.printf("<%s>", g_matrix_enabled ? "ON" : "OFF");
                    break;
                default:
                    break;
            }
        }
    }

    // Разделитель перед пунктом "Play"
    int divY = START_Y + (static_cast<int>(MenuItemID::PLAY) - 1) * ITEM_H + ITEM_H - 1;
    g_canvas.drawFastHLine(MENU_X, divY, MENU_W, C_GRAY_DK);

    // Подсказка внизу
    g_canvas.setTextColor(C_GRAY_DK);
    g_canvas.setCursor(3, SCREEN_H - 8);
    g_canvas.setTextSize(1);
    g_canvas.print("I/K:nav  J/L:val  Enter:play");
}

// ═══════════════════════════════════════════════════════════════════════════
//  РЕЖИМ ИГРЫ
// ═══════════════════════════════════════════════════════════════════════════
void drawPlay() {
    g_canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, C_BG);

    // Информация о источнике клавиатуры
    g_canvas.setTextSize(1);
    g_canvas.setTextColor(C_GRAY);
    g_canvas.setCursor(4, CONTENT_Y + 2);
    if (g_kbd_source == 0)
        g_canvas.print("Built-in kbd");
    else if (g_kbd_source == 1)
        g_canvas.print("Matrix kbd");
    else
        g_canvas.print("Both kbd");
    g_canvas.print("  BS=menu");

    // Коробка с информацией о текущей ноте
    int noteBoxY = CONTENT_Y + 13;
    int noteBoxH = 35;
    g_canvas.fillRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH, 6, C_STATUS_BG);
    g_canvas.drawRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH,
                           6, g_bleConnected ? C_ACCENT : C_GRAY_DK);

    if (g_last_midi_note >= 0) {
        g_canvas.setTextSize(2);
        g_canvas.setTextColor(C_ACCENT);
        int tw = g_last_note_label.length() * 12;
        int tx = (SCREEN_W - tw) / 2;
        g_canvas.setCursor(tx, noteBoxY + 6);
        g_canvas.print(g_last_note_label);

        g_canvas.setTextSize(1);
        g_canvas.setTextColor(C_GRAY);
        g_canvas.setCursor(10, noteBoxY + noteBoxH - 10);
        g_canvas.printf("MIDI:%d vel:%d", g_last_midi_note, g_velocity);
    } else {
        g_canvas.setTextSize(1);
        g_canvas.setTextColor(C_GRAY);
        g_canvas.setCursor(SCREEN_W / 2 - 35, noteBoxY + 12);
        g_canvas.print("play a note...");
    }

    // Визуализация встроенной клавиатуры (если активна)
    if (g_kbd_source == 0 || g_kbd_source == 2) {
        int pianoY = noteBoxY + noteBoxH + 2;
        int pianoH = SCREEN_H - pianoY - 1;
        float keyW = (float)SCREEN_W / WK_COUNT;

        // Белые клавиши
        for (int i = 0; i < WK_COUNT; i++) {
            bool pressed = g_key_state[WHITE_KEYS[i].key];
            int  x  = (int)(i * keyW);
            int  w  = (int)((i + 1) * keyW) - x;
            uint16_t fill = pressed ? C_PIANO_PR : C_PIANO_W;

            g_canvas.fillRect(x + 1, pianoY, w - 1, pianoH - 1, fill);
            g_canvas.drawRect(x, pianoY - 1, w + 1, pianoH + 1, C_GRAY_DK);

            if (pressed) {
                g_canvas.drawRect(x + 1, pianoY, w - 1, pianoH - 1, C_ACCENT);
            }
        }

        // Чёрные клавиши
        int bkW = (int)(keyW * 0.62f);
        int bkH = (pianoH * 6) / 10;

        for (int i = 0; i < BK_COUNT; i++) {
            bool pressed = g_key_state[BLACK_KEYS[i].key];
            int  bx = (int)(BK_POS[i] * keyW) - bkW / 2;
            uint16_t fill = pressed ? C_PIANO_PR : C_PIANO_B;

            g_canvas.fillRect(bx, pianoY, bkW, bkH, fill);
            g_canvas.drawRect(bx - 1, pianoY - 1, bkW + 2, bkH + 2, C_GRAY_DK);
            if (pressed) {
                g_canvas.drawRect(bx, pianoY, bkW, bkH, C_ACCENT);
            }
        }
    }

    // Статус BLE
    g_canvas.setTextSize(1);
    if (!g_bleConnected) {
        g_canvas.setTextColor(C_ORANGE);
        g_canvas.setCursor(SCREEN_W - 80, CONTENT_Y + 2);
        g_canvas.print("BLE:searching");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  ПОЛНАЯ ПЕРЕРИСОВКА
// ═══════════════════════════════════════════════════════════════════════════
void redraw() {
    g_canvas.fillSprite(C_BG);
    drawStatusBar();
    
    if (g_appMode == AppMode::MENU) {
        drawMenu();
    } else {
        drawPlay();
    }
    
    g_canvas.pushSprite(0, 0);
    g_needRedraw = false;
}
