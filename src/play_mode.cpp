#include "play_mode.h"
#include "app_state.h"
#include "config.h"
#include "keyboard_map.h"
#include "matrix_keyboard.h"
#include "midi_helpers.h"
#include "ble_midi_handler.h"    // ← ВАЖНО! Включить до использования MIDI
#include <M5Cardputer.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ВСТРОЕННАЯ КЛАВИАТУРА
// ═══════════════════════════════════════════════════════════════════════════

static bool handleBuiltInKeyboard(unsigned long now) {
    bool changed = false;
    
    // Белые клавиши
    for (int i = 0; i < WK_COUNT; i++) {
        uint8_t k   = WHITE_KEYS[i].key;
        bool nowOn  = M5Cardputer.Keyboard.isKeyPressed(k);
        bool wasOn  = g_key_state[k];

        if (nowOn && !wasOn) {
            int mn = midiNote(WHITE_KEYS[i].rel);
            MIDI.sendNoteOn(mn, g_velocity, g_midi_ch);
            g_key_state[k] = true;
            g_last_midi_note = mn;
            g_last_note_label = noteLabel(mn);
            changed = true;
            Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [built-in]\n",
                          g_last_note_label.c_str(), mn, g_velocity, g_midi_ch);
        }
        else if (!nowOn && wasOn) {
            int mn = midiNote(WHITE_KEYS[i].rel);
            MIDI.sendNoteOff(mn, 0, g_midi_ch);
            g_key_state[k] = false;
            changed = true;
            if (!anyKeyPressed()) {
                g_last_midi_note = -1;
                g_last_note_label = "";
            }
        }
    }

    // Чёрные клавиши
    for (int i = 0; i < BK_COUNT; i++) {
        uint8_t k   = BLACK_KEYS[i].key;
        bool nowOn  = M5Cardputer.Keyboard.isKeyPressed(k);
        bool wasOn  = g_key_state[k];

        if (nowOn && !wasOn) {
            int mn = midiNote(BLACK_KEYS[i].rel);
            MIDI.sendNoteOn(mn, g_velocity, g_midi_ch);
            g_key_state[k] = true;
            g_last_midi_note = mn;
            g_last_note_label = noteLabel(mn);
            changed = true;
            Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [built-in]\n",
                          g_last_note_label.c_str(), mn, g_velocity, g_midi_ch);
        }
        else if (!nowOn && wasOn) {
            int mn = midiNote(BLACK_KEYS[i].rel);
            MIDI.sendNoteOff(mn, 0, g_midi_ch);
            g_key_state[k] = false;
            changed = true;
            if (!anyKeyPressed()) {
                g_last_midi_note = -1;
                g_last_note_label = "";
            }
        }
    }

    return changed;
}

// ═══════════════════════════════════════════════════════════════════════════
//  МАТРИЧНАЯ КЛАВИАТУРА
// ═══════════════════════════════════════════════════════════════════════════

static bool handleMatrixKeyboard(unsigned long now) {
    bool changed = false;

    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            int8_t rel = MATRIX_MAP[row][col];
            if (rel < 0) continue;  // пропускаем неиспользуемые ячейки

            // Уникальный ID для матричной ячейки: 150 + row*10 + col
            uint8_t matrixKey = 150 + row * MATRIX_COLS + col;
            bool nowOn = getMatrixKey(row, col);
            bool wasOn = g_key_state[matrixKey];

            if (nowOn && !wasOn) {
                int mn = midiNote(rel);
                MIDI.sendNoteOn(mn, g_velocity, g_midi_ch);
                g_key_state[matrixKey] = true;
                g_last_midi_note = mn;
                g_last_note_label = noteLabel(mn);
                changed = true;
                Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [matrix %d,%d]\n",
                              g_last_note_label.c_str(), mn, g_velocity, g_midi_ch, row, col);
            }
            else if (!nowOn && wasOn) {
                int mn = midiNote(rel);
                MIDI.sendNoteOff(mn, 0, g_midi_ch);
                g_key_state[matrixKey] = false;
                changed = true;
                if (!anyKeyPressed()) {
                    g_last_midi_note = -1;
                    g_last_note_label = "";
                }
            }
        }
    }

    return changed;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ОСНОВНАЯ ФУНКЦИЯ ОБРАБОТКИ РЕЖИМА ИГРЫ
// ═══════════════════════════════════════════════════════════════════════════

bool handlePlayMode(unsigned long now, bool matrixChanged) {
    bool redraw_needed = false;

    // ──────────────────────────────────────────────────────────────
    //  ВЫХОД В МЕНЮ (Backspace)
    // ──────────────────────────────────────────────────────────────
    bool kbdChanged = M5Cardputer.Keyboard.isChange();
    bool backspacePressed = kbdChanged &&
        (M5Cardputer.Keyboard.isKeyPressed('\b') ||
         M5Cardputer.Keyboard.isKeyPressed(0x7F) ||
         M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE));

    if (backspacePressed) {
        panicAllNotesOff();
        g_appMode = AppMode::MENU;
        g_needRedraw = true;
        Serial.println("[APP] Back to Menu");
        return true;
    }

    // ──────────────────────────────────────────────────────────────
    //  ВСТРОЕННАЯ КЛАВИАТУРА
    // ──────────────────────────────────────────────────────────────
    if (!backspacePressed && kbdChanged && (g_kbd_source == 0 || g_kbd_source == 2)) {
        if (handleBuiltInKeyboard(now)) {
            redraw_needed = true;
        }
    }

    // ──────────────────────────────────────────────────────────────
    //  МАТРИЧНАЯ КЛАВИАТУРА
    // ──────────────────────────────────────────────────────────────
    if (matrixChanged && (g_kbd_source == 1 || g_kbd_source == 2)) {
        if (handleMatrixKeyboard(now)) {
            redraw_needed = true;
        }
    }

    return redraw_needed;
}
