#ifndef BLE_MIDI_H
#define BLE_MIDI_H

#include <Arduino.h>

typedef void (*BleMidiCallback)(uint8_t ccId, uint8_t value);

void bleMidiInit(BleMidiCallback callback);
void bleMidiDeinit();
void bleMidiLoop();
bool isBleMidiConnected();

#endif
