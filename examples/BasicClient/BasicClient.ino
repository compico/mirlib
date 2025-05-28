/*
 * BasicClient.ino
 * 
 * Простой пример клиента для чтения счетчика со всеми поддерживаемыми командами
 * Демонстрирует основные возможности библиотеки Mirlib для взаимодействия с электросчетчиками
 * 
 * Подключение CC1101:
 * VCC -> 3.3V
 * GND -> GND
 * MOSI -> GPIO 23 (ESP32) / D7 (ESP8266)
 * MISO -> GPIO 19 (ESP32) / D6 (ESP8266)
 * SCK -> GPIO 18 (ESP32) / D5 (ESP8266)
 * CS -> GPIO 5 (ESP32/ESP8266)
 * GDO0 -> GPIO 2 (опционально)
 * GDO2 -> GPIO 4 (опционально)
 */

#include <Mirlib.h>

// Конфигурация
const int CS_PIN = 5; // Пин Chip Select для CC1101
const int GDO0_PIN = 2; // Пин GDO0 (опционально)
const int GDO2_PIN = 4; // Пин GDO2 (опционально)
const uint16_t METER_ADDRESS = 0x1234; // Адрес счетчика для опроса
const uint32_t METER_PASSWORD = 0x12345678; // Пароль счетчика (если требуется)

// Инициализация протокола в клиентском режиме
Mirlib protocol(Mirlib::CLIENT, 0xFFFF);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Mirlib BasicClient Example ===");
    Serial.println("Инициализация протокола и CC1101...");

    // Инициализация CC1101
    if (!protocol.begin(CS_PIN, GDO0_PIN, GDO2_PIN)) {
        Serial.println("❌ Ошибка инициализации CC1101!");
        Serial.println("Проверьте подключение модуля:");
        Serial.println("- Подключение проводов SPI");
        Serial.println("- Пин CS = " + String(CS_PIN));
        while (1) {
            delay(1000);
        }
    }

    Serial.println("✅ CC1101 инициализирован успешно");

    // Включение отладочного режима
    protocol.setDebugMode(true);
    protocol.setTimeout(5000);

    Serial.println("🔍 Начинаем опрос счетчика по адресу 0x" + String(METER_ADDRESS, HEX));
    Serial.println();
}

void loop() {
    Serial.println("==========================================");
    Serial.println("🚀 Начинаем цикл опроса счетчика");
    Serial.println("==========================================");

    // 1. Ping - проверка связи
    Serial.println("\n📡 1. Ping - проверка связи");
    Serial.println("----------------------------");

    PingCommand pingCmd;
    if (protocol.sendCommand(&pingCmd, METER_ADDRESS)) {
        Serial.println("✅ Ping успешен!");
        Serial.println("   📋 Версия прошивки: 0x" + String(pingCmd.getFirmwareVersion(), HEX));
        Serial.println("   📍 Адрес устройства: 0x" + String(pingCmd.getDeviceAddress(), HEX));
    } else {
        Serial.println("❌ Ошибка Ping: " + String(protocol.getLastError()));
        delay(5000);
        return;
    }

    // 2. GetInfo - получение информации о счетчике
    Serial.println("\n📊 2. GetInfo - информация о счетчике");
    Serial.println("--------------------------------------");

    GetInfoCommand infoCmd;
    if (protocol.sendCommand(&infoCmd, METER_ADDRESS)) {
        Serial.println("✅ GetInfo успешен!");
        Serial.println("   🔧 ID платы: 0x" + String(infoCmd.getBoardId(), HEX));
        Serial.println("   💾 Версия прошивки: 0x" + String(infoCmd.getFirmwareVersion(), HEX));

        GenerationInfo genInfo = infoCmd.getGenerationInfo();
        Serial.print("   🏭 Поколение: ");
        if (genInfo.isOldGeneration) {
            Serial.println("Старое");
        } else if (genInfo.isTransitionGeneration) {
            Serial.println("Переходное");
        } else if (genInfo.isNewGeneration) {
            Serial.println("Новое");
        } else {
            Serial.println("Неизвестное");
        }

        Serial.println("   ⚡ Поддержка 100А: " + String(infoCmd.supports100A() ? "Да" : "Нет"));

        if (infoCmd.isNewGeneration()) {
            Serial.println("   💡 Управление освещением: " + String(infoCmd.hasStreetLightingControl() ? "Да" : "Нет"));
        }

        // Автоматическое определение поколения для дальнейших команд
        if (protocol.autoDetectGeneration(METER_ADDRESS)) {
            Serial.println("   🎯 Поколение автоматически определено для протокола");
        }
    } else {
        Serial.println("❌ Ошибка GetInfo: " + String(protocol.getLastError()));
    }

    // 3. ReadDateTime - чтение даты и времени
    Serial.println("\n🕐 3. ReadDateTime - дата и время");
    Serial.println("----------------------------------");

    ReadDateTimeCommand dateTimeCmd;
    if (protocol.sendCommand(&dateTimeCmd, METER_ADDRESS)) {
        Serial.println("✅ ReadDateTime успешен!");

        char timeStr[20];
        dateTimeCmd.formatDateTime(timeStr);
        Serial.println("   📅 Время счетчика: " + String(timeStr));
        Serial.println("   📆 День недели: " + String(dateTimeCmd.getDayOfWeekName()));

        if (dateTimeCmd.isDateTimeValid()) {
            Serial.println("   ✅ Время корректно");
        } else {
            Serial.println("   ⚠️  Время некорректно");
        }
    } else {
        Serial.println("❌ Ошибка ReadDateTime: " + String(protocol.getLastError()));
    }

    // 4. ReadStatus - чтение состояния счетчика
    Serial.println("\n📈 4. ReadStatus - состояние счетчика");
    Serial.println("-------------------------------------");

    // Повторно получаем информацию для определения поколения
    GetInfoCommand infoForStatus;
    if (protocol.sendCommand(&infoForStatus, METER_ADDRESS)) {
        ReadStatusCommand statusCmd;
        statusCmd.setGeneration(infoForStatus.getBoardId(), 0x32);
        statusCmd.setRequest(ACTIVE_FORWARD); // Активная энергия прямого направления

        if (protocol.sendCommand(&statusCmd, METER_ADDRESS)) {
            Serial.println("✅ ReadStatus успешен!");

            if (statusCmd.isOldGeneration()) {
                auto response = statusCmd.getOldResponse();
                Serial.println("   📊 Общая энергия: " + String(response.totalEnergy));
                Serial.println("   🎯 Активный тариф: " + String(response.configByte.activeTariff));
                Serial.println("   📍 Знаков на дисплее: " + String(response.configByte.displayDigits + 6));
                Serial.println("   🔢 Коэффициент деления: " + String(response.divisionCoeff));

                Serial.println("   💰 Тарифы:");
                for (int i = 0; i < 4; i++) {
                    Serial.println("      Тариф " + String(i + 1) + ": " + String(response.tariffValues[i]));
                }
            } else {
                auto response = statusCmd.getNewResponse();
                Serial.println("   📊 Общая активная энергия: " + String(response.totalActive));
                Serial.println("   📈 Общая полная энергия: " + String(response.totalFull));
                Serial.println("   🎯 Активный тариф: " + String(response.configByte.activeTariff));
                Serial.println("   ⚡ Коэфф. трансформации тока: " + String(response.currentTransformCoeff));
                Serial.println("   🔌 Коэфф. трансформации напряжения: " + String(response.voltageTransformCoeff));

                Serial.println("   💰 Тарифы:");
                for (int i = 0; i < 4; i++) {
                    Serial.println("      Тариф " + String(i + 1) + ": " + String(response.tariffValues[i]));
                }
            }
        } else {
            Serial.println("❌ Ошибка ReadStatus: " + String(protocol.getLastError()));
        }
    }

    // 5. ReadInstantValue - мгновенные значения (только для переходного/нового поколения)
    Serial.println("\n⚡ 5. ReadInstantValue - мгновенные значения");
    Serial.println("--------------------------------------------");

    GetInfoCommand infoForInstant;
    if (protocol.sendCommand(&infoForInstant, METER_ADDRESS)) {
        GenerationInfo genInfo = infoForInstant.getGenerationInfo();

        if (genInfo.isOldGeneration) {
            Serial.println("⚠️  ReadInstantValue не поддерживается старым поколением");
        } else {
            ReadInstantValueCommand instantCmd;
            instantCmd.setGeneration(infoForInstant.getBoardId(), 0x32);
            instantCmd.setRequest(GROUP_BASIC); // Основные параметры

            if (protocol.sendCommand(&instantCmd, METER_ADDRESS)) {
                Serial.println("✅ ReadInstantValue успешен!");

                if (instantCmd.isTransitionGeneration()) {
                    auto response = instantCmd.getTransitionResponse();
                    Serial.println("   🔌 Напряжения:");
                    Serial.println("      Фаза A: " + String(response.getVoltageA(), 2) + " В");
                    Serial.println("      Фаза B: " + String(response.getVoltageB(), 2) + " В");
                    Serial.println("      Фаза C: " + String(response.getVoltageC(), 2) + " В");

                    Serial.println("   ⚡ Токи:");
                    Serial.println("      Фаза A: " + String(response.getCurrentA(), 3) + " А");
                    Serial.println("      Фаза B: " + String(response.getCurrentB(), 3) + " А");
                    Serial.println("      Фаза C: " + String(response.getCurrentC(), 3) + " А");

                    Serial.println("   📊 Другие параметры:");
                    Serial.println("      Частота: " + String(response.getFrequencyHz(), 2) + " Гц");
                    Serial.println("      cos φ: " + String(response.getCosPhiValue(), 3));
                    Serial.println("      Поддержка 100А: " + String(response.is100ASupport ? "Да" : "Нет"));
                } else if (instantCmd.isNewGeneration()) {
                    auto response = instantCmd.getNewBasicResponse();
                    Serial.println("   🔌 Напряжения:");
                    Serial.println("      Фаза A: " + String(response.getVoltageA(), 2) + " В");
                    Serial.println("      Фаза B: " + String(response.getVoltageB(), 2) + " В");
                    Serial.println("      Фаза C: " + String(response.getVoltageC(), 2) + " В");

                    Serial.println("   ⚡ Токи:");
                    Serial.println("      Фаза A: " + String(response.getCurrentA(), 3) + " А");
                    Serial.println("      Фаза B: " + String(response.getCurrentB(), 3) + " А");
                    Serial.println("      Фаза C: " + String(response.getCurrentC(), 3) + " А");

                    Serial.println("   🔋 Мощности:");
                    Serial.println("      Активная: " + String(response.getActivePowerKW(), 3) + " кВт");
                    Serial.println("      Реактивная: " + String(response.getReactivePowerKvar(), 3) + " квар");

                    Serial.println("   📊 Другие параметры:");
                    Serial.println("      Частота: " + String(response.getFrequencyHz(), 2) + " Гц");
                    Serial.println("      cos φ: " + String(response.getCosPhiValue(), 3));
                }
            } else {
                Serial.println("❌ Ошибка ReadInstantValue: " + String(protocol.getLastError()));
            }
        }
    }

    // Заключение цикла
    Serial.println("\n🏁 Цикл опроса завершен");
    Serial.println("Ожидание 10 секунд до следующего цикла...");
    Serial.println("==========================================\n");

    delay(10000); // Пауза 10 секунд
}
