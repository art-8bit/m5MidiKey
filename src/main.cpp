/**
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║   M5Stack Cardputer — BLE MIDI Keyboard                                 ║
 * ║   Стиль UI: M5Stack Factory Test Demo                                   ║
 * ║                                                                          ║
 * ║   Маппинг клавиш (октава 0 = C4, MIDI 60):                              ║
 * ║     Белые ноты : A  S  D  F  G  H  J  K  L  ;                          ║
 * ║                  C  D  E  F  G  A  B  C5 D5 E5                          ║
 * ║     Чёрные ноты: W  E     T  Y  U     O  P                              ║
 * ║                  C# D#    F# G# A#    C#5D#5                             ║
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
#include "esp_bt.h"          // esp_ble_tx_power_set, esp_power_level_t

// ─────────────────────────────────────────────────────────────────────────────
//  BLE MIDI Instance
//  Макрос создаёт два объекта:
//    BLEMIDI — транспортный слой (BLE + MIDI = BLEMIDI)
//    MIDI    — высокоуровневый MIDI-интерфейс
// ─────────────────────────────────────────────────────────────────────────────
BLEMIDI_CREATE_INSTANCE("Cardputer MIDI", MIDI)

// ─────────────────────────────────────────────────────────────────────────────
//  Display constants
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int SCREEN_W  = 240;
static constexpr int SCREEN_H  = 135;
static constexpr int STATUS_H  = 22;   // высота статус-бара
static constexpr int CONTENT_Y = STATUS_H;
static constexpr int CONTENT_H = SCREEN_H - STATUS_H;

// ─────────────────────────────────────────────────────────────────────────────
//  Цветовая палитра (RGB565), вдохновлена M5Stack Factory Demo
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint16_t C_BG        = 0x0841;  // тёмно-синий фон
static constexpr uint16_t C_STATUS_BG = 0x0010;  // почти чёрный синий
static constexpr uint16_t C_ACCENT    = 0x07FF;  // яркий циан — главный акцент
static constexpr uint16_t C_ACCENT2   = 0x03EF;  // приглушённый циан
static constexpr uint16_t C_WHITE     = 0xFFFF;
static constexpr uint16_t C_GRAY_LT   = 0xC618;  // светло-серый
static constexpr uint16_t C_GRAY      = 0x8410;  // серый
static constexpr uint16_t C_GRAY_DK   = 0x4208;  // тёмно-серый
static constexpr uint16_t C_GREEN     = 0x07E0;
static constexpr uint16_t C_RED       = 0xF800;
static constexpr uint16_t C_YELLOW    = 0xFFE0;
static constexpr uint16_t C_ORANGE    = 0xFD00;
static constexpr uint16_t C_SEL_BG    = 0x0233;  // фон выделенного пункта меню
static constexpr uint16_t C_PIANO_W   = 0xF7DE;  // белая клавиша
static constexpr uint16_t C_PIANO_B   = 0x1082;  // чёрная клавиша (не совсем чёрная)
static constexpr uint16_t C_PIANO_PR  = 0x07FF;  // нажатая клавиша

// ─────────────────────────────────────────────────────────────────────────────
//  Режимы приложения
// ─────────────────────────────────────────────────────────────────────────────
enum AppMode : uint8_t { MODE_MENU = 0, MODE_PLAY };

// ─────────────────────────────────────────────────────────────────────────────
//  Маппинг клавиш → MIDI-ноты (относительно базовой C)
// ─────────────────────────────────────────────────────────────────────────────
struct NoteKey { uint8_t key; int8_t rel; }; // rel = полутоны от C текущей октавы

// Белые клавиши (средний ряд)
static constexpr NoteKey WHITE_KEYS[] = {
    {'a',  0}, // C
    {'s',  2}, // D
    {'d',  4}, // E
    {'f',  5}, // F
    {'g',  7}, // G
    {'h',  9}, // A
    {'j', 11}, // B
    {'k', 12}, // C+1
    {'l', 14}, // D+1
    {';', 16}, // E+1
};
static constexpr int WK_COUNT = (int)(sizeof(WHITE_KEYS) / sizeof(WHITE_KEYS[0]));

// Чёрные клавиши (верхний ряд)
static constexpr NoteKey BLACK_KEYS[] = {
    {'w',  1}, // C#
    {'e',  3}, // D#
    {'t',  6}, // F#
    {'y',  8}, // G#
    {'u', 10}, // A#
    {'o', 13}, // C#5
    {'p', 15}, // D#5
};
static constexpr int BK_COUNT = (int)(sizeof(BLACK_KEYS) / sizeof(BLACK_KEYS[0]));

// Позиции чёрных клавиш относительно белых (в единицах ширины белой клавиши)
// Стандартное фортепиано: C# между C(0) и D(1) → 0.6, D# → 1.6, ...
static constexpr float BK_POS[BK_COUNT] = {
    0.6f, 1.6f,        // C# D#
    3.6f, 4.6f, 5.6f,  // F# G# A#
    7.6f, 8.6f,        // C#5 D#5
};

// Имена нот
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
    MN_PLAY,
    MN_COUNT
};
static constexpr const char* MN_LABELS[MN_COUNT] = {
    "Octave",
    "Velocity",
    "BLE Power",
    "MIDI Channel",
    "\x10 Play Mode \x11",  // ► Play Mode ◄
};

// ─────────────────────────────────────────────────────────────────────────────
//  Global state
// ─────────────────────────────────────────────────────────────────────────────
M5Canvas canvas(&M5Cardputer.Display);

AppMode   appMode      = MODE_MENU;
bool      bleConnected = false;
bool      needRedraw   = true;

// Настройки
int  gOctave   =  0;   // -2..+2
int  gVelocity = 100;  // 0..127
int  gBlePwr   =  1;   // индекс в BLE_PWR_TABLE
int  gMidiCh   =  1;   // 1..16

// Меню
int           menuSel     = 0;
unsigned long lastNavMs   = 0;
static constexpr unsigned long NAV_DEBOUNCE = 180; // мс

// Нажатые клавиши (ASCII → bool)
bool keyState[256] = {};

// Информация о последней сыгранной ноте
int    lastMidiNote  = -1;
String lastNoteLabel = "";

// Для обновления батарейки реже (раз в 10 секунд)
unsigned long lastBatMs   = 0;
int           cachedBat   = -1;
static constexpr unsigned long BAT_INTERVAL = 10000;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
inline int midiNote(int8_t rel) {
    // базовая нота C4 = 60, смещение октавы
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
//  Panic: заглушить все залипшие ноты
// ─────────────────────────────────────────────────────────────────────────────
void panicAllNotesOff() {
    for (int ch = 1; ch <= 16; ch++) {
        MIDI.sendControlChange(123, 0, ch); // All Notes Off
        MIDI.sendControlChange(121, 0, ch); // Reset All Controllers (бонус)
    }
    memset(keyState, 0, sizeof(keyState));
    lastMidiNote  = -1;
    lastNoteLabel = "";
    Serial.println("[MIDI] Panic: All Notes Off sent on all channels");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Применить мощность BLE-передатчика
// ─────────────────────────────────────────────────────────────────────────────
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
    // Фон статус-бара
    canvas.fillRect(0, 0, SCREEN_W, STATUS_H, C_STATUS_BG);
    // Нижняя граница с акцентом
    canvas.drawFastHLine(0, STATUS_H - 1, SCREEN_W, C_ACCENT);

    canvas.setTextSize(1);

    // ── Левая часть: название устройства ──
    canvas.setTextColor(C_ACCENT, C_STATUS_BG);
    canvas.setCursor(5, 7);
    canvas.print("Cardputer MIDI");

    // ── Центр: режим + октава ──
    canvas.setTextColor(C_YELLOW, C_STATUS_BG);
    canvas.setCursor(SCREEN_W / 2 - 22, 7);
    canvas.printf("Oct %+d", gOctave);

    // ── Правая часть: батарея ──
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

    // ── BLE-индикатор (маленький кружок справа) ──
    int cx = SCREEN_W - 7;
    int cy = STATUS_H / 2 - 1;
    if (bleConnected) {
        canvas.fillCircle(cx, cy, 4, C_GREEN);
        // Буква B внутри
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

    static constexpr int ITEM_H  = 19;
    static constexpr int MENU_W  = 218;
    static constexpr int MENU_X  = (SCREEN_W - MENU_W) / 2;
    static constexpr int START_Y = CONTENT_Y + 3;
    static constexpr int VAL_X   = MENU_X + 128; // колонка значений

    canvas.setTextSize(1);

    for (int i = 0; i < MN_COUNT; i++) {
        int y = START_Y + i * ITEM_H;
        bool sel = (i == menuSel);

        if (sel) {
            // Подсвеченный пункт: заливка + скруглённая рамка
            canvas.fillRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 5, C_SEL_BG);
            canvas.drawRoundRect(MENU_X - 3, y, MENU_W + 6, ITEM_H - 2, 5, C_ACCENT);
            // Стрелка-маркер слева
            canvas.fillTriangle(
                MENU_X - 10, y + ITEM_H/2 - 3,
                MENU_X - 4,  y + ITEM_H/2 - 1 + 1,
                MENU_X - 10, y + ITEM_H/2 + 4,
                C_ACCENT
            );
        }

        // Метка
        canvas.setTextColor(sel ? C_ACCENT : C_GRAY_LT, 0); // прозрачный фон
        if (!sel) canvas.setTextColor(C_GRAY_LT);
        canvas.setCursor(MENU_X + 4, y + 5);
        if (i == MN_PLAY) {
            canvas.setTextColor(sel ? C_ACCENT : C_ACCENT2);
        }
        canvas.print(MN_LABELS[i]);

        // Значение (для всех пунктов кроме PLAY)
        if (i != MN_PLAY) {
            canvas.setTextColor(sel ? C_WHITE : C_YELLOW);
            canvas.setCursor(VAL_X, y + 5);
            switch (i) {
                case MN_OCTAVE:
                    canvas.printf("< %+d >", gOctave);
                    break;
                case MN_VELOCITY:
                    canvas.printf("< %3d >", gVelocity);
                    break;
                case MN_BLE_POWER:
                    canvas.printf("<%s>", BLE_PWR_TABLE[gBlePwr].label);
                    break;
                case MN_MIDI_CH:
                    canvas.printf("< %2d >", gMidiCh);
                    break;
            }
        }
    }

    // Разделитель перед PLAY (визуально отделяем от настроек)
    int divY = START_Y + (MN_PLAY - 1) * ITEM_H + ITEM_H - 1;
    canvas.drawFastHLine(MENU_X, divY, MENU_W, C_GRAY_DK);

    // Подсказка навигации
    canvas.setTextColor(C_GRAY_DK);
    canvas.setCursor(5, SCREEN_H - 9);
    canvas.print("I/K:nav   J/L:value   Enter:confirm");
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Draw: Play Mode  ═════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void drawPlay() {
    canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, C_BG);

    // ── Верхняя инструкция ──
    canvas.setTextSize(1);
    canvas.setTextColor(C_GRAY);
    canvas.setCursor(4, CONTENT_Y + 2);
    canvas.print("ASDF=white  WETY=black  BS=menu");

    // ── Область отображения ноты ──
    int noteBoxY = CONTENT_Y + 13;
    int noteBoxH = 44;
    canvas.fillRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH, 7, C_STATUS_BG);
    canvas.drawRoundRect(8, noteBoxY, SCREEN_W - 16, noteBoxH,
                         7, bleConnected ? C_ACCENT : C_GRAY_DK);

    if (lastMidiNote >= 0) {
        // Большое имя ноты
        canvas.setTextSize(3);
        canvas.setTextColor(C_ACCENT);
        // Примерная ширина символа: 6px * size = 18px
        int tw = lastNoteLabel.length() * 18;
        int tx = (SCREEN_W - tw) / 2;
        canvas.setCursor(tx, noteBoxY + 8);
        canvas.print(lastNoteLabel);

        // MIDI номер ноты справа внизу
        canvas.setTextSize(1);
        canvas.setTextColor(C_GRAY);
        canvas.setCursor(SCREEN_W - 48, noteBoxY + noteBoxH - 11);
        canvas.printf("MIDI:%d", lastMidiNote);

        // Индикатор velocity (маленькая полоска)
        canvas.setTextColor(C_GRAY);
        canvas.setCursor(10, noteBoxY + noteBoxH - 11);
        canvas.printf("vel:%d", gVelocity);
        int barW = (gVelocity * 60) / 127;
        canvas.fillRect(40, noteBoxY + noteBoxH - 8, barW, 4, C_ACCENT2);
        canvas.drawRect(40, noteBoxY + noteBoxH - 8, 60, 4, C_GRAY_DK);
    } else {
        canvas.setTextSize(1);
        canvas.setTextColor(C_GRAY);
        canvas.setCursor(SCREEN_W / 2 - 30, noteBoxY + 16);
        canvas.print("play a note...");
    }

    // ══════════════════════════════════
    //  Мини-пианино
    // ══════════════════════════════════
    int pianoY = noteBoxY + noteBoxH + 4;
    int pianoH = SCREEN_H - pianoY - 1;
    float keyW = (float)SCREEN_W / WK_COUNT;

    // Белые клавиши
    for (int i = 0; i < WK_COUNT; i++) {
        bool pressed = keyState[WHITE_KEYS[i].key];
        int  x  = (int)(i * keyW);
        int  w  = (int)((i + 1) * keyW) - x;

        uint16_t fill   = pressed ? C_PIANO_PR : C_PIANO_W;
        uint16_t border = C_GRAY_DK;

        canvas.fillRect(x + 1, pianoY,     w - 1, pianoH - 1, fill);
        canvas.drawRect(x,     pianoY - 1, w + 1, pianoH + 1, border);

        if (pressed) {
            // Гало-рамка при нажатии
            canvas.drawRect(x + 1, pianoY, w - 1, pianoH - 1, C_ACCENT);
        }

        // Буква клавиши (только если клавиша достаточно широкая)
        if (w > 12) {
            canvas.setTextSize(1);
            canvas.setTextColor(pressed ? C_BG : C_GRAY_DK);
            canvas.setCursor(x + (w - 6) / 2, pianoY + pianoH - 10);
            canvas.printf("%c", WHITE_KEYS[i].key - 0x20); // uppercase
        }
    }

    // Чёрные клавиши (поверх белых)
    int bkW = (int)(keyW * 0.62f);
    int bkH = (pianoH * 6) / 10;

    for (int i = 0; i < BK_COUNT; i++) {
        bool pressed = keyState[BLACK_KEYS[i].key];
        int  bx = (int)(BK_POS[i] * keyW) - bkW / 2;

        uint16_t fill = pressed ? C_PIANO_PR : C_PIANO_B;

        canvas.fillRect(bx,     pianoY,     bkW,     bkH,     fill);
        canvas.drawRect(bx - 1, pianoY - 1, bkW + 2, bkH + 2, C_GRAY_DK);
        if (pressed) {
            canvas.drawRect(bx, pianoY, bkW, bkH, C_ACCENT);
        }
    }

    // Статус BLE внизу экрана
    canvas.setTextSize(1);
    if (!bleConnected) {
        canvas.setTextColor(C_ORANGE);
        canvas.setCursor(SCREEN_W - 95, CONTENT_Y + 2);
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
    // ── CPU throttle для охлаждения (240 → 80 MHz) ──
    setCpuFrequencyMhz(80);

    Serial.begin(115200);
    Serial.println("[BOOT] Cardputer BLE MIDI Keyboard");
    Serial.printf("[CPU] Frequency: %d MHz\n", getCpuFrequencyMhz());

    // ── M5Cardputer init ──
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);  // true = init Keyboard
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(120);

    // Первичный экран до готовности
    M5Cardputer.Display.fillScreen(C_BG);
    M5Cardputer.Display.setTextColor(C_ACCENT);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(60, 55);
    M5Cardputer.Display.print("Starting BLE MIDI...");

    // ── Canvas для double-buffering ──
    canvas.createSprite(SCREEN_W, SCREEN_H);
    canvas.setTextWrap(false);

    // ── BLE MIDI init ──
    MIDI.begin(MIDI_CHANNEL_OMNI);
    BLEMIDI.setHandleConnected(onBleConnected);
    BLEMIDI.setHandleDisconnected(onBleDisconnected);

    applyBlePower();

    // Первый замер батареи
    cachedBat = M5Cardputer.Power.getBatteryLevel();

    needRedraw = true;
    Serial.println("[BOOT] Ready");
}

// ─────────────────────────────────────────────────────────────────────────────
//  ═══  Loop  ════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    M5Cardputer.update();
    MIDI.read();  // обслуживаем BLE стек

    unsigned long now = millis();

    // ── Периодическое обновление батареи ──
    if (now - lastBatMs >= BAT_INTERVAL) {
        int bat = M5Cardputer.Power.getBatteryLevel();
        if (bat != cachedBat) {
            cachedBat  = bat;
            needRedraw = true;
        }
        lastBatMs = now;
    }

    // ══════════════════════════════════════════════════════════
    //  Режим МЕНЮ
    // ══════════════════════════════════════════════════════════
    if (appMode == MODE_MENU) {
        if (M5Cardputer.Keyboard.isChange()) {

            bool debounceOk = (now - lastNavMs >= NAV_DEBOUNCE);

            // ── Навигация вверх / вниз (I / K) ──
            if (debounceOk) {
                if (M5Cardputer.Keyboard.isKeyPressed('i')) {
                    menuSel    = (menuSel - 1 + MN_COUNT) % MN_COUNT;
                    lastNavMs  = now;
                    needRedraw = true;
                }
                if (M5Cardputer.Keyboard.isKeyPressed('k')) {
                    menuSel    = (menuSel + 1) % MN_COUNT;
                    lastNavMs  = now;
                    needRedraw = true;
                }
            }

            // ── Изменение значения (J = -1, L = +1) ──
            if (debounceOk) {
                int dir = 0;
                if (M5Cardputer.Keyboard.isKeyPressed('j')) dir = -1;
                if (M5Cardputer.Keyboard.isKeyPressed('l')) dir = +1;

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
                        default:
                            break;
                    }
                    lastNavMs  = now;
                    needRedraw = true;
                }
            }

            // ── Enter / Return — подтвердить / войти в режим игры ──
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

    // ══════════════════════════════════════════════════════════
    //  Режим ИГРЫ
    // ══════════════════════════════════════════════════════════
    else /* MODE_PLAY */ {
        if (M5Cardputer.Keyboard.isChange()) {

            // ── Backspace → выход в меню + Panic ──
            if (M5Cardputer.Keyboard.isKeyPressed('\b') ||   // ASCII 8
                M5Cardputer.Keyboard.isKeyPressed(0x7F) ||   // DEL
                M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE))
            {
                panicAllNotesOff();
                appMode    = MODE_MENU;
                needRedraw = true;
                Serial.println("[APP] Back to Menu");
                delay(20);  // поглощаем клавишу
                return;
            }

            bool pianoChanged = false;

            // ── Белые клавиши ──
            for (int i = 0; i < WK_COUNT; i++) {
                uint8_t k   = WHITE_KEYS[i].key;
                bool nowOn  = M5Cardputer.Keyboard.isKeyPressed(k);
                bool wasOn  = keyState[k];

                if (nowOn && !wasOn) {
                    // NOTE ON
                    int mn = midiNote(WHITE_KEYS[i].rel);
                    MIDI.sendNoteOn(mn, gVelocity, gMidiCh);
                    keyState[k]   = true;
                    lastMidiNote  = mn;
                    lastNoteLabel = noteLabel(mn);
                    pianoChanged  = true;
                    Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d\n",
                                  lastNoteLabel.c_str(), mn, gVelocity, gMidiCh);
                }
                else if (!nowOn && wasOn) {
                    // NOTE OFF
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

            // ── Чёрные клавиши ──
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
                    Serial.printf("[MIDI] NoteOn  %s (%d) vel=%d ch=%d\n",
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
    }

    // ── Перерисовка только если нужно (double-buffering) ──
    if (needRedraw) {
        redraw();
    }

    // ── Задержка охлаждения (не даём ESP32 греться) ──
    delay(12);
}