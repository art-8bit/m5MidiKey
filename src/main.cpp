/**
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║   M5Stack Cardputer — BLE MIDI Keyboard с поддержкой матрицы             ║
 * ║   Стиль UI: M5Stack Factory Test Demo                                   ║
 * ║                                                                          ║
 * ║   ВСТРОЕННАЯ КЛАВИАТУРА:                                                ║
 * ║   Маппинг клавиш (октава 0 = C4, MIDI 60):                              ║
 * ║     Белые ноты : A  S  D  F  G  H  J  K  L  ;                          ║
 * ║                  C  D  E  F  G  A  B  C5 D5 E5                          ║
 * ║     Чёрные ноты: W  E     T  Y  U     O  P                              ║
 * ║                  C# D#    F# G# A#    C#5D#5                             ║
 * ║                                                                          ║
 * ║   МАТРИЧНАЯ КЛАВИАТУРА (74HC595 + 74HC165):                             ║
 * ║   4×10 матрица на сдвиговых регистрах                                    ║
 * ║                                                                          ║
 * ║   Навигация меню: I/K — вверх/вниз, J/L — изменить значение             ║
 * ║   Вход в режим игры: Enter. Выход: Backspace (+ Panic All Notes Off)    ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

// ─────────────────────────────────────────────────────────────────────────────
//  Includes
// ─────────────────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5Unified.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#include "esp_bt.h"

// ─────────────────────────────────────────────────────────────────────────────
//  BLE MIDI Instance
// ─────────────────────────────────────────────────────────────────────────────
BLEMIDI_CREATE_INSTANCE("Cardputer MIDI", MIDI)

// ─────────────────────────────────────────────────────────────────────────────
//  Display constants
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int SCREEN_W  = 240;
static constexpr int SCREEN_H  = 135;
static constexpr int STATUS_H  = 22;
static constexpr int CONTENT_Y = STATUS_H;
static constexpr int CONTENT_H = SCREEN_H - STATUS_H;

// ─────────────────────────────────────────────────────────────────────────────
//  Цветовая палитра (RGB565)
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  Режимы приложения
// ─────────────────────────────────────────────────────────────────────────────
enum AppMode : uint8_t { MODE_MENU = 0, MODE_PLAY };

// ─────────────────────────────────────────────────────────────────────────────
//  ════════════════════════════════════════════════════════════════════════════
//  МАТРИЧНАЯ КЛАВИАТУРА: 74HC595 + 74HC165
//  ════════════════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────

// ── Пины сдвиговых регистров ──
static constexpr int SR_DATA_OUT = 3;   // G3  → DS (Data Set 595)
static constexpr int SR_CLOCK    = 4;   // G4  → SH/CP (Clock)
static constexpr int SR_LATCH_OUT= 5;   // G5  → ST (Strobe/Latch 595)
static constexpr int SR_LOAD_IN  = 6;   // G6  → PL (Parallel Load 165)
static constexpr int SR_DATA_IN  = 13;  // G13 → Q7 (Data Out 165)

// ── Параметры матрицы ──
static constexpr int MATRIX_ROWS = 4;   // количество строк (595 chips = 2 × 8 бит = 16 выходов, используем 4)
static constexpr int MATRIX_COLS = 10;  // количество столбцов (165 = 8 бит, используем 10, но это кастом)

// Маппинг матричной позиции → MIDI нота (относительно C текущей октавы)
// Форма: [строка][столбец] → смещение в полутонах от C
static constexpr int8_t MATRIX_MAP[MATRIX_ROWS][MATRIX_COLS] = {
    // Строка 0: белые ноты C D E F G
    { 0,  2,  4,  5,  7,  9, 11, 12, 14, 16 },  // C D E F G A B C' D' E'
    // Строка 1: чёрные ноты C# D# F# G# A#
    { 1,  3, -1,  6,  8, 10, -1, 13, 15, -1 },  // C# D# -- F# G# A# -- C#' D#' --
    // Строка 2: дополнительные коэффициенты октавы (если нужны тангенты)
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  // резерв
    // Строка 3: система (резерв под особые функции)
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  // резерв
};

// Кэш состояния матричной клавиатуры
static uint8_t matrixState[MATRIX_ROWS] = {};
static uint8_t matrixStatePrev[MATRIX_ROWS] = {};

// ─────────────────────────────────────────────────────────────────────────────
//  Маппинг встроенной клавиатуры → MIDI-ноты
// ─────────────────────────────────────────────────────────────────────────────
struct NoteKey { uint8_t key; int8_t rel; };

static constexpr NoteKey WHITE_KEYS[] = {
    {'a',  0}, {'s',  2}, {'d',  4}, {'f',  5}, {'g',  7},
    {'h',  9}, {'j', 11}, {'k', 12}, {'l', 14}, {';', 16},
};
static constexpr int WK_COUNT = sizeof(WHITE_KEYS) / sizeof(WHITE_KEYS[0]);

static constexpr NoteKey BLACK_KEYS[] = {
    {'w',  1}, {'e',  3}, {'t',  6}, {'y',  8}, {'u', 10},
    {'o', 13}, {'p', 15},
};
static constexpr int BK_COUNT = sizeof(BLACK_KEYS) / sizeof(BLACK_KEYS[0]);

static constexpr float BK_POS[BK_COUNT] = {
    0.6f, 1.6f, 3.6f, 4.6f, 5.6f, 7.6f, 8.6f,
};

static constexpr const char* NOTE_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

// ─────────────────────────────────────────────────────────────────────────────
//  BLE Power table
// ─────────────────────────────────────────────────────────────────────────────
struct BLEPwrEntry { const char* label; esp_power_level_t level; };
static constexpr BLEPwrEntry BLE_PWR_TABLE[] = {
    {"-12dBm", ESP_PWR_LVL_N12},
    {"  0dBm", ESP_PWR_LVL_N0 },
    {" +9dBm", ESP_PWR_LVL_P9 },
};
static constexpr int BLE_PWR_COUNT = 3;

// ─────────────────────────────────────────────────────────────────────────────
//  Menu items
// ─────────────────────────────────────────────────────────────────────────────
enum MenuID : uint8_t {
    MN_OCTAVE = 0,
    MN_VELOCITY,
    MN_BLE_POWER,
    MN_MIDI_CH,
    MN_KBD_SOURCE,      // [NEW] Источник клавиатуры
    MN_MATRIX_ENABLED,  // [NEW] Включить матричную клавиатуру
    MN_PLAY,
    MN_COUNT
};
static constexpr const char* MN_LABELS[MN_COUNT] = {
    "Octave",
    "Velocity",
    "BLE Power",
    "MIDI Channel",
    "Keyboard Source",  // [NEW]
    "Matrix Keyboard",  // [NEW]
    "\x10 Play Mode \x11",
};

// ─────────────────────────────────────────────────────────────────────────────
//  Global state
// ─────────────────────────────────────────────────────────────────────────────
M5Canvas canvas(&M5Cardputer.Display);

AppMode   appMode        = MODE_MENU;
bool      bleConnected   = false;
bool      needRedraw     = true;

// Настройки
int  gOctave        =  0;   // -2..+2
int  gVelocity      = 100;  // 0..127
int  gBlePwr        =  1;   // индекс в BLE_PWR_TABLE
int  gMidiCh        =  1;   // 1..16
int  gKbdSource     =  0;   // 0=встроенная, 1=матрица, 2=обе
bool gMatrixEnabled =  true; // включена ли матрица

// Меню
int           menuSel     = 0;
unsigned long lastNavMs   = 0;
static constexpr unsigned long NAV_DEBOUNCE = 180;

// Нажатые клавиши
bool keyState[256] = {};

// Информация о последней ноте
int    lastMidiNote  = -1;
String lastNoteLabel = "";

// Батарея
unsigned long lastBatMs   = 0;
int           cachedBat   = -1;
static constexpr unsigned long BAT_INTERVAL = 10000;

// ─────────────────────────────────────────────────────────────────────────────
//  ════════════════════════════════════════════════════════════════════════════
//  ФУНКЦИИ СДВИГОВЫХ РЕГИСТРОВ
//  ════════════════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Инициализация GPIO для сдвиговых регистров
 */
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

/**
 * Отправить данные в 74HC595 (строка матрицы)
 * rowData — 16 бит (2 чипа 595): каждый бит = одна строка
 */
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

/**
 * Прочитать данные из 74HC165 (столбцы матрицы)
 * Возвращает 8 бит со столбцов
 */
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

/**
 * Сканировать матричную клавиатуру
 * Возвращает true если состояние изменилось
 */
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
        
        if (colData != matrixStatePrev[row]) {
            matrixState[row] = colData;
            matrixStatePrev[row] = colData;
            changed = true;
        } else {
            matrixState[row] = colData;
        }
    }
    
    // Отключаем все строки
    shiftOut595(0xFFFF);
    
    return changed;
}

/**
 * Получить состояние конкретной клетки матрицы
 * Возвращает true если клавиша нажата
 */
bool getMatrixKey(int row, int col) {
    if (row < 0 || row >= MATRIX_ROWS || col < 0 || col >= MATRIX_COLS) {
        return false;
    }
    return (matrixState[row] >> col) & 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
inline int midiNote(int8_t rel) {
    return constrain(60 + gOctave * 12 + rel, 0, 127);
}

String noteLabel(int midi) {
    int oct = (midi / 12) - 1;
    int n   = midi % 12;
    return String(NOTE_NAMES[n]) + String(oct);
}

bool anyKeyPressed() {
    for (int i = 0; i < WK_COUNT; i++)
        if (keyState[WHITE_KEYS[i].key]) return true;
    for (int i = 0; i < BK_COUNT; i++)
        if (keyState[BLACK_KEYS[i].key]) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  BLE callbacks
// ─────────────────────────────────────────────────────────────────────────────
void onBleConnected() {
    bleConnected = true;
    needRedraw   = true;
    Serial.println("[BLE] Connected");
}

void onBleDisconnected() {
    bleConnected = false;
    needRedraw   = true;
    Serial.println("[BLE] Disconnected");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Panic
// ─────────────────────────────────────────────────────────────────────────────
void panicAllNotesOff() {
    for (int ch = 1; ch <= 16; ch++) {
        MIDI.sendControlChange(123, 0, ch);
        MIDI.sendControlChange(121, 0, ch);
    }
    memset(keyState, 0, sizeof(keyState));
    lastMidiNote  = -1;
    lastNoteLabel = "";
    Serial.println("[MIDI] Panic: All Notes Off sent on all channels");
}

void applyBlePower() {
    esp_power_level_t lvl = BLE_PWR_TABLE[gBlePwr].level;
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,    lvl);
    Serial.printf("[BLE] TX power → %s\n", BLE_PWR_TABLE[gBlePwr].label);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Draw: Status Bar  ════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void drawStatusBar() {
    canvas.fillRect(0, 0, SCREEN_W, STATUS_H, C_STATUS_BG);
    canvas.drawFastHLine(0, STATUS_H - 1, SCREEN_W, C_ACCENT);

    canvas.setTextSize(1);

    canvas.setTextColor(C_ACCENT, C_STATUS_BG);
    canvas.setCursor(5, 7);
    canvas.print("Cardputer MIDI");

    canvas.setTextColor(C_YELLOW, C_STATUS_BG);
    canvas.setCursor(SCREEN_W / 2 - 22, 7);
    canvas.printf("Oct %+d", gOctave);

    int batX = SCREEN_W - 62;
    if (cachedBat < 0) {
        canvas.setTextColor(C_GRAY, C_STATUS_BG);
        canvas.setCursor(batX, 7);
        canvas.print("BAT:--");
    } else {
        uint16_t batColor = (cachedBat > 30) ? C_GREEN
                          : (cachedBat > 15) ? C_YELLOW
                          :                    C_RED;
        canvas.setTextColor(batColor, C_STATUS_BG);
        canvas.setCursor(batX, 7);
        canvas.printf("BAT:%d%%", cachedBat);
    }

    int cx = SCREEN_W - 7;
    int cy = STATUS_H / 2 - 1;
    if (bleConnected) {
        canvas.fillCircle(cx, cy, 4, C_GREEN);
        canvas.setTextColor(C_BG, C_GREEN);
        canvas.setCursor(cx - 2, cy - 3);
        canvas.print("B");
    } else {
        canvas.drawCircle(cx, cy, 4, C_GRAY);
        canvas.fillCircle(cx, cy, 3, C_STATUS_BG);
        canvas.setTextColor(C_GRAY, C_STATUS_BG);
        canvas.setCursor(cx - 2, cy - 3);
        canvas.print("B");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Draw: Menu  ══════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void drawMenu() {
    canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, C_BG);

    static constexpr int ITEM_H  = 16;
    static constexpr int MENU_W  = 218;
    static constexpr int MENU_X  = (SCREEN_W - MENU_W) / 2;
    static constexpr int START_Y = CONTENT_Y + 2;
    static constexpr int VAL_X   = MENU_X + 128;

    canvas.setTextSize(1);

    for (int i = 0; i < MN_COUNT; i++) {
        int y = START_Y + i * ITEM_H;
        bool sel = (i == menuSel);

        if (sel) {
            canvas.fillRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 4, C_SEL_BG);
            canvas.drawRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 4, C_ACCENT);
            canvas.fillTriangle(
                MENU_X - 10, y + ITEM_H/2 - 2,
                MENU_X - 4,  y + ITEM_H/2,
                MENU_X - 10, y + ITEM_H/2 + 3,
                C_ACCENT
            );
        }

        canvas.setTextColor(sel ? C_ACCENT : C_GRAY_LT);
        canvas.setCursor(MENU_X + 4, y + 3);
        if (i == MN_PLAY) {
            canvas.setTextColor(sel ? C_ACCENT : C_ACCENT2);
        }
        canvas.print(MN_LABELS[i]);

        // Значение
        if (i != MN_PLAY) {
            canvas.setTextColor(sel ? C_WHITE : C_YELLOW);
            canvas.setCursor(VAL_X, y + 3);
            switch (i) {
                case MN_OCTAVE:
                    canvas.printf("<%+d>", gOctave);
                    break;
                case MN_VELOCITY:
                    canvas.printf("<%3d>", gVelocity);
                    break;
                case MN_BLE_POWER:
                    canvas.printf("<%s>", BLE_PWR_TABLE[gBlePwr].label);
                    break;
                case MN_MIDI_CH:
                    canvas.printf("<%2d>", gMidiCh);
                    break;
                case MN_KBD_SOURCE:
                    canvas.setTextColor(sel ? C_WHITE : C_ACCENT2);
                    if (gKbdSource == 0)
                        canvas.printf("<Built-in>");
                    else if (gKbdSource == 1)
                        canvas.printf("<Matrix>");
                    else
                        canvas.printf("<Both>");
                    break;
                case MN_MATRIX_ENABLED:
                    canvas.setTextColor(sel ? C_WHITE : C_GREEN);
                    canvas.printf("<%s>", gMatrixEnabled ? "ON" : "OFF");
                    break;
            }
        }
    }

    int divY = START_Y + (MN_PLAY - 1) * ITEM_H + ITEM_H - 1;
    canvas.drawFastHLine(MENU_X, divY, MENU_W, C_GRAY_DK);

    canvas.setTextColor(C_GRAY_DK);
    canvas.setCursor(3, SCREEN_H - 8);
    canvas.setTextSize(1);
    canvas.print("';'/'.':nav  ','/'/':val  Enter:play");
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Draw: Play Mode  ═════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void drawPlay() {
    canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, C_BG);

    canvas.setTextSize(1);
    canvas.setTextColor(C_GRAY);
    canvas.setCursor(4, CONTENT_Y + 2);
    if (gKbdSource == 0)
        canvas.print("Built-in kbd");
    else if (gKbdSource == 1)
        canvas.print("Matrix kbd");
    else
        canvas.print("Both kbd");
    canvas.print("  BS=menu");

    int noteBoxY = CONTENT_Y + 13;
    int noteBoxH = 35;
    canvas.fillRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH, 6, C_STATUS_BG);
    canvas.drawRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH,
                         6, bleConnected ? C_ACCENT : C_GRAY_DK);

    if (lastMidiNote >= 0) {
        canvas.setTextSize(2);
        canvas.setTextColor(C_ACCENT);
        int tw = lastNoteLabel.length() * 12;
        int tx = (SCREEN_W - tw) / 2;
        canvas.setCursor(tx, noteBoxY + 6);
        canvas.print(lastNoteLabel);

        canvas.setTextSize(1);
        canvas.setTextColor(C_GRAY);
        canvas.setCursor(10, noteBoxY + noteBoxH - 10);
        canvas.printf("MIDI:%d vel:%d", lastMidiNote, gVelocity);
    } else {
        canvas.setTextSize(1);
        canvas.setTextColor(C_GRAY);
        canvas.setCursor(SCREEN_W / 2 - 35, noteBoxY + 12);
        canvas.print("play a note...");
    }

    // Мини-пианино (встроенная клавиатура если она активна)
    if (gKbdSource == 0 || gKbdSource == 2) {
        int pianoY = noteBoxY + noteBoxH + 2;
        int pianoH = SCREEN_H - pianoY - 1;
        float keyW = (float)SCREEN_W / WK_COUNT;

        for (int i = 0; i < WK_COUNT; i++) {
            bool pressed = keyState[WHITE_KEYS[i].key];
            int  x  = (int)(i * keyW);
            int  w  = (int)((i + 1) * keyW) - x;
            uint16_t fill = pressed ? C_PIANO_PR : C_PIANO_W;

            canvas.fillRect(x + 1, pianoY, w - 1, pianoH - 1, fill);
            canvas.drawRect(x, pianoY - 1, w + 1, pianoH + 1, C_GRAY_DK);

            if (pressed) {
                canvas.drawRect(x + 1, pianoY, w - 1, pianoH - 1, C_ACCENT);
            }
        }

        int bkW = (int)(keyW * 0.62f);
        int bkH = (pianoH * 6) / 10;

        for (int i = 0; i < BK_COUNT; i++) {
            bool pressed = keyState[BLACK_KEYS[i].key];
            int  bx = (int)(BK_POS[i] * keyW) - bkW / 2;
            uint16_t fill = pressed ? C_PIANO_PR : C_PIANO_B;

            canvas.fillRect(bx, pianoY, bkW, bkH, fill);
            canvas.drawRect(bx - 1, pianoY - 1, bkW + 2, bkH + 2, C_GRAY_DK);
            if (pressed) {
                canvas.drawRect(bx, pianoY, bkW, bkH, C_ACCENT);
            }
        }
    }

    // Статус BLE
    canvas.setTextSize(1);
    if (!bleConnected) {
        canvas.setTextColor(C_ORANGE);
        canvas.setCursor(SCREEN_W - 80, CONTENT_Y + 2);
        canvas.print("BLE:searching");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Full Redraw  ═════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void redraw() {
    canvas.fillSprite(C_BG);
    drawStatusBar();
    if (appMode == MODE_MENU) {
        drawMenu();
    } else {
        drawPlay();
    }
    canvas.pushSprite(0, 0);
    needRedraw = false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Setup  ═══════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    setCpuFrequencyMhz(80);

    Serial.begin(115200);
    Serial.println("[BOOT] Cardputer BLE MIDI Keyboard + Matrix");
    Serial.printf("[CPU] Frequency: %d MHz\n", getCpuFrequencyMhz());

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(120);

    M5Cardputer.Display.fillScreen(C_BG);
    M5Cardputer.Display.setTextColor(C_ACCENT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(50, 50);
    M5Cardputer.Display.print("Initializing...");
    M5Cardputer.Display.setCursor(50, 70);
    M5Cardputer.Display.print("BLE + Matrix Keyboard");

    canvas.createSprite(SCREEN_W, SCREEN_H);
    canvas.setTextWrap(false);

    // Инициализируем сдвиговые регистры
    initShiftRegisters();
    Serial.println("[HW] Shift Registers initialized");

    // BLE MIDI
    MIDI.begin(MIDI_CHANNEL_OMNI);
    BLEMIDI.setHandleConnected(onBleConnected);
    BLEMIDI.setHandleDisconnected(onBleDisconnected);

    applyBlePower();

    cachedBat = M5Cardputer.Power.getBatteryLevel();

    needRedraw = true;
    Serial.println("[BOOT] Ready");
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Loop  ════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    M5Cardputer.update();
    MIDI.read();

    unsigned long now = millis();

    // ── Обновление батареи ──
    if (now - lastBatMs >= BAT_INTERVAL) {
        int bat = M5Cardputer.Power.getBatteryLevel();
        if (bat != cachedBat) {
            cachedBat  = bat;
            needRedraw = true;
        }
        lastBatMs = now;
    }

    // ── Сканируем матричную клавиатуру ──
    bool matrixChanged = false;
    if (gMatrixEnabled) {
        matrixChanged = scanMatrixKeyboard();
    }

    // ════════════════════════════════════════════════════════════
    //  РЕЖИМ МЕНЮ
    // ════════════════════════════════════════════════════════════
    if (appMode == MODE_MENU) {
        if (M5Cardputer.Keyboard.isChange()) {

            bool debounceOk = (now - lastNavMs >= NAV_DEBOUNCE);

            if (debounceOk) {
                if (M5Cardputer.Keyboard.isKeyPressed(';')) {
                    menuSel    = (menuSel - 1 + MN_COUNT) % MN_COUNT;
                    lastNavMs  = now;
                    needRedraw = true;
                }
                if (M5Cardputer.Keyboard.isKeyPressed('.')) {
                    menuSel    = (menuSel + 1) % MN_COUNT;
                    lastNavMs  = now;
                    needRedraw = true;
                }
            }

            if (debounceOk) {
                int dir = 0;
                if (M5Cardputer.Keyboard.isKeyPressed(',')) dir = -1;
                if (M5Cardputer.Keyboard.isKeyPressed('/')) dir = +1;

                if (dir != 0) {
                    switch (menuSel) {
                        case MN_OCTAVE:
                            gOctave = constrain(gOctave + dir, -2, 2);
                            break;
                        case MN_VELOCITY:
                            gVelocity = constrain(gVelocity + dir * 8, 0, 127);
                            break;
                        case MN_BLE_POWER:
                            gBlePwr = constrain(gBlePwr + dir, 0, BLE_PWR_COUNT - 1);
                            applyBlePower();
                            break;
                        case MN_MIDI_CH:
                            gMidiCh = constrain(gMidiCh + dir, 1, 16);
                            break;
                        case MN_KBD_SOURCE:
                            gKbdSource = (gKbdSource + dir + 3) % 3;  // 0, 1, 2
                            break;
                        case MN_MATRIX_ENABLED:
                            gMatrixEnabled = !gMatrixEnabled;
                            break;
                        default:
                            break;
                    }
                    lastNavMs  = now;
                    needRedraw = true;
                }
            }

            if (M5Cardputer.Keyboard.isKeyPressed('\n') ||
                M5Cardputer.Keyboard.keysState().enter)
            {
                if (debounceOk) {
                    if (menuSel == MN_PLAY) {
                        appMode   = MODE_PLAY;
                        lastMidiNote  = -1;
                        lastNoteLabel = "";
                        memset(keyState, 0, sizeof(keyState));
                        needRedraw = true;
                        Serial.println("[APP] Enter Play Mode");
                    }
                    lastNavMs = now;
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    //  РЕЖИМ ИГРЫ
    // ════════════════════════════════════════════════════════════
    else /* MODE_PLAY */ {
        
        // Backspace → выход в меню
        if (M5Cardputer.Keyboard.isChange() &&
            (M5Cardputer.Keyboard.isKeyPressed('\b') ||
             M5Cardputer.Keyboard.isKeyPressed(0x7F) ||
             M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)))
        {
            panicAllNotesOff();
            appMode    = MODE_MENU;
            needRedraw = true;
            Serial.println("[APP] Back to Menu");
            delay(20);
            return;
        }

        bool pianoChanged = false;

        // ══════════════════════════════════════════════════
        //  ВСТРОЕННАЯ КЛАВИАТУРА
        // ══════════════════════════════════════════════════
        if (M5Cardputer.Keyboard.isChange() && (gKbdSource == 0 || gKbdSource == 2)) {

            for (int i = 0; i < WK_COUNT; i++) {
                uint8_t k   = WHITE_KEYS[i].key;
                bool nowOn  = M5Cardputer.Keyboard.isKeyPressed(k);
                bool wasOn  = keyState[k];

                if (nowOn && !wasOn) {
                    int mn = midiNote(WHITE_KEYS[i].rel);
                    MIDI.sendNoteOn(mn, gVelocity, gMidiCh);
                    keyState[k]   = true;
                    lastMidiNote  = mn;
                    lastNoteLabel = noteLabel(mn);
                    pianoChanged  = true;
                    Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [built-in]\n",
                                  lastNoteLabel.c_str(), mn, gVelocity, gMidiCh);
                }
                else if (!nowOn && wasOn) {
                    int mn = midiNote(WHITE_KEYS[i].rel);
                    MIDI.sendNoteOff(mn, 0, gMidiCh);
                    keyState[k]  = false;
                    pianoChanged = true;
                    if (!anyKeyPressed()) {
                        lastMidiNote  = -1;
                        lastNoteLabel = "";
                    }
                }
            }

            for (int i = 0; i < BK_COUNT; i++) {
                uint8_t k   = BLACK_KEYS[i].key;
                bool nowOn  = M5Cardputer.Keyboard.isKeyPressed(k);
                bool wasOn  = keyState[k];

                if (nowOn && !wasOn) {
                    int mn = midiNote(BLACK_KEYS[i].rel);
                    MIDI.sendNoteOn(mn, gVelocity, gMidiCh);
                    keyState[k]   = true;
                    lastMidiNote  = mn;
                    lastNoteLabel = noteLabel(mn);
                    pianoChanged  = true;
                    Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [built-in]\n",
                                  lastNoteLabel.c_str(), mn, gVelocity, gMidiCh);
                }
                else if (!nowOn && wasOn) {
                    int mn = midiNote(BLACK_KEYS[i].rel);
                    MIDI.sendNoteOff(mn, 0, gMidiCh);
                    keyState[k]  = false;
                    pianoChanged = true;
                    if (!anyKeyPressed()) {
                        lastMidiNote  = -1;
                        lastNoteLabel = "";
                    }
                }
            }

            if (pianoChanged) needRedraw = true;
        }

        // ══════════════════════════════════════════════════
        //  МАТРИЧНАЯ КЛАВИАТУРА
        // ══════════════════════════════════════════════════
        if (matrixChanged && (gKbdSource == 1 || gKbdSource == 2)) {

            for (int row = 0; row < MATRIX_ROWS; row++) {
                for (int col = 0; col < MATRIX_COLS; col++) {
                    int8_t rel = MATRIX_MAP[row][col];
                    if (rel < 0) continue;  // пропускаем неиспользуемые ячейки

                    // Уникальный ID для матричной ячейки: 150 + row*10 + col
                    uint8_t matrixKey = 150 + row * MATRIX_COLS + col;
                    bool nowOn = getMatrixKey(row, col);
                    bool wasOn = keyState[matrixKey];

                    if (nowOn && !wasOn) {
                        int mn = midiNote(rel);
                        MIDI.sendNoteOn(mn, gVelocity, gMidiCh);
                        keyState[matrixKey] = true;
                        lastMidiNote = mn;
                        lastNoteLabel = noteLabel(mn);
                        pianoChanged = true;
                        Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d [matrix %d,%d]\n",
                                      lastNoteLabel.c_str(), mn, gVelocity, gMidiCh, row, col);
                    }
                    else if (!nowOn && wasOn) {
                        int mn = midiNote(rel);
                        MIDI.sendNoteOff(mn, 0, gMidiCh);
                        keyState[matrixKey] = false;
                        pianoChanged = true;
                        if (!anyKeyPressed()) {
                            lastMidiNote = -1;
                            lastNoteLabel = "";
                        }
                    }
                }
            }

            if (pianoChanged) needRedraw = true;
        }
    }

    // ── Перерисовка ──
    if (needRedraw) {
        redraw();
    }

    delay(12);
}