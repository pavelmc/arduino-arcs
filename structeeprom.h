#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

template <class T> void eeWriteStruct(int ee, const T& value) {
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) EEPROM.write(ee++, *p++);
}

template <class T> void eeReadStruct(int ee, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) *p++ = EEPROM.read(ee++);
}
