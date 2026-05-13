#ifndef MIDI_HELPERS_H
#define MIDI_HELPERS_H

#include "app_state.h"
#include "config.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ MIDI
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Конвертировать относительное смещение полутонов в абсолютный MIDI номер ноты
 * 
 * @param rel - смещение в полутонах от C текущей октавы
 * @return MIDI номер (0..127)
 */
inline int midiNote(int8_t rel) {
    return constrain(60 + g_octave * 12 + rel, 0, 127);
}

/**
 * Получить имя ноты из MIDI номера
 * Например: 60 → "C4", 61 → "C#4", 72 → "C5"
 * 
 * @param midi - MIDI номер ноты
 * @return строка с названием ноты
 */
inline String noteLabel(int midi) {
    int oct = (midi / 12) - 1;
    int n   = midi % 12;
    return String(NOTE_NAMES[n]) + String(oct);
}

/**
 * Проверить, нажата ли какая-нибудь встроенная клавиша
 * 
 * @return true если хотя бы одна белая или чёрная клавиша нажата
 */
bool anyKeyPressed();

#endif // MIDI_HELPERS_H
