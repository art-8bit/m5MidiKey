#include "midi_helpers.h"
#include "keyboard_map.h"

bool anyKeyPressed() {
    for (int i = 0; i < WK_COUNT; i++)
        if (g_key_state[WHITE_KEYS[i].key]) return true;
    for (int i = 0; i < BK_COUNT; i++)
        if (g_key_state[BLACK_KEYS[i].key]) return true;
    return false;
}
