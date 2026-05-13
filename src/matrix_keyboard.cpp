#include "matrix_keyboard.h"
#include "app_state.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ИНИЦИАЛИЗАЦИЯ
// ═══════════════════════════════════════════════════════════════════════════

void initShiftRegisters() {
    pinMode(SR_DATA_OUT,  OUTPUT);
    pinMode(SR_CLOCK,     OUTPUT);
    pinMode(SR_LATCH_OUT, OUTPUT);
    pinMode(SR_LOAD_IN,   OUTPUT);
    pinMode(SR_DATA_IN,   INPUT);

    // Устанавливаем начальные состояния
    digitalWrite(SR_DATA_OUT,  LOW);
    digitalWrite(SR_CLOCK,     LOW);
    digitalWrite(SR_LATCH_OUT, HIGH);  // Latch неактивен (high)
    digitalWrite(SR_LOAD_IN,   HIGH);  // Load неактивен (high)
}

// ═══════════════════════════════════════════════════════════════════════════
//  РАБОТА СО СДВИГОВЫМИ РЕГИСТРАМИ
// ═══════════════════════════════════════════════════════════════════════════

void shiftOut595(uint16_t rowData) {
    // Сдвигаем 16 бит в выходной регистр
    for (int i = 15; i >= 0; i--) {
        digitalWrite(SR_DATA_OUT, (rowData >> i) & 1 ? HIGH : LOW);
        digitalWrite(SR_CLOCK, HIGH);
        digitalWrite(SR_CLOCK, LOW);
    }
    
    // Фиксируем данные на выходах (latch pulse)
    digitalWrite(SR_LATCH_OUT, HIGH);
    digitalWrite(SR_LATCH_OUT, LOW);
}

uint8_t shiftIn165() {
    uint8_t data = 0;
    
    // Загружаем данные со входов параллельно
    digitalWrite(SR_LOAD_IN, LOW);
    digitalWrite(SR_LOAD_IN, HIGH);
    
    // Читаем серийно
    for (int i = 7; i >= 0; i--) {
        data |= (digitalRead(SR_DATA_IN) << i);
        digitalWrite(SR_CLOCK, HIGH);
        digitalWrite(SR_CLOCK, LOW);
    }
    
    return data;
}

// ═══════════════════════════════════════════════════════════════════════════
//  СКАНИРОВАНИЕ МАТРИЦЫ
// ═══════════════════════════════════════════════════════════════════════════

bool scanMatrixKeyboard() {
    bool changed = false;
    
    // Сканируем каждую строку
    for (int row = 0; row < MATRIX_ROWS; row++) {
        // Устанавливаем активной одну строку (бит = 0 активирует строку)
        uint16_t rowData = ~(1 << row);  // инвертируем для активации
        shiftOut595(rowData);
        
        delayMicroseconds(50);  // небольшая задержка для стабилизации
        
        // Читаем столбцы
        uint8_t colData = shiftIn165();
        
        if (colData != g_matrix_state_prev[row]) {
            g_matrix_state[row] = colData;
            g_matrix_state_prev[row] = colData;
            changed = true;
        } else {
            g_matrix_state[row] = colData;
        }
    }
    
    // Отключаем все строки
    shiftOut595(0xFFFF);
    
    return changed;
}

bool getMatrixKey(int row, int col) {
    if (row < 0 || row >= MATRIX_ROWS || col < 0 || col >= MATRIX_COLS) {
        return false;
    }
    return (g_matrix_state[row] >> col) & 1;
}
