#include "menu_logic.h"
#include "app_state.h"
#include "config.h"
#include "types.h"
#include "ble_midi_handler.h"
#include <M5Cardputer.h>
#include <Arduino.h>

bool handleMenuInput(unsigned long now) {
    if (!M5Cardputer.Keyboard.isChange()) {
        return false;
    }

    bool debounceOk = (now - g_last_nav_ms >= NAV_DEBOUNCE);
    bool redraw_needed = false;

    // ──────────────────────────────────────────────────────────────
    //  НАВИГАЦИЯ ПО МЕНЮ (I/K)
    // ──────────────────────────────────────────────────────────────
    if (debounceOk) {
        // I — вверх (;)
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            g_menu_sel = (g_menu_sel - 1 + static_cast<int>(MenuItemID::COUNT)) 
                       % static_cast<int>(MenuItemID::COUNT);
            g_last_nav_ms = now;
            redraw_needed = true;
        }
        // K — вниз (.)
        if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            g_menu_sel = (g_menu_sel + 1) % static_cast<int>(MenuItemID::COUNT);
            g_last_nav_ms = now;
            redraw_needed = true;
        }
    }

    // ──────────────────────────────────────────────────────────────
    //  ИЗМЕНЕНИЕ ЗНАЧЕНИЙ (J/L)
    // ──────────────────────────────────────────────────────────────
    if (debounceOk) {
        int dir = 0;
        if (M5Cardputer.Keyboard.isKeyPressed(',')) dir = -1;  // J
        if (M5Cardputer.Keyboard.isKeyPressed('/')) dir = +1;  // L

        if (dir != 0) {
            MenuItemID item_id = static_cast<MenuItemID>(g_menu_sel);
            
            switch (item_id) {
                case MenuItemID::OCTAVE:
                    g_octave = constrain(g_octave + dir, OCTAVE_MIN, OCTAVE_MAX);
                    break;
                case MenuItemID::VELOCITY:
                    g_velocity = constrain(g_velocity + dir * VELOCITY_STEP, 
                                          VELOCITY_MIN, VELOCITY_MAX);
                    break;
                case MenuItemID::BLE_POWER:
                    g_ble_pwr = constrain(g_ble_pwr + dir, 0, BLE_PWR_COUNT - 1);
                    applyBlePower();
                    break;
                case MenuItemID::MIDI_CHANNEL:
                    g_midi_ch = constrain(g_midi_ch + dir, MIDI_CH_MIN, MIDI_CH_MAX);
                    break;
                case MenuItemID::KBD_SOURCE:
                    g_kbd_source = (g_kbd_source + dir + 3) % 3;  // 0, 1, 2
                    break;
                case MenuItemID::MATRIX_ENABLED:
                    g_matrix_enabled = !g_matrix_enabled;
                    break;
                default:
                    break;
            }
            g_last_nav_ms = now;
            redraw_needed = true;
        }
    }

    // ──────────────────────────────────────────────────────────────
    //  ВХОД В РЕЖИМ ИГРЫ (Enter)
    // ──────────────────────────────────────────────────────────────
    if (debounceOk) {
        bool enter_pressed = M5Cardputer.Keyboard.isKeyPressed('\n') ||
                            M5Cardputer.Keyboard.keysState().enter;
        
        if (enter_pressed) {
            if (g_menu_sel == static_cast<int>(MenuItemID::PLAY)) {
                g_appMode = AppMode::PLAY;
                g_last_midi_note = -1;
                g_last_note_label = "";
                memset(g_key_state, 0, sizeof(g_key_state));
                g_needRedraw = true;
                Serial.println("[APP] Enter Play Mode");
            }
            g_last_nav_ms = now;
        }
    }

    return redraw_needed;
}
