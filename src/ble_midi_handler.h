#ifndef BLE_MIDI_HANDLER_H
#define BLE_MIDI_HANDLER_H

#include <cstdint>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#include "esp_bt.h"
#include "types.h"

// ═══════════════════════════════════════════════════════════════════════════
//  ПРИМЕЧАНИЕ: MIDI и BLEMIDI объекты создаются через макро
//  BLEMIDI_CREATE_INSTANCE в ble_midi_handler.cpp
//  Все остальные файлы видят их через включение этого заголовка
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
//  ТАБЛИЦА УРОВНЕЙ МОЩНОСТИ BLE
// ═══════════════════════════════════════════════════════════════════════════
static constexpr BLEPwrEntry BLE_PWR_TABLE[] = {
    {"-12dBm", ESP_PWR_LVL_N12},
    {"  0dBm", ESP_PWR_LVL_N0 },
    {" +9dBm", ESP_PWR_LVL_P9 },
};
static constexpr int BLE_PWR_COUNT = 3;

// ═══════════════════════════════════════════════════════════════════════════
//  CALLBACKS И УПРАВЛЕНИЕ BLE
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Callback: BLE соединение установлено
 */
void onBleConnected();

/**
 * Callback: BLE соединение разорвано
 */
void onBleDisconnected();

/**
 * Применить уровень мощности BLE
 * Использует текущее значение g_ble_pwr
 */
void applyBlePower();

/**
 * Паника: отправить All Notes Off на все каналы и очистить состояние
 */
void panicAllNotesOff();

#endif // BLE_MIDI_HANDLER_H
