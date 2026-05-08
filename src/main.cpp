#include <M5Cardputer.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>

BLEMIDI_CREATE_DEFAULT_INSTANCE();

struct KeyNote {
    char key;
    uint8_t note;
    bool isPlaying; // Флаг, чтобы знать, что нота уже звучит
};

KeyNote keyMap[] = {
    {'a', 60, false}, {'s', 62, false}, {'d', 64, false}, {'f', 65, false}, 
    {'g', 67, false}, {'h', 69, false}, {'j', 71, false}, {'k', 72, false},
    {'w', 61, false}, {'e', 63, false}, {'t', 66, false}, {'y', 68, false}, {'u', 70, false}
};
const int mapSize = sizeof(keyMap) / sizeof(KeyNote);

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);
    MIDI.begin(MIDI_CHANNEL_OMNI);

    M5Cardputer.Display.println("MIDI Keyboard Ready");
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
        for (int i = 0; i < mapSize; i++) {
            // Проверяем, нажата ли конкретная клавиша в данный момент
            bool pressed = M5Cardputer.Keyboard.isKeyPressed(keyMap[i].key);

            if (pressed && !keyMap[i].isPlaying) {
                // Нота только что нажата
                MIDI.sendNoteOn(keyMap[i].note, 127, 1);
                keyMap[i].isPlaying = true;
                M5Cardputer.Display.fillRect(0, 40, 240, 20, BLACK);
                M5Cardputer.Display.setCursor(0, 40);
                M5Cardputer.Display.printf("ON: %d", keyMap[i].note);
            } 
            else if (!pressed && keyMap[i].isPlaying) {
                // Нота только что отпущена
                MIDI.sendNoteOff(keyMap[i].note, 0, 1);
                keyMap[i].isPlaying = false;
                M5Cardputer.Display.fillRect(0, 40, 240, 20, BLACK);
                M5Cardputer.Display.setCursor(0, 40);
                M5Cardputer.Display.printf("OFF: %d", keyMap[i].note);
            }
        }
    }
  delay(5); // Небольшая задержка для снижения нагрузки на процессор
}
