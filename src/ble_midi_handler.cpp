#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#include "esp_bt.h"
#include "ble_midi_handler.h"
#include "app_state.h"
#include "config.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  СОЗДАНИЕ ГЛОБАЛЬНОГО MIDI ОБЪЕКТА (один раз в cpp файле!)
// ═══════════════════════════════════════════════════════════════════════════
BLEMIDI_CREATE_INSTANCE("Cardputer MIDI", MIDI)

// ═══════════════════════════════════════════════════════════════════════════
//  CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════

void onBleConnected() {
    g_bleConnected = true;
    g_needRedraw   = true;
    Serial.println("[BLE] Connected");
}

void onBleDisconnected() {
    g_bleConnected = false;
    g_needRedraw   = true;
    Serial.println("[BLE] Disconnected");
}

// ═══════════════════════════════════════════════════════════════════════════
//  УПРАВЛЕНИЕ ПИТАНИЕМ BLE
// ═══════════════════════════════════════════════════════════════════════════

void applyBlePower() {
    esp_power_level_t lvl = BLE_PWR_TABLE[g_ble_pwr].level;
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,    lvl);
    Serial.printf("[BLE] TX power → %s\n", BLE_PWR_TABLE[g_ble_pwr].label);
}

// ═══════════════════════════════════════════════════════════════════════════
//  ПАНИКА: ОТПРАВИТЬ ALL NOTES OFF
// ═══════════════════════════════════════════════════════════════════════════

void panicAllNotesOff() {
    for (int ch = 1; ch <= 16; ch++) {
        MIDI.sendControlChange(123, 0, ch);
        MIDI.sendControlChange(121, 0, ch);
    }
    memset(g_key_state, 0, sizeof(g_key_state));
    g_last_midi_note  = -1;
    g_last_note_label = "";
    Serial.println("[MIDI] Panic: All Notes Off sent on all channels");
}
