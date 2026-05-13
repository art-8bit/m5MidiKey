#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ЭКРАН И ДИСПЛЕЙ
// ═══════════════════════════════════════════════════════════════════════════
static constexpr int SCREEN_W    = 240;
static constexpr int SCREEN_H    = 135;
static constexpr int STATUS_H    = 22;
static constexpr int CONTENT_Y   = STATUS_H;
static constexpr int CONTENT_H   = SCREEN_H - STATUS_H;

// ═══════════════════════════════════════════════════════════════════════════
//  ЦВЕТОВАЯ ПАЛИТРА (RGB565)
// ═══════════════════════════════════════════════════════════════════════════
static constexpr uint16_t C_BG        = 0x0841;
static constexpr uint16_t C_STATUS_BG = 0x0010;
static constexpr uint16_t C_ACCENT    = 0x07FF;
static constexpr uint16_t C_ACCENT2   = 0x03EF;
static constexpr uint16_t C_WHITE     = 0xFFFF;
static constexpr uint16_t C_GRAY_LT   = 0xC618;
static constexpr uint16_t C_GRAY      = 0x8410;
static constexpr uint16_t C_GRAY_DK   = 0x4208;
static constexpr uint16_t C_GREEN     = 0x07E0;
static constexpr uint16_t C_RED       = 0xF800;
static constexpr uint16_t C_YELLOW    = 0xFFE0;
static constexpr uint16_t C_ORANGE    = 0xFD00;
static constexpr uint16_t C_SEL_BG    = 0x0233;
static constexpr uint16_t C_PIANO_W   = 0xF7DE;
static constexpr uint16_t C_PIANO_B   = 0x1082;
static constexpr uint16_t C_PIANO_PR  = 0x07FF;

// ═══════════════════════════════════════════════════════════════════════════
//  МАТРИЧНАЯ КЛАВИАТУРА: ПИНЫ
// ═══════════════════════════════════════════════════════════════════════════
static constexpr int SR_DATA_OUT  = 3;   // G3  → DS (Data Set 595)
static constexpr int SR_CLOCK     = 4;   // G4  → SH/CP (Clock)
static constexpr int SR_LATCH_OUT = 5;   // G5  → ST (Strobe/Latch 595)
static constexpr int SR_LOAD_IN   = 6;   // G6  → PL (Parallel Load 165)
static constexpr int SR_DATA_IN   = 13;  // G13 → Q7 (Data Out 165)

// ═══════════════════════════════════════════════════════════════════════════
//  МАТРИЧНАЯ КЛАВИАТУРА: РАЗМЕРЫ
// ═══════════════════════════════════════════════════════════════════════════
static constexpr int MATRIX_ROWS = 4;
static constexpr int MATRIX_COLS = 10;

// Маппинг матрицы: [строка][столбец] → смещение в полутонах от C
static constexpr int8_t MATRIX_MAP[MATRIX_ROWS][MATRIX_COLS] = {
    // Белые ноты:  C D E F G A B C' D' E'
    { 0,  2,  4,  5,  7,  9, 11, 12, 14, 16 },
    // Чёрные ноты:  C# D# -- F# G# A# -- C#' D#' --
    { 1,  3, -1,  6,  8, 10, -1, 13, 15, -1 },
    // Резерв
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
};

// ═══════════════════════════════════════════════════════════════════════════
//  ВСТРОЕННАЯ КЛАВИАТУРА
// ═══════════════════════════════════════════════════════════════════════════
static constexpr const char* NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// ═══════════════════════════════════════════════════════════════════════════
//  БАТАРЕЯ И ОБНОВЛЕНИЯ
// ═══════════════════════════════════════════════════════════════════════════
static constexpr unsigned long BAT_INTERVAL  = 10000;  // 10 сек
static constexpr unsigned long NAV_DEBOUNCE  = 180;    // мс

// ═══════════════════════════════════════════════════════════════════════════
//  ПАРАМЕТРЫ MIDI И BLE
// ═══════════════════════════════════════════════════════════════════════════
static constexpr int OCTAVE_MIN   = -2;
static constexpr int OCTAVE_MAX   = 2;
static constexpr int OCTAVE_DEFAULT = 0;

static constexpr int VELOCITY_MIN   = 0;
static constexpr int VELOCITY_MAX   = 127;
static constexpr int VELOCITY_DEFAULT = 100;
static constexpr int VELOCITY_STEP  = 8;

static constexpr int MIDI_CH_MIN   = 1;
static constexpr int MIDI_CH_MAX   = 16;
static constexpr int MIDI_CH_DEFAULT = 1;

#endif // CONFIG_H
