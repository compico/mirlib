cmake_minimum_required(VERSION 3.16)
project(Mirlib)

set(CMAKE_CXX_STANDARD 17)

# Включаем автоматически сгенерированные пути (если файл существует)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clion_paths.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/clion_paths.cmake")
    message(STATUS "Loaded CLion paths configuration")
endif()

# Ручная настройка путей для разработки в CLion
option(ENABLE_CLION_DEVELOPMENT "Enable CLion development mode with Arduino paths" OFF)

if(ENABLE_CLION_DEVELOPMENT)
    # Пути Arduino для Arch Linux (настройте под свою систему)
    set(ARDUINO_SDK_PATH "$ENV{HOME}/.arduino15/packages/esp32/hardware/esp32/2.0.11" CACHE PATH "Arduino SDK Path")
    set(ARDUINO_LIBRARIES_PATH "$ENV{HOME}/Arduino/libraries" CACHE PATH "Arduino Libraries Path")

    if(EXISTS "${ARDUINO_SDK_PATH}/cores/esp32/Arduino.h")
        message(STATUS "Found Arduino.h at: ${ARDUINO_SDK_PATH}/cores/esp32/Arduino.h")
        include_directories("${ARDUINO_SDK_PATH}/cores/esp32")
        include_directories("${ARDUINO_SDK_PATH}/variants/esp32")
        include_directories("${ARDUINO_SDK_PATH}/tools/sdk/include")
        include_directories("${ARDUINO_SDK_PATH}/tools/sdk/include/freertos")
        include_directories("${ARDUINO_SDK_PATH}/libraries")

        # Arduino defines
        add_definitions(-DARDUINO=10815)
        add_definitions(-DESP32)
        add_definitions(-DF_CPU=240000000L)
        add_definitions(-DARDUINO_ARCH_ESP32)
        add_definitions(-DARDUINO_ESP32_DEV)
    endif()

    # Поиск CC1101 библиотеки
    set(CC1101_SEARCH_PATHS
            "${ARDUINO_LIBRARIES_PATH}/SmartRC-CC1101-Driver-Lib"
            "${ARDUINO_LIBRARIES_PATH}/ELECHOUSE_CC1101_SRC_DRV"
            "${ARDUINO_LIBRARIES_PATH}/CC1101"
    )

    foreach(cc1101_path ${CC1101_SEARCH_PATHS})
        if(EXISTS "${cc1101_path}/ELECHOUSE_CC1101_SRC_DRV.h")
            message(STATUS "Found CC1101 library at: ${cc1101_path}")
            include_directories("${cc1101_path}")
            add_definitions(-DCC1101_AVAILABLE)
            break()
        endif()
    endforeach()

    # Добавляем общие пути библиотек Arduino
    if(EXISTS "${ARDUINO_LIBRARIES_PATH}")
        include_directories("${ARDUINO_LIBRARIES_PATH}")
    endif()
endif()

# Source files
set(SOURCES
        src/MirlibBase.cpp
        src/MirlibClient.cpp
        src/MirlibServer.cpp
        src/ProtocolUtils.cpp
)

# Header files
set(HEADERS
        src/MirlibClient.h
        src/MirlibServer.h
        src/MirlibErrors.h
        src/MirlibDebug.h
        src/ProtocolTypes.h
        src/ProtocolUtils.h
        src/Commands/BaseCommand.h
        src/Commands/GetInfoCommand.h
        src/Commands/PingCommand.h
        src/Commands/ReadDateTimeCommand.h
        src/Commands/ReadInstantValueCommand.h
        src/Commands/ReadStatusCommand.h
)

add_library(Mirlib ${SOURCES} ${HEADERS})

# Public include directories
target_include_directories(Mirlib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Компилируем как C++ код с поддержкой Arduino макросов
set_target_properties(Mirlib PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
)

# Условная компиляция для разных сред
if(ARDUINO)
    # Настройки для реальной Arduino компиляции
    target_compile_definitions(Mirlib PUBLIC ARDUINO)
elseif(ENABLE_CLION_DEVELOPMENT)
    # Настройки для разработки в CLion
    target_compile_definitions(Mirlib PUBLIC
            CLION_DEVELOPMENT
            ARDUINO=10815
    )
endif()

# Installation
install(TARGETS Mirlib
        EXPORT MirlibTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(FILES ${HEADERS} DESTINATION include/Mirlib)

install(DIRECTORY src/Commands/ DESTINATION include/Mirlib/Commands
        FILES_MATCHING PATTERN "*.h"
)

# Вспомогательные target'ы для разработки
if(ENABLE_CLION_DEVELOPMENT)
    # Target для обновления путей
    add_custom_target(update_arduino_paths
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/find_arduino_paths.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Updating Arduino paths for CLion"
    )

    # Target для проверки заголовков
    add_custom_target(check_headers
            COMMAND echo "Checking Arduino headers..."
            COMMAND test -f "${ARDUINO_SDK_PATH}/cores/esp32/Arduino.h" && echo "✓ Arduino.h found" || echo "✗ Arduino.h not found"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Checking Arduino header files"
    )
endif()