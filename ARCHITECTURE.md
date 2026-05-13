# Архитектура: Диаграмма зависимостей и взаимодействия

## 📊 Граф зависимостей

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.cpp                                │
│                   (setup + loop)                                │
│  • Инициализирует оборудование                                 │
│  • Вызывает все модули в правильном порядке                    │
│  • Главный loop цикла приложения                               │
└────────────┬──────────────────────────────────────────────────────┘
             │
             │ включает и вызывает все модули
             │
             ├─→ config.h (константы) ────────────┐
             │                                     │ зависят от
             ├─→ types.h (типы) ──────────────────┤
             │                                     │
             ├─→ app_state.h/cpp (состояние) ◄─────┘
             │
             ├─→ matrix_keyboard.h/cpp (матрица)
             │        ↓
             │    config.h, Arduino.h
             │
             ├─→ keyboard_map.h (раскладка)
             │        ↓
             │    types.h, config.h
             │
             ├─→ midi_helpers.h/cpp (MIDI)
             │        ↓
             │    app_state.h, config.h, keyboard_map.h
             │
             ├─→ ble_midi_handler.h/cpp (BLE)
             │        ↓
             │    types.h, app_state.h, config.h, esp_bt.h
             │
             ├─→ display.h/cpp (UI)
             │        ↓
             │    app_state.h, config.h, types.h, keyboard_map.h
             │
             ├─→ menu_logic.h/cpp (меню)
             │        ↓
             │    app_state.h, config.h, types.h, ble_midi_handler.h
             │
             └─→ play_mode.h/cpp (игра)
                      ↓
                  app_state.h, config.h, keyboard_map.h,
                  matrix_keyboard.h, midi_helpers.h, ble_midi_handler.h
```

---

## 🔄 Поток данных в loop()

```
loop() [main.cpp]
  │
  ├─ M5Cardputer.update()          ← опрос встроенной клавиатуры
  │
  ├─ MIDI.read()                   ← получение MIDI событий
  │
  ├─ Обновление батареи
  │  └─ g_cached_bat = M5Cardputer.Power.getBatteryLevel()
  │
  ├─ scanMatrixKeyboard()          ← опрос матричной клавиатуры
  │  ├─ shiftOut595()              ← активация строк (74HC595)
  │  └─ shiftIn165()               ← чтение столбцов (74HC165)
  │
  └─ Обработка по режимам:
     │
     ├─ if (MODE_MENU)
     │  └─ handleMenuInput()        ← menu_logic.cpp
     │     ├─ Навигация (I/K)
     │     ├─ Изменение (J/L)
     │     └─ Вход (Enter)
     │
     └─ if (MODE_PLAY)
        └─ handlePlayMode()         ← play_mode.cpp
           ├─ Выход (Backspace)
           ├─ handleBuiltInKeyboard() ← встроенная клавиатура
           │  ├─ MIDI.sendNoteOn()
           │  └─ MIDI.sendNoteOff()
           └─ handleMatrixKeyboard()  ← матричная клавиатура
              ├─ getMatrixKey()
              ├─ MIDI.sendNoteOn()
              └─ MIDI.sendNoteOff()

Если нужна перерисовка:
  └─ redraw()                      ← display.cpp
     ├─ drawStatusBar()            ← верхняя полоса
     ├─ drawMenu() или drawPlay()  ← содержимое экрана
     └─ canvas.pushSprite()        ← вывод на дисплей
```

---

## 📦 Содержимое каждого модуля

```
config.h
├── DISPLAY (SCREEN_W, SCREEN_H, STATUS_H, ...)
├── COLORS (C_BG, C_ACCENT, C_WHITE, ...)
├── MATRIX PINS (SR_DATA_OUT, SR_CLOCK, ...)
├── MATRIX SIZE (MATRIX_ROWS, MATRIX_COLS)
├── MATRIX_MAP[4][10]
├── NOTE_NAMES[12]
├── TIMINGS (BAT_INTERVAL, NAV_DEBOUNCE)
└── MIDI RANGES (OCTAVE_MIN/MAX, VELOCITY_MIN/MAX, ...)

types.h
├── enum AppMode { MENU, PLAY }
├── enum MenuItemID { OCTAVE, VELOCITY, BLE_POWER, ... }
├── enum KeyboardSource { BUILT_IN, MATRIX, BOTH }
├── struct BLEPwrEntry { label, level }
└── struct NoteKey { key, rel }

app_state.h/cpp
├── g_appMode
├── g_bleConnected
├── g_needRedraw
├── g_octave, g_velocity, g_ble_pwr, g_midi_ch
├── g_kbd_source, g_matrix_enabled
├── g_menu_sel, g_last_nav_ms
├── g_key_state[256]
├── g_last_midi_note, g_last_note_label
├── g_last_bat_ms, g_cached_bat
└── g_matrix_state[], g_matrix_state_prev[]

matrix_keyboard.h/cpp
├── initShiftRegisters()
├── shiftOut595(uint16_t rowData)
├── shiftIn165() → uint8_t
├── scanMatrixKeyboard() → bool
└── getMatrixKey(row, col) → bool

keyboard_map.h
├── WHITE_KEYS[] { key, rel } × 10 клавиш
├── WK_COUNT = 10
├── BLACK_KEYS[] { key, rel } × 7 клавиш
├── BK_COUNT = 7
└── BK_POS[] - позиции чёрных клавиш для рисования

midi_helpers.h/cpp
├── midiNote(int8_t rel) → int
├── noteLabel(int midi) → String
└── anyKeyPressed() → bool

ble_midi_handler.h/cpp
├── BLE_PWR_TABLE[] { label, level } × 3
├── BLE_PWR_COUNT = 3
├── onBleConnected()
├── onBleDisconnected()
├── applyBlePower()
└── panicAllNotesOff()

display.h/cpp
├── drawStatusBar()
├── drawMenu()
├── drawPlay()
└── redraw()

menu_logic.h/cpp
└── handleMenuInput(unsigned long now) → bool

play_mode.h/cpp
├── handleBuiltInKeyboard() → bool (static)
├── handleMatrixKeyboard() → bool (static)
└── handlePlayMode(unsigned long now, bool matrixChanged) → bool

main.cpp
├── setup()
└── loop()
```

---

## 🔌 Подключения к аппаратуре

```
M5Stack Cardputer
├── Display (SPI внутренний)
│  └── M5Cardputer.Display
│      └── drawStatusBar(), drawMenu(), drawPlay()
│
├── Встроенная клавиатура
│  └── M5Cardputer.Keyboard
│      └── isKeyPressed(), isChange()
│          └── handleBuiltInKeyboard()
│
├── Батарея
│  └── M5Cardputer.Power
│      └── getBatteryLevel()
│
├── GPIO для сдвиговых регистров
│  ├── SR_DATA_OUT (G3)
│  ├── SR_CLOCK (G4)
│  ├── SR_LATCH_OUT (G5) ─┐
│  ├── SR_LOAD_IN (G6)    ├─→ 74HC595 (выводы)
│  │                       │
│  └── SR_DATA_IN (G13) ─→ 74HC165 (входы)
│
└── BLE MIDI (встроенный Bluetooth)
   └── BLEMIDI
       └── MIDI.sendNoteOn(), MIDI.sendNoteOff()
           └── Беспроводная MIDI клавиатура
```

---

## 📈 Размер и сложность модулей

```
Строк кода по модулям:

app_state.cpp     ████░░░░░░ 80 строк (состояние)
display.cpp       ███████░░░ 230 строк (самый большой - UI)
play_mode.cpp     █████░░░░░ 150 строк (логика игры)
matrix_keyboard   ████░░░░░░ 100 строк (сдвиговые регистры)
ble_midi_handler  ███░░░░░░░ 70 строк (BLE управление)
menu_logic.cpp    ███░░░░░░░ 70 строк (логика меню)
midi_helpers.cpp  ░░░░░░░░░░ 10 строк (маленький, полезный)
main.cpp          ██░░░░░░░░ 90 строк (главный файл)
keyboard_map.h    ██░░░░░░░░ 30 строк (только данные)
config.h          ██░░░░░░░░ 80 строк (только константы)
types.h           █░░░░░░░░░ 40 строк (только типы)

Итого: ~860 строк в 11 файлах (было 900 в 1 файле)

Сложность:
- Легкие модули (для новичков): config.h, types.h, keyboard_map.h
- Средние модули: midi_helpers, ble_midi_handler, menu_logic
- Сложные модули: matrix_keyboard, play_mode, display
- Совсем лёгкие модули: app_state (просто определения)
```

---

## 🎯 Как добавить новую функцию

### Пример 1: Добавить новый пункт в меню "Volume"

```
1. types.h
   enum MenuItemID {
       // ... существующие
       VOLUME,      ← ДОБАВИТЬ
   }

2. config.h
   #define VOLUME_MIN 0
   #define VOLUME_MAX 100
   static constexpr int VOLUME_DEFAULT = 50;  ← ДОБАВИТЬ

3. app_state.h/cpp
   extern int g_volume;  ← ДОБАВИТЬ ПЕРЕМЕННУЮ

4. menu_logic.cpp
   case MenuItemID::VOLUME:
       g_volume = constrain(g_volume + dir, VOLUME_MIN, VOLUME_MAX);
       break;  ← ДОБАВИТЬ ОБРАБОТКУ

5. display.cpp
   case MenuItemID::VOLUME:
       canvas.printf("<%3d>", g_volume);
       break;  ← ДОБАВИТЬ ОТРИСОВКУ
```

### Пример 2: Добавить эффект (сдвиг октавы при двойном нажатии)

```
1. app_state.h/cpp
   extern unsigned long g_last_key_press_time;  ← ДОБАВИТЬ

2. play_mode.cpp
   if (timeSinceLastPress < 300ms) {
       // Двойное нажатие
       g_octave = (g_octave + 1) % 5;
   }  ← ДОБАВИТЬ ЛОГИКУ

3. display.cpp
   // Может нужна визуализация

4. main.cpp
   // Всё работает автоматически!
```

---

## ⚖️ Баланс между файлами

Каждый файл имеет примерно 50-100 строк (кроме display.cpp с 230).

Это оптимальный размер для:
- Чтения в одном сеансе
- Навигации по коду
- Поиска нужной функции
- Редактирования без конфликтов

Файлы построены по принципу:
```
Большое = часто меняется    (display, play_mode)
Среднее = иногда меняется   (app_state, ble_midi_handler, menu_logic)
Маленькое = редко меняется  (config, types, keyboard_map)
```

---

## 🎁 Бонус: как тестировать модули отдельно

```cpp
// В tests/test_midi_helpers.cpp
#include "../src/midi_helpers.h"
#include <gtest/gtest.h>

TEST(MidiHelpers, midiNote) {
    g_octave = 0;  // С глобальным состоянием нужна подготовка
    EXPECT_EQ(midiNote(0), 60);   // C4
    EXPECT_EQ(midiNote(12), 72);  // C5
}

TEST(MidiHelpers, noteLabel) {
    EXPECT_EQ(noteLabel(60), "C4");
    EXPECT_EQ(noteLabel(61), "C#4");
}
```

Каждый модуль можно тестировать отдельно благодаря модульной архитектуре!

---

## 📚 Итог

Модульная архитектура обеспечивает:
- ✅ Читаемость (маленькие файлы)
- ✅ Поддерживаемость (чёткое разделение ответственности)
- ✅ Расширяемость (просто добавляй новые модули)
- ✅ Тестируемость (модули можно тестировать отдельно)
- ✅ Переиспользуемость (модули можно переиспользовать)

Это профессиональный подход, используемый в производственном коде! 🎉
