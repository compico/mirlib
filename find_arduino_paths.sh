#!/bin/bash

# Скрипт для поиска Arduino путей на Arch Linux
# Запустите: ./find_arduino_paths.sh

echo "=== Поиск Arduino SDK и библиотек ==="
echo

# Поиск Arduino IDE
echo "Поиск Arduino IDE:"
ARDUINO_PATHS=(
    "/opt/arduino"
    "/usr/share/arduino"
    "/snap/arduino/current"
    "$HOME/.local/share/Arduino"
)

ARDUINO_FOUND=false
for path in "${ARDUINO_PATHS[@]}"; do
    if [[ -d "$path" ]]; then
        echo "  ✓ Найден Arduino IDE: $path"
        ARDUINO_FOUND=true
        
        # Проверяем наличие Arduino.h
        if [[ -f "$path/hardware/arduino/avr/cores/arduino/Arduino.h" ]]; then
            echo "    ✓ Arduino.h найден: $path/hardware/arduino/avr/cores/arduino/Arduino.h"
            ARDUINO_CORE_PATH="$path/hardware/arduino/avr/cores/arduino"
        fi
    fi
done

if [[ "$ARDUINO_FOUND" = false ]]; then
    echo "  ✗ Arduino IDE не найден в стандартных местах"
fi

echo

# Поиск ESP32 Arduino Core
echo "Поиск ESP32 Arduino Core:"
ESP32_BASE="$HOME/.arduino15/packages/esp32/hardware/esp32"
if [[ -d "$ESP32_BASE" ]]; then
    echo "  ✓ ESP32 пакеты найдены: $ESP32_BASE"
    
    # Ищем последнюю версию
    ESP32_LATEST=$(ls -1v "$ESP32_BASE" | tail -1)
    if [[ -n "$ESP32_LATEST" ]]; then
        ESP32_PATH="$ESP32_BASE/$ESP32_LATEST"
        echo "  ✓ Последняя версия ESP32: $ESP32_PATH"
        
        if [[ -f "$ESP32_PATH/cores/esp32/Arduino.h" ]]; then
            echo "    ✓ ESP32 Arduino.h найден: $ESP32_PATH/cores/esp32/Arduino.h"
            ARDUINO_CORE_PATH="$ESP32_PATH/cores/esp32"
        fi
    fi
else
    echo "  ✗ ESP32 Arduino Core не найден"
fi

echo

# Поиск библиотек Arduino
echo "Поиск библиотек Arduino:"
LIBRARY_PATHS=(
    "$HOME/Arduino/libraries"
    "$HOME/sketchbook/libraries"
    "$HOME/.arduino15/libraries"
)

for lib_path in "${LIBRARY_PATHS[@]}"; do
    if [[ -d "$lib_path" ]]; then
        echo "  ✓ Библиотеки найдены: $lib_path"
        lib_count=$(ls -1 "$lib_path" 2>/dev/null | wc -l)
        echo "    Количество библиотек: $lib_count"
    else
        echo "  ✗ Не найдено: $lib_path"
    fi
done

echo

# Поиск CC1101 библиотеки
echo "Поиск CC1101 библиотеки:"
CC1101_NAMES=(
    "SmartRC-CC1101-Driver-Lib"
    "ELECHOUSE_CC1101_SRC_DRV"
    "CC1101"
)

CC1101_FOUND=false
for lib_path in "${LIBRARY_PATHS[@]}"; do
    if [[ -d "$lib_path" ]]; then
        for cc1101_name in "${CC1101_NAMES[@]}"; do
            if [[ -d "$lib_path/$cc1101_name" ]]; then
                echo "  ✓ CC1101 найден: $lib_path/$cc1101_name"
                
                if [[ -f "$lib_path/$cc1101_name/ELECHOUSE_CC1101_SRC_DRV.h" ]]; then
                    echo "    ✓ Header найден: $lib_path/$cc1101_name/ELECHOUSE_CC1101_SRC_DRV.h"
                    CC1101_PATH="$lib_path/$cc1101_name"
                    CC1101_FOUND=true
                fi
            fi
        done
    fi
done

if [[ "$CC1101_FOUND" = false ]]; then
    echo "  ✗ CC1101 библиотека не найдена"
    echo "  Установите через Arduino IDE: Tools -> Manage Libraries -> SmartRC-CC1101-Driver-Lib"
fi

echo

# Создание конфигурационного файла для CLion
echo "Создание конфигурации для CLion..."

cat > clion_paths.cmake << EOF
# Автоматически сгенерированные пути для CLion
# Включите этот файл в ваш CMakeLists.txt: include(clion_paths.cmake)

EOF

if [[ -n "$ARDUINO_CORE_PATH" ]]; then
    cat >> clion_paths.cmake << EOF
# Arduino Core path
set(ARDUINO_CORE_PATH "$ARDUINO_CORE_PATH")
include_directories("\${ARDUINO_CORE_PATH}")

EOF
fi

if [[ -n "$CC1101_PATH" ]]; then
    cat >> clion_paths.cmake << EOF
# CC1101 Library path
set(CC1101_LIBRARY_PATH "$CC1101_PATH")
include_directories("\${CC1101_LIBRARY_PATH}")

EOF
fi

# Добавляем все найденные пути библиотек
for lib_path in "${LIBRARY_PATHS[@]}"; do
    if [[ -d "$lib_path" ]]; then
        cat >> clion_paths.cmake << EOF
# Arduino Libraries path
include_directories("$lib_path")
EOF
    fi
done

cat >> clion_paths.cmake << EOF

# Arduino defines
add_definitions(-DARDUINO=10815)
add_definitions(-DESP32)
add_definitions(-DF_CPU=240000000L)
add_definitions(-DARDUINO_ARCH_ESP32)
EOF

echo "✓ Конфигурация сохранена в clion_paths.cmake"
echo

# Инструкции для CLion
echo "=== Инструкции для настройки CLion ==="
echo "1. Добавьте в начало вашего CMakeLists.txt:"
echo "   include(\${CMAKE_CURRENT_SOURCE_DIR}/clion_paths.cmake)"
echo
echo "2. В CLion откройте Settings -> Build, Execution, Deployment -> CMake"
echo "3. В поле 'CMake options' добавьте:"
echo "   -DCMAKE_TOOLCHAIN_FILE=arduino-toolchain.cmake"
echo
echo "4. Нажмите 'Apply' и перезагрузите CMake проект"
echo

# Создание простого include файла
cat > arduino_includes.h << EOF
// Автоматически сгенерированный файл для включения Arduino заголовков
#pragma once

#ifdef ARDUINO
    #include <Arduino.h>
#else
    // Заглушки для CLion IntelliSense
    #include <stdint.h>
    #include <stddef.h>
    
    #define HIGH 1
    #define LOW 0
    #define INPUT 0
    #define OUTPUT 1
    #define F(string_literal) (string_literal)
    
    typedef uint8_t byte;
    
    void delay(unsigned long ms);
    unsigned long millis();
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t val);
    int digitalRead(uint8_t pin);
    
    class SerialClass {
    public:
        void begin(unsigned long baud);
        void print(const char* str);
        void println(const char* str);
        void print(int val);
        void println(int val);
        void print(int val, int format);
        void println(int val, int format);
    };
    
    extern SerialClass Serial;
#endif

#ifdef CC1101_AVAILABLE
    #include <ELECHOUSE_CC1101_SRC_DRV.h>
#else
    // Заглушки для CC1101
    class CC1101Class {
    public:
        void setSpiPin(int cs, int mosi = -1, int miso = -1, int gdo0 = -1, int gdo2 = -1) {}
        bool getCC1101() { return true; }
        void Init() {}
        void setCCMode(int mode) {}
        void setModulation(int mod) {}
        void setMHZ(float freq) {}
        void setDeviation(float dev) {}
        void setChannel(int ch) {}
        void setChsp(float chsp) {}
        void setRxBW(float bw) {}
        void setDRate(float rate) {}
        void setPA(int pa) {}
        void setSyncMode(int mode) {}
        void setPktFormat(int format) {}
        void setLengthConfig(int config) {}
        void setPacketLength(int len) {}
        void setCrc(int crc) {}
        void setWhiteData(int white) {}
        void setAddressCheck(int check) {}
        void setAddr(int addr) {}
        void setDcFilterOff(int filter) {}
        void SetTx() {}
        void SetRx() {}
        void SendData(uint8_t* data, int len) {}
        bool CheckRxFifo(int timeout) { return false; }
        int ReceiveData(uint8_t* data) { return 0; }
    };
    
    extern CC1101Class ELECHOUSE_cc1101;
#endif
EOF

echo "✓ Создан arduino_includes.h с заглушками для IntelliSense"
echo "  Включите его в ваши файлы вместо прямого #include <Arduino.h>"