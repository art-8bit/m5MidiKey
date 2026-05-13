/**
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║   M5Stack Cardputer — BLE MIDI Keyboard с поддержкой матрицы             ║
 * ║   РЕФАКТОРИНГ: Разделено на модули для улучшения читаемости              ║
 * ║                                                                          ║
 * ║   Основные модули:                                                       ║
 * ║   • config.h - константы и конфигурация                                  ║
 * ║   • types.h - типы данных                                                ║
 * ║   • app_state.h/cpp - глобальное состояние                               ║
 * ║   • matrix_keyboard.h/cpp - матричная клавиатура                         ║
 * ║   • keyboard_map.h - маппинг встроенной клавиатуры                        ║
 * ║   • ble_midi_handler.h/cpp - работа с BLE MIDI                           ║
 * ║   • midi_helpers.h/cpp - вспомогательные функции MIDI                    ║
 * ║   • display.h/cpp - рисование UI                                         ║
 * ║   • menu_logic.h/cpp - логика меню                                       ║
 * ║   • play_mode.h/cpp - логика режима игры                                 ║
 * ║                                                                          ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5Unified.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#include "esp_bt.h"

// Наши модули - ВАЖНО: ble_midi_handler.h включить рано!
#include "config.h"
#include "types.h"
#include "ble_midi_handler.h"    // ← ЗДЕСЬ! Создаёт MIDI и BLEMIDI
#include "app_state.h"
#include "matrix_keyboard.h"
#include "keyboard_map.h"
#include "midi_helpers.h"
#include "display.h"
#include "menu_logic.h"
#include "play_mode.h"

// ═══════════════════════════════════════════════════════════════════════════
//  ════════════════════════════════════════════════════════════════════════════
//  SETUP
//  ════════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    // ──────────────────────────────────────────────────────────────
    //  ИНИЦИАЛИЗАЦИЯ ОБОРУДОВАНИЯ
    // ──────────────────────────────────────────────────────────────
    setCpuFrequencyMhz(80);

    Serial.begin(115200);
    Serial.println("\n\n[BOOT] ════════════════════════════════════════════════════════");
    Serial.println("[BOOT] Cardputer BLE MIDI Keyboard + Matrix");
    Serial.printf("[BOOT] CPU Frequency: %d MHz\n", getCpuFrequencyMhz());

    // ──────────────────────────────────────────────────────────────
    //  M5STACK ИНИЦИАЛИЗАЦИЯ
    // ──────────────────────────────────────────────────────────────
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(120);

    // Экран загрузки
    M5Cardputer.Display.fillScreen(C_BG);
    M5Cardputer.Display.setTextColor(C_ACCENT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(50, 50);
    M5Cardputer.Display.print("Initializing...");
    M5Cardputer.Display.setCursor(50, 70);
    M5Cardputer.Display.print("BLE + Matrix Keyboard");

    // ──────────────────────────────────────────────────────────────
    //  ХОЛСТ ДЛЯ ДВОЙНОЙ БУФЕРИЗАЦИИ
    // ──────────────────────────────────────────────────────────────
    g_canvas.createSprite(SCREEN_W, SCREEN_H);
    g_canvas.setTextWrap(false);

    // ──────────────────────────────────────────────────────────────
    //  ИНИЦИАЛИЗАЦИЯ СДВИГОВЫХ РЕГИСТРОВ
    // ──────────────────────────────────────────────────────────────
    initShiftRegisters();
    Serial.println("[HW] Shift Registers initialized");

    // ──────────────────────────────────────────────────────────────
    //  BLE MIDI
    // ──────────────────────────────────────────────────────────────
    MIDI.begin(MIDI_CHANNEL_OMNI);
    BLEMIDI.setHandleConnected(onBleConnected);
    BLEMIDI.setHandleDisconnected(onBleDisconnected);
    applyBlePower();
    Serial.println("[BLE] MIDI initialized");

    // ──────────────────────────────────────────────────────────────
    //  БАТАРЕЯ
    // ──────────────────────────────────────────────────────────────
    g_cached_bat = M5Cardputer.Power.getBatteryLevel();
    Serial.printf("[POWER] Battery: %d%%\n", g_cached_bat);

    // ──────────────────────────────────────────────────────────────
    //  ГОТОВО
    // ──────────────────────────────────────────────────────────────
    g_needRedraw = true;
    Serial.println("[BOOT] ════════════════════════════════════════════════════════");
    Serial.println("[BOOT] Ready! Press Enter to play");
    Serial.println();
}

// ═══════════════════════════════════════════════════════════════════════════
//  ════════════════════════════════════════════════════════════════════════════
//  LOOP
//  ════════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
    M5Cardputer.update();
    MIDI.read();

    unsigned long now = millis();

    // ════════════════════════════════════════════════════════════
    //  БАТАРЕЯ (обновляем периодически)
    // ════════════════════════════════════════════════════════════
    if (now - g_last_bat_ms >= BAT_INTERVAL) {
        int bat = M5Cardputer.Power.getBatteryLevel();
        if (bat != g_cached_bat) {
            g_cached_bat  = bat;
            g_needRedraw = true;
        }
        g_last_bat_ms = now;
    }

    // ════════════════════════════════════════════════════════════
    //  МАТРИЧНАЯ КЛАВИАТУРА (сканируем если включена)
    // ════════════════════════════════════════════════════════════
    bool matrixChanged = false;
    if (g_matrix_enabled) {
        matrixChanged = scanMatrixKeyboard();
    }

    // ════════════════════════════════════════════════════════════
    //  ОБРАБОТКА ВХОДОВ ПО РЕЖИМАМ
    // ════════════════════════════════════════════════════════════

    if (g_appMode == AppMode::MENU) {
        // ══════════════════════════════════════════════════════
        //  РЕЖИМ МЕНЮ
        // ══════════════════════════════════════════════════════
        if (handleMenuInput(now)) {
            g_needRedraw = true;
        }
    } 
    else /* AppMode::PLAY */ {
        // ══════════════════════════════════════════════════════
        //  РЕЖИМ ИГРЫ
        // ══════════════════════════════════════════════════════
        if (handlePlayMode(now, matrixChanged)) {
            g_needRedraw = true;
        }
    }

    // ════════════════════════════════════════════════════════════
    //  ПЕРЕРИСОВКА ЭКРАНА
    // ════════════════════════════════════════════════════════════
    if (g_needRedraw) {
        redraw();
    }

    // ════════════════════════════════════════════════════════════
    //  НЕБОЛЬШАЯ ЗАДЕРЖКА ДЛЯ СТАБИЛЬНОСТИ
    // ════════════════════════════════════════════════════════════
    delay(12);
}
