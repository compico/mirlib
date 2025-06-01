#ifndef MIRLIBDEBUG_H
#define MIRLIBDEBUG_H

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
  #define MIRLIB_DEBUG_PRINT(msg) \
  do {                            \
    Serial.print(F("[Mirlib] ")); \
    Serial.println(F(msg));       \
  } while (0)

#define MIRLIB_DEBUG_PRINT_PACKAGE() \
   do { \
        \
  } while (0)

#define MIRLIB_DEBUG_PRINT_ERROR(code) \
  do { \
    Serial.print(F("Ошибка #")); \
    Serial.println(code); \
  } while (0)
#elif defined(ESP32)
  #define MIRLIB_DEBUG_PRINT(msg) \
  do {                            \
    Serial.print("[Mirlib] "); \
    Serial.println(msg);       \
  } while (0)

  #define MIRLIB_DEBUG_PRINT_ERROR(code) \
  do { \
    Serial.print("Ошибка #"); \
    Serial.println(code); \
  } while (0)
#else
  #error "Mirlib: неподдерживаемая платформа!"
#endif

#endif //MIRLIBDEBUG_H
