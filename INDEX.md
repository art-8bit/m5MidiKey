# 📑 Индекс файлов проекта

## 📖 Документация (начни отсюда)

| Файл | Назначение | Читать в первую очередь? |
|------|-----------|--------------------------|
| **README.md** | 🎯 Главный файл с навигацией | ⭐⭐⭐ ДА! |
| **MIGRATION.md** | 📦 Как перейти на новую версию (6 минут) | ⭐⭐⭐ ДА! |
| **README_STRUCTURE.md** | 🏗️ Полное описание структуры проекта | ⭐⭐ Потом |
| **ARCHITECTURE.md** | 📊 Диаграммы и граф зависимостей | ⭐⭐ Потом |
| **CHEATSHEET.md** | ⚡ Шпаргалка для быстрого поиска | ⭐⭐⭐ При редактировании |

---

## 🎯 Файлы исходного кода по группам

### 🚀 Главный файл (начальная точка)

```
main.cpp                    90 строк
├─ setup()      - инициализация всего
├─ loop()       - главный цикл программы
└─ # включает все остальные модули
```

### ⚙️ Конфигурация и типы (не меняется часто)

```
config.h                    80 строк
├─ SCREEN_W, SCREEN_H       - размеры экрана
├─ все цвета (C_*)         - цветовая палитра
├─ SR_DATA_OUT, SR_CLOCK   - пины сдвиговых регистров
├─ MATRIX_ROWS, MATRIX_COLS - размеры матрицы
├─ MATRIX_MAP[4][10]       - маппинг матрицы → MIDI
├─ NOTE_NAMES[12]          - названия нот
└─ VELOCITY_MIN, OCTAVE_MIN - диапазоны параметров

types.h                     40 строк
├─ enum AppMode            - MENU, PLAY
├─ enum MenuItemID         - пункты меню
├─ enum KeyboardSource     - BUILT_IN, MATRIX, BOTH
├─ struct BLEPwrEntry      - таблица мощности BLE
└─ struct NoteKey          - маппинг клавиши
```

### 🌐 Глобальное состояние (часто используется)

```
app_state.h                 50 строк
└─ extern g_appMode, g_octave, g_velocity, ... (все переменные)

app_state.cpp               30 строк
└─ Определение и инициализация глобальных переменных
```

### ⌨️ Ввод и аппаратура

```
keyboard_map.h              30 строк
├─ WHITE_KEYS[]            - A S D F G H J K L ;
├─ BLACK_KEYS[]            - W E T Y U O P
└─ BK_POS[]                - позиции для рисования

matrix_keyboard.h           35 строк
└─ initShiftRegisters(), shiftOut595(), shiftIn165(),
   scanMatrixKeyboard(), getMatrixKey()

matrix_keyboard.cpp         50 строк
└─ Реализация работы с 74HC595 + 74HC165
```

### 🎵 MIDI и звук

```
midi_helpers.h              20 строк
└─ midiNote(), noteLabel(), anyKeyPressed()

midi_helpers.cpp            10 строк
└─ Реализация помощников

ble_midi_handler.h          30 строк
├─ BLE_PWR_TABLE[]         - уровни мощности
└─ onBleConnected(), onBleDisconnected(),
   applyBlePower(), panicAllNotesOff()

ble_midi_handler.cpp        40 строк
└─ Реализация BLE управления
```

### 🎨 Интерфейс и логика

```
display.h                   25 строк
└─ drawStatusBar(), drawMenu(), drawPlay(), redraw()

display.cpp                 230 строк (самый большой!)
├─ drawStatusBar()          - верхняя полоса
├─ drawMenu()               - экран меню
├─ drawPlay()               - экран игры с пианино
└─ redraw()                 - полная перерисовка

menu_logic.h                20 строк
└─ handleMenuInput()

menu_logic.cpp              70 строк
└─ Обработка I/K/J/L/Enter в режиме меню

play_mode.h                 15 строк
└─ handlePlayMode()

play_mode.cpp               150 строк
├─ handleBuiltInKeyboard()  - встроенная клавиатура
├─ handleMatrixKeyboard()   - матричная клавиатура
└─ handlePlayMode()         - объединение обеих
```

### 🔧 Конфигурация проекта

```
platformio.ini              - Конфиг PlatformIO
└─ board, platform, libraries, upload settings
```

---

## 📊 Статистика

```
Всего файлов:     24 файла
  Исходный код:   18 файлов (.h + .cpp)
  Документация:    5 файлов (.md)
  Конфигурация:    1 файл (.ini)

Строк кода:       ~860 строк
  Заголовки:      ~250 строк
  Реализация:     ~610 строк
  
Самые большие файлы:
  1. display.cpp        230 строк (рисование UI)
  2. play_mode.cpp      150 строк (логика игры)
  3. matrix_keyboard    100 строк (матричная клавиатура)
  4. config.h            80 строк (константы)
  5. ble_midi_handler    70 строк (BLE управление)

Самые маленькие файлы:
  1. midi_helpers.cpp    10 строк (очень полезный!)
  2. types.h             40 строк (типы)
  3. play_mode.h         15 строк (декларации)
  4. menu_logic.h        20 строк (декларации)
```

---

## 🗺️ Как скопировать файлы в свой проект

### Шаг 1: Скопировать в папку src/
```bash
# Скопируй все файлы в папку src/ твоего PlatformIO проекта
src/
├── main.cpp
├── config.h
├── types.h
├── app_state.h
├── app_state.cpp
├── matrix_keyboard.h
├── matrix_keyboard.cpp
├── keyboard_map.h
├── midi_helpers.h
├── midi_helpers.cpp
├── ble_midi_handler.h
├── ble_midi_handler.cpp
├── display.h
├── display.cpp
├── menu_logic.h
├── menu_logic.cpp
├── play_mode.h
└── play_mode.cpp
```

### Шаг 2: Обновить platformio.ini
```bash
# Скопируй содержимое platformio.ini (если оно отличается)
```

### Шаг 3: Компилировать
```bash
pio build
pio upload
```

---

## 🎯 Какой файл читать в зависимости от задачи

### "Нужно разобраться в проекте" (30 мин)
1. README.md - обзор
2. README_STRUCTURE.md - описание модулей
3. ARCHITECTURE.md - диаграммы
4. Открой main.cpp - смотри, как компактно!

### "Нужно что-то найти" (2 мин)
1. CHEATSHEET.md - таблица переменных и функций
2. Используй Ctrl+F для поиска по таблице

### "Нужно что-то изменить" (10 мин)
1. CHEATSHEET.md - найди где это находится
2. Открой нужный файл в редакторе
3. Измени нужное
4. `pio build` для проверки

### "Нужно добавить новую функцию" (30 мин)
1. ARCHITECTURE.md - раздел "Как добавить новую функцию"
2. README_STRUCTURE.md - "Как редактировать проект"
3. Напиши код
4. `pio build` для проверки

---

## 📚 Рекомендуемый порядок чтения

### День 1: Понимание архитектуры (1 час)
```
README.md                    (10 мин) ← начни отсюда
  ↓
MIGRATION.md                 (5 мин)
  ↓
README_STRUCTURE.md          (20 мин) основные концепции
  ↓
ARCHITECTURE.md              (15 мин) видишь граф зависимостей
  ↓
main.cpp                     (10 мин) смотришь простоту!
```

### День 2: Понимание деталей (1-2 часа)
```
CHEATSHEET.md                (10 мин) таблицы переменных
  ↓
app_state.h/cpp              (5 мин) где всё состояние
  ↓
config.h                     (5 мин) где все константы
  ↓
display.cpp                  (20 мин) понимаешь UI
  ↓
play_mode.cpp                (20 мин) понимаешь логику игры
  ↓
matrix_keyboard.cpp          (15 мин) понимаешь матрицу
```

### День 3+: Редактирование и расширение
```
Используй CHEATSHEET.md для быстрого поиска
Читай нужный файл полностью
Делай изменения
Компилируй и тестируй
```

---

## 🔍 Быстрые команды поиска в коде

```bash
# Найти все использования переменной
grep -r "g_octave" .

# Найти определение функции
grep "void drawMenu" *.cpp

# Найти все MIDI отправки
grep "MIDI.send" *.cpp

# Найти все цветовые константы
grep "^static constexpr uint16_t C_" config.h

# Подсчитать строки в файле
wc -l display.cpp

# Показать только имена функций
grep "^void\|^bool\|^int " *.cpp | cut -d: -f1,2
```

---

## ✅ Чек-лист перед первой компиляцией

- [ ] Скопировал все файлы (.h и .cpp!) в src/
- [ ] Обновил platformio.ini
- [ ] В src/ есть 18 файлов исходного кода
- [ ] Запустил `pio build`
- [ ] Ошибок нет или только warnings
- [ ] Запустил `pio upload`
- [ ] Программа работает на плате

---

## 🎯 Типичные ошибки

| Ошибка | Причина | Решение |
|--------|---------|---------|
| `undefined reference to ...` | Не скопировал .cpp файл | Скопируй ВСЕ файлы |
| `multiple definition of ...` | Дублируется определение | Проверь app_state.cpp |
| `no matching function` | Неправильный сигнатура | Смотри .h файл |
| Компилируется но не работает | Что-то не инициализировано | Проверь setup() |

Подробнее в MIGRATION.md - "Если что-то не работает"

---

## 🚀 После успешной компиляции

1. Тестируй на плате
2. Используй CHEATSHEET.md при редактировании
3. Начни добавлять свои функции
4. Делись улучшениями! 🎉

---

## 📖 Дополнительные ресурсы

- **[README.md](README.md)** - главный файл с навигацией
- **[CHEATSHEET.md](CHEATSHEET.md)** - шпаргалка (ОЧЕНЬ ПОЛЕЗНАЯ!)
- **[MIGRATION.md](MIGRATION.md)** - как переехать на новую версию
- **[README_STRUCTURE.md](README_STRUCTURE.md)** - описание структуры
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - диаграммы и примеры

---

**Версия:** 2.0 (Модульная архитектура)  
**Обновлено:** 2025  
**Язык:** C++ (Arduino)  
**Платформа:** M5Stack Cardputer, ESP32-S3  

Спасибо за использование этого проекта! 🎉
