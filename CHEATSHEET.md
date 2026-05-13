# 🚀 CHEAT SHEET - Быстрый поиск нужного кода

## 🎯 Я хочу найти...

### Переменные и настройки

| Что | Где | Примечание |
|-----|-----|-----------|
| текущую октаву | `g_octave` в `app_state.h` | -2..+2 |
| текущую скорость ноты | `g_velocity` в `app_state.h` | 0..127 |
| MIDI канал | `g_midi_ch` в `app_state.h` | 1..16 |
| выбранный пункт меню | `g_menu_sel` в `app_state.h` | 0..6 |
| нужна ли перерисовка | `g_needRedraw` в `app_state.h` | true/false |
| включена ли матрица | `g_matrix_enabled` в `app_state.h` | true/false |
| статус батареи | `g_cached_bat` в `app_state.h` | % |
| открытый режим | `g_appMode` в `app_state.h` | AppMode::MENU/PLAY |
| статус BLE | `g_bleConnected` в `app_state.h` | true/false |

### Константы и конфигурация

| Что | Где | Примечание |
|-----|-----|-----------|
| размеры экрана | `config.h` | SCREEN_W, SCREEN_H |
| высота статусбара | `config.h` | STATUS_H = 22 |
| цвет фона | `config.h` | C_BG = 0x0841 |
| все цвета палитры | `config.h` | C_ACCENT, C_WHITE, C_GREEN, ... |
| пины сдвиговых регистров | `config.h` | SR_DATA_OUT, SR_CLOCK, ... |
| размер матрицы | `config.h` | MATRIX_ROWS = 4, MATRIX_COLS = 10 |
| маппинг матрицы | `config.h` | MATRIX_MAP[4][10] |
| названия нот | `config.h` | NOTE_NAMES[12] = "C", "C#", ... |
| диапазон октав | `config.h` | OCTAVE_MIN = -2, OCTAVE_MAX = 2 |

### Клавиатуры

| Что | Где |
|-----|-----|
| встроенные белые клавиши | `keyboard_map.h::WHITE_KEYS[]` |
| встроенные чёрные клавиши | `keyboard_map.h::BLACK_KEYS[]` |
| позиции чёрных клавиш для рисования | `keyboard_map.h::BK_POS[]` |
| состояние нажатой встроенной клавиши | `g_key_state[charKey]` |
| состояние нажатой матричной клавиши | `g_key_state[150 + row*10 + col]` |

---

## 🔧 Нужна функция для...

### Математика и MIDI

| Функция | Файл | Что делает |
|---------|------|-----------|
| `midiNote(rel)` | `midi_helpers.h` | Смещение в полутонах → MIDI номер |
| `noteLabel(midi)` | `midi_helpers.h` | MIDI номер → имя ноты (например "C#4") |
| `anyKeyPressed()` | `midi_helpers.h` | Проверить, нажата ли встроенная клавиша |

### Матричная клавиатура

| Функция | Файл | Что делает |
|---------|------|-----------|
| `initShiftRegisters()` | `matrix_keyboard.cpp` | Инициализировать GPIO |
| `shiftOut595(data)` | `matrix_keyboard.cpp` | Отправить 16 бит в 74HC595 |
| `shiftIn165()` | `matrix_keyboard.cpp` | Прочитать 8 бит из 74HC165 |
| `scanMatrixKeyboard()` | `matrix_keyboard.cpp` | Отсканировать матрицу → true/false |
| `getMatrixKey(row, col)` | `matrix_keyboard.cpp` | Получить состояние ячейки матрицы |

### BLE MIDI

| Функция | Файл | Что делает |
|---------|------|-----------|
| `onBleConnected()` | `ble_midi_handler.cpp` | Callback - BLE подключилось |
| `onBleDisconnected()` | `ble_midi_handler.cpp` | Callback - BLE отключилось |
| `applyBlePower()` | `ble_midi_handler.cpp` | Установить уровень мощности BLE |
| `panicAllNotesOff()` | `ble_midi_handler.cpp` | Отправить All Notes Off + очистить состояние |
| `MIDI.sendNoteOn(note, vel, ch)` | M5 library | Отправить NoteOn |
| `MIDI.sendNoteOff(note, vel, ch)` | M5 library | Отправить NoteOff |

### Рисование UI

| Функция | Файл | Что делает |
|---------|------|-----------|
| `drawStatusBar()` | `display.cpp` | Нарисовать верхнюю полоску |
| `drawMenu()` | `display.cpp` | Нарисовать экран меню |
| `drawPlay()` | `display.cpp` | Нарисовать экран игры с пианино |
| `redraw()` | `display.cpp` | Полная перерисовка всего экрана |
| `g_canvas.fillRect(x,y,w,h,color)` | M5 library | Нарисовать прямоугольник |
| `g_canvas.print(text)` | M5 library | Нарисовать текст |

### Логика меню

| Функция | Файл | Что делает |
|---------|------|-----------|
| `handleMenuInput(now)` | `menu_logic.cpp` | Обработать нажатия I/K/J/L в меню |

### Логика игры

| Функция | Файл | Что делает |
|---------|------|-----------|
| `handlePlayMode(now, changed)` | `play_mode.cpp` | Обработать встроенную + матрицу |

### Инициализация

| Функция | Файл | Что делает |
|---------|------|-----------|
| `setup()` | `main.cpp` | Инициализация всего |
| `loop()` | `main.cpp` | Главный цикл программы |

---

## 🎮 Клавиши управления

| Клавиша | Эффект | Файл с логикой |
|---------|--------|----------------|
| I (`;`) | Вверх в меню | `menu_logic.cpp` |
| K (`.`) | Вниз в меню | `menu_logic.cpp` |
| J (`,`) | Уменьшить значение | `menu_logic.cpp` |
| L (`/`) | Увеличить значение | `menu_logic.cpp` |
| Enter | Вход в режим игры / выбор | `menu_logic.cpp` |
| A-; | Белые клавиши пианино | `play_mode.cpp` |
| W,E,T,Y,U,O,P | Чёрные клавиши пианино | `play_mode.cpp` |
| Backspace | Выход из режима игры | `play_mode.cpp` |

---

## 🎨 Цвета

| Константа | Значение | Использование |
|-----------|----------|-----------------|
| `C_BG` | 0x0841 | фон экрана (тёмный синий) |
| `C_ACCENT` | 0x07FF | текст (голубой) |
| `C_ACCENT2` | 0x03EF | альтернативный текст |
| `C_WHITE` | 0xFFFF | белый |
| `C_YELLOW` | 0xFFE0 | жёлтый |
| `C_GREEN` | 0x07E0 | зелёный |
| `C_RED` | 0xF800 | красный |
| `C_ORANGE` | 0xFD00 | оранжевый |
| `C_GRAY` | 0x8410 | серый |
| `C_PIANO_W` | 0xF7DE | белая клавиша |
| `C_PIANO_B` | 0x1082 | чёрная клавиша |
| `C_PIANO_PR` | 0x07FF | нажата клавиша |

Все цвета в `config.h`

---

## 📍 Куда смотреть для...

### "Пусть матрица работает вот так..."
→ `matrix_keyboard.cpp::scanMatrixKeyboard()`

### "Нужна новая клавиша в меню..."
→ `types.h::MenuItemID` + `menu_logic.cpp` + `display.cpp`

### "Изменить цвет меню..."
→ `config.h` (цвета) + `display.cpp::drawMenu()`

### "Изменить пины..."
→ `config.h` (SR_* константы)

### "Добавить новый MIDI эффект..."
→ `play_mode.cpp::handlePlayMode()` или `play_mode.cpp::handleMatrixKeyboard()`

### "Батарея показывается неправильно..."
→ `display.cpp::drawStatusBar()` или `app_state.cpp::g_cached_bat`

### "BLE не подключается..."
→ `ble_midi_handler.cpp::onBleConnected()` + `main.cpp::setup()`

### "Встроенная клавиатура не работает..."
→ `play_mode.cpp::handleBuiltInKeyboard()` или `M5Cardputer.Keyboard`

---

## 🚀 Быстрые команды поиска

```bash
# Найти все использования переменной
grep -r "g_octave" src/

# Найти определение функции
grep "void drawMenu" src/*.cpp

# Найти все MIDI отправки
grep "MIDI.send" src/

# Найти все константы цвета
grep "C_" src/config.h

# Подсчитать строки кода в файле
wc -l src/display.cpp
```

---

## ⏰ Таймауты и интервалы

| Переменная | Значение | Использование |
|------------|----------|-----------------|
| `BAT_INTERVAL` | 10000 мс | Интервал обновления батареи |
| `NAV_DEBOUNCE` | 180 мс | Дебаунс навигации в меню |
| `delay()` в loop | 12 мс | Общая задержка в цикле |

Все в `config.h`

---

## 📈 Размеры матрицы

| Параметр | Значение | Примечание |
|----------|----------|-----------|
| Строк (MATRIX_ROWS) | 4 | физические строки матрицы |
| Столбцов (MATRIX_COLS) | 10 | физические столбцы матрицы |
| Используемых | 3 (четвёртая резерв) | для нот |
| Всего ячеек | 4 × 10 = 40 | максимум клавиш |
| Используемых ячеек | ~24 | примерно столько нот |

---

## 🎯 Примеры использования

### Отправить MIDI ноту
```cpp
int mn = midiNote(5);  // F текущей октавы
MIDI.sendNoteOn(mn, g_velocity, g_midi_ch);
// позже...
MIDI.sendNoteOff(mn, 0, g_midi_ch);
```

### Проверить, нажата встроенная клавиша
```cpp
if (M5Cardputer.Keyboard.isKeyPressed('a')) {
    // клавиша A нажата
}
```

### Получить состояние матричной ячейки
```cpp
if (getMatrixKey(0, 5)) {  // строка 0, столбец 5
    // клавиша нажата
}
```

### Установить флаг перерисовки
```cpp
g_needRedraw = true;
// в следующей итерации loop вызовется redraw()
```

---

Используй эту шпаргалку для быстрого поиска! 🎉
