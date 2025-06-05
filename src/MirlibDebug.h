#ifndef MIRLIBDEBUG_H
#define MIRLIBDEBUG_H

// Определение платформы
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
  #define MIRLIB_PLATFORM_AVR
  #include <Arduino.h>
#elif defined(ESP32)
  #define MIRLIB_PLATFORM_ESP32
  #include <Arduino.h>
#elif defined(__GNUC__) && !defined(ARDUINO)
  #define MIRLIB_PLATFORM_GCC
  #include <stdio.h>
  #include <stdint.h>
  #include <stddef.h>
  #include <string.h>
#else
  #error "Mirlib: неподдерживаемая платформа!"
#endif

// Реализации для Arduino AVR (UNO/NANO)
#ifdef MIRLIB_PLATFORM_AVR

inline void MIRLIB_DEBUG_PRINT_HEX(const uint8_t *data, size_t size, const char *title = nullptr) {
  if (title && strlen(title) > 0) {
    Serial.print(title);
    Serial.print(F(": "));
  }

  for (size_t i = 0; i < size; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

inline void MIRLIB_DEBUG_PRINT_PACKET(const PacketData &packet, const char *title) {
  Serial.print(title);
  Serial.print(F(": "));
  MIRLIB_DEBUG_PRINT_HEX(packet.rawPacket, packet.rawSize, nullptr);
}

#define MIRLIB_DEBUG_PRINT(msg)        \
  do {                                 \
    Serial.print(F("[Mirlib] "));      \
    Serial.println(msg);            \
  } while (0)

#define MIRLIB_DEBUG_PRINT_ERROR(code) \
  do {                                 \
    Serial.print(F("Ошибка #"));       \
    Serial.println(code);              \
  } while (0)

// Реализации для ESP32
#elif defined(MIRLIB_PLATFORM_ESP32)

inline void MIRLIB_DEBUG_PRINT_HEX(const uint8_t *data, size_t size, const char *title = nullptr) {
  if (title && strlen(title) > 0) {
    Serial.print(title);
    Serial.print(": ");
  }

  for (size_t i = 0; i < size; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

inline void MIRLIB_DEBUG_PRINT_PACKET(const PacketData &packet, const char *title) {
  Serial.print(title);
  Serial.print(": ");
  MIRLIB_DEBUG_PRINT_HEX(packet.rawPacket, packet.rawSize, nullptr);
}

#define MIRLIB_DEBUG_PRINT(msg)        \
  do {                                 \
    Serial.print("[Mirlib] ");         \
    Serial.println(msg);               \
  } while (0)

#define MIRLIB_DEBUG_PRINT_ERROR(code) \
  do {                                 \
    Serial.print("Ошибка #");          \
    Serial.println(code);              \
  } while (0)

// Реализации для GCC (desktop/Linux)
#elif defined(MIRLIB_PLATFORM_GCC)

inline void MIRLIB_DEBUG_PRINT_HEX(const uint8_t *data, size_t size, const char *title = nullptr) {
  if (title && strlen(title) > 0) {
    printf("%s: ", title);
  }

  for (size_t i = 0; i < size; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
}

inline void MIRLIB_DEBUG_PRINT_PACKET(const PacketData &packet, const char *title) {
  printf("%s: ", title);
  MIRLIB_DEBUG_PRINT_HEX(packet.rawPacket, packet.rawSize, nullptr);
}

#define MIRLIB_DEBUG_PRINT(msg)        \
  do {                                 \
    printf("[Mirlib] %s\n", msg);      \
  } while (0)

#define MIRLIB_DEBUG_PRINT_ERROR(code) \
  do {                                 \
    printf("Ошибка #%d\n", code);      \
  } while (0)

#endif

#endif // MIRLIBDEBUG_H