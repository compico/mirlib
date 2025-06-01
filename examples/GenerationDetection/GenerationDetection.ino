/*
 * GenerationDetection.ino
 * 
 * Пример автоматического определения поколения счетчиков и демонстрации
 * совместимости команд с различными поколениями устройств
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

#include <MirlibClient.h>

// Конфигурация
const int CS_PIN = 5;
const int GDO0_PIN = 2;
const int GDO2_PIN = 4;

// Список адресов для сканирования
const uint16_t SCAN_ADDRESSES[] = {
    0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x1111, 0x2222, 0x3333, 0x4444,
    0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC
};
const int SCAN_COUNT = sizeof(SCAN_ADDRESSES) / sizeof(SCAN_ADDRESSES[0]);

// Инициализация протокола
MirlibClient protocol(0xFFFF);

// Структура для хранения информации об обнаруженных устройствах
struct DeviceInfo {
    uint16_t address;
    bool isOnline;
    uint8_t boardId;
    uint16_t firmwareVersion;
    MirlibClient::Generation generation;
    String generationName;
    bool supports100A;
    bool hasStreetLighting;
    bool supportsPing;
    bool supportsGetInfo;
    bool supportsReadDateTime;
    bool supportsReadStatus;
    bool supportsReadInstantValue;
};

std::vector<DeviceInfo> discoveredDevices;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Mirlib Generation Detection Example ===");
    Serial.println("🔍 Автоматическое определение поколений счетчиков");

    // Инициализация CC1101
    if (!protocol.begin(CS_PIN, GDO0_PIN, GDO2_PIN)) {
        Serial.println("❌ Ошибка инициализации CC1101!");
        while (1) delay(1000);
    }
    Serial.println("✅ CC1101 инициализирован");

    // Настройка протокола
    protocol.setTimeout(2000); // Короткий таймаут для быстрого сканирования

    Serial.println("\n🔎 Начинаем сканирование и определение поколений...");
    Serial.println("Адресов для проверки: " + String(SCAN_COUNT));
    Serial.println("=========================================");

    // Сканирование устройств
    scanDevices();

    // Анализ обнаруженных устройств
    analyzeDevices();

    // Тестирование совместимости команд
    testCommandCompatibility();

    Serial.println("\n🏁 Анализ завершен. Введите 'help' для списка команд.");
}

void loop() {
    handleSerialCommands();
    delay(100);
}

void scanDevices() {
    Serial.println("📡 Сканирование устройств...");

    int foundCount = 0;

    for (int i = 0; i < SCAN_COUNT; i++) {
        uint16_t address = SCAN_ADDRESSES[i];

        Serial.print("   Проверка 0x" + String(address, HEX) + "... ");

        DeviceInfo device;
        device.address = address;
        device.isOnline = false;
        device.boardId = 0;
        device.firmwareVersion = 0;
        device.generation = MirlibClient::UNKNOWN;
        device.generationName = "Неизвестно";
        device.supports100A = false;
        device.hasStreetLighting = false;

        // Попытка Ping
        PingCommand pingCmd;
        if (protocol.sendCommand(&pingCmd, address)) {
            device.isOnline = true;
            device.firmwareVersion = pingCmd.getFirmwareVersion();
            device.supportsPing = true;
            foundCount++;

            Serial.print("✅ Онлайн ");

            // Определение поколения через GetInfo
            if (determineGeneration(device)) {
                Serial.println("(" + device.generationName + ")");
            } else {
                Serial.println("(поколение не определено)");
            }

            discoveredDevices.push_back(device);
        } else {
            Serial.println("❌ Недоступен");
        }

        delay(200); // Пауза между проверками
    }

    Serial.println("\n📊 Сканирование завершено");
    Serial.println("   Найдено устройств: " + String(foundCount) + " из " + String(SCAN_COUNT));
}

bool determineGeneration(DeviceInfo &device) {
    GetInfoCommand infoCmd;

    if (protocol.sendCommand(&infoCmd, device.address)) {
        device.supportsGetInfo = true;
        device.boardId = infoCmd.getBoardId();
        device.supports100A = infoCmd.supports100A();

        if (infoCmd.isNewGeneration()) {
            device.hasStreetLighting = infoCmd.hasStreetLightingControl();
        }

        GenerationInfo genInfo = infoCmd.getGenerationInfo();

        if (genInfo.isOldGeneration) {
            device.generation = MirlibClient::OLD_GENERATION;
            device.generationName = "Старое";
        } else if (genInfo.isTransitionGeneration) {
            device.generation = MirlibClient::TRANSITION_GENERATION;
            device.generationName = "Переходное";
        } else if (genInfo.isNewGeneration) {
            device.generation = MirlibClient::NEW_GENERATION;
            device.generationName = "Новое";
        } else {
            device.generation = MirlibClient::UNKNOWN;
            device.generationName = "Неизвестно";
            return false;
        }

        return true;
    } else {
        device.supportsGetInfo = false;
        return false;
    }
}

void analyzeDevices() {
    if (discoveredDevices.empty()) {
        Serial.println("⚠️ Устройства не найдены");
        return;
    }

    Serial.println("\n📊 АНАЛИЗ ОБНАРУЖЕННЫХ УСТРОЙСТВ");
    Serial.println("==================================");

    // Общая статистика
    int oldCount = 0, transitionCount = 0, newCount = 0, unknownCount = 0;
    int support100A = 0, supportStreetLighting = 0;

    for (const auto &device: discoveredDevices) {
        switch (device.generation) {
            case MirlibClient::OLD_GENERATION: oldCount++;
                break;
            case MirlibClient::TRANSITION_GENERATION: transitionCount++;
                break;
            case MirlibClient::NEW_GENERATION: newCount++;
                break;
            default: unknownCount++;
                break;
        }

        if (device.supports100A) support100A++;
        if (device.hasStreetLighting) supportStreetLighting++;
    }

    Serial.println("📈 Статистика по поколениям:");
    Serial.println("   Старое поколение: " + String(oldCount));
    Serial.println("   Переходное поколение: " + String(transitionCount));
    Serial.println("   Новое поколение: " + String(newCount));
    Serial.println("   Неизвестное поколение: " + String(unknownCount));
    Serial.println("   Поддержка 100А: " + String(support100A) + " из " + String(discoveredDevices.size()));
    Serial.println(
        "   Управление освещением: " + String(supportStreetLighting) + " из " + String(discoveredDevices.size()));

    // Детальная информация по каждому устройству
    Serial.println("\n📋 Детальная информация:");
    Serial.println("Адрес  | Поколение   | ID   | Прошивка | 100А | Освещение | Особенности");
    Serial.println("-------|-------------|------|----------|------|-----------|------------");

    for (const auto &device: discoveredDevices) {
        String addr = "0x" + String(device.address, HEX);
        while (addr.length() < 6) addr += " ";

        String gen = device.generationName;
        while (gen.length() < 11) gen += " ";

        String boardId = "0x" + String(device.boardId, HEX);
        while (boardId.length() < 4) boardId += " ";

        String firmware = "0x" + String(device.firmwareVersion, HEX);
        while (firmware.length() < 8) firmware += " ";

        String support100A = device.supports100A ? "Да  " : "Нет ";
        String lighting = device.hasStreetLighting ? "Да       " : "Нет      ";

        String features = "";
        if (device.generation == MirlibClient::OLD_GENERATION) {
            features = "Базовая функциональность";
        } else if (device.generation == MirlibClient::TRANSITION_GENERATION) {
            features = "Мгновенные значения, трансформация";
        } else if (device.generation == MirlibClient::NEW_GENERATION) {
            features = "Полная функциональность, множественные интерфейсы";
        }

        Serial.println(addr + " | " + gen + " | " + boardId + " | " + firmware + " | " +
                       support100A + " | " + lighting + " | " + features);
    }

    // Анализ ID плат
    Serial.println("\n🔧 Анализ ID плат:");
    printBoardIdAnalysis();
}

void printBoardIdAnalysis() {
    // Группировка по ID плат
    std::map<uint8_t, int> boardCounts;
    for (const auto &device: discoveredDevices) {
        boardCounts[device.boardId]++;
    }

    Serial.println("ID платы | Количество | Поколение   | Описание");
    Serial.println("---------|------------|-------------|----------");

    for (const auto &pair: boardCounts) {
        uint8_t boardId = pair.first;
        int count = pair.second;

        String id = "0x" + String(boardId, HEX);
        while (id.length() < 8) id += " ";

        String countStr = String(count);
        while (countStr.length() < 10) countStr += " ";

        String generation = "";
        String description = "";

        // Определение поколения и описания по ID платы
        switch (boardId) {
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x0C:
            case 0x0D:
            case 0x11:
            case 0x12:
                generation = "Старое     ";
                description = "Базовые измерения энергии";
                break;
            case 0x07:
            case 0x08:
            case 0x0A:
            case 0x0B:
                generation = "Переходное ";
                description = "Расширенные измерения, трансформация";
                break;
            case 0x09:
            case 0x0E:
            case 0x0F:
            case 0x10:
            case 0x20:
            case 0x21:
            case 0x22:
                generation = "Новое      ";
                description = "Полная функциональность";
                break;
            default:
                generation = "Неизвестно ";
                description = "Нестандартная плата";
                break;
        }

        Serial.println(id + " | " + countStr + " | " + generation + " | " + description);
    }
}

void testCommandCompatibility() {
    if (discoveredDevices.empty()) {
        return;
    }

    Serial.println("\n🧪 ТЕСТИРОВАНИЕ СОВМЕСТИМОСТИ КОМАНД");
    Serial.println("=====================================");

    for (auto &device: discoveredDevices) {
        Serial.println("\n📱 Тестирование устройства 0x" + String(device.address, HEX) +
                       " (" + device.generationName + " поколение)");

        testDeviceCommands(device);
    }

    // Сводка по совместимости
    printCompatibilitySummary();
}

void testDeviceCommands(DeviceInfo &device) {
    Serial.println("   🔍 Тестирование команд:");

    // 1. Ping (поддерживается всеми)
    PingCommand pingCmd;
    device.supportsPing = protocol.sendCommand(&pingCmd, device.address);
    Serial.println("      Ping: " + String(device.supportsPing ? "✅ Поддерживается" : "❌ Не поддерживается"));

    // 2. GetInfo (поддерживается всеми)
    GetInfoCommand infoCmd;
    device.supportsGetInfo = protocol.sendCommand(&infoCmd, device.address);
    Serial.println("      GetInfo: " + String(device.supportsGetInfo ? "✅ Поддерживается" : "❌ Не поддерживается"));

    // 3. ReadDateTime (поддерживается всеми)
    ReadDateTimeCommand dateTimeCmd;
    device.supportsReadDateTime = protocol.sendCommand(&dateTimeCmd, device.address);
    Serial.println(
        "      ReadDateTime: " + String(device.supportsReadDateTime ? "✅ Поддерживается" : "❌ Не поддерживается"));

    // 4. ReadStatus (поддерживается всеми, но формат разный)
    ReadStatusCommand statusCmd;
    statusCmd.setGeneration(device.boardId, 0x32);
    if (device.generation != MirlibClient::OLD_GENERATION) {
        statusCmd.setRequest(ACTIVE_FORWARD);
    }
    device.supportsReadStatus = protocol.sendCommand(&statusCmd, device.address);
    Serial.println(
        "      ReadStatus: " + String(device.supportsReadStatus ? "✅ Поддерживается" : "❌ Не поддерживается"));

    // 5. ReadInstantValue (только переходное и новое поколения)
    if (device.generation != MirlibClient::OLD_GENERATION) {
        ReadInstantValueCommand instantCmd;
        instantCmd.setGeneration(device.boardId, 0x32);
        instantCmd.setRequest(GROUP_BASIC);
        device.supportsReadInstantValue = protocol.sendCommand(&instantCmd, device.address);
        Serial.println(
            "      ReadInstantValue: " + String(device.supportsReadInstantValue
                                                    ? "✅ Поддерживается"
                                                    : "❌ Не поддерживается"));
    } else {
        device.supportsReadInstantValue = false;
        Serial.println("      ReadInstantValue: ⚠️ Не поддерживается старым поколением");
    }

    delay(1000); // Пауза между устройствами
}

void printCompatibilitySummary() {
    Serial.println("\n📊 СВОДКА ПО СОВМЕСТИМОСТИ КОМАНД");
    Serial.println("==================================");

    Serial.println("Команда         | Старое | Переходное | Новое | Общая совместимость");
    Serial.println("----------------|--------|------------|-------|-------------------");

    // Подсчет поддержки команд по поколениям
    struct CommandSupport {
        int oldTotal = 0, oldSupported = 0;
        int transitionTotal = 0, transitionSupported = 0;
        int newTotal = 0, newSupported = 0;
    };

    CommandSupport ping, getInfo, dateTime, readStatus, instantValue;

    for (const auto &device: discoveredDevices) {
        switch (device.generation) {
            case MirlibClient::OLD_GENERATION:
                ping.oldTotal++;
                getInfo.oldTotal++;
                dateTime.oldTotal++;
                readStatus.oldTotal++;
                instantValue.oldTotal++;
                if (device.supportsPing) ping.oldSupported++;
                if (device.supportsGetInfo) getInfo.oldSupported++;
                if (device.supportsReadDateTime) dateTime.oldSupported++;
                if (device.supportsReadStatus) readStatus.oldSupported++;
                if (device.supportsReadInstantValue) instantValue.oldSupported++;
                break;
            case MirlibClient::TRANSITION_GENERATION:
                ping.transitionTotal++;
                getInfo.transitionTotal++;
                dateTime.transitionTotal++;
                readStatus.transitionTotal++;
                instantValue.transitionTotal++;
                if (device.supportsPing) ping.transitionSupported++;
                if (device.supportsGetInfo) getInfo.transitionSupported++;
                if (device.supportsReadDateTime) dateTime.transitionSupported++;
                if (device.supportsReadStatus) readStatus.transitionSupported++;
                if (device.supportsReadInstantValue) instantValue.transitionSupported++;
                break;
            case MirlibClient::NEW_GENERATION:
                ping.newTotal++;
                getInfo.newTotal++;
                dateTime.newTotal++;
                readStatus.newTotal++;
                instantValue.newTotal++;
                if (device.supportsPing) ping.newSupported++;
                if (device.supportsGetInfo) getInfo.newSupported++;
                if (device.supportsReadDateTime) dateTime.newSupported++;
                if (device.supportsReadStatus) readStatus.newSupported++;
                if (device.supportsReadInstantValue) instantValue.newSupported++;
                break;
        }
    }

    // Вывод статистики
    auto printCommandLine = [](const String &name, const CommandSupport &support) {
        String nameStr = name;
        while (nameStr.length() < 15) nameStr += " ";

        String oldStr = (support.oldTotal > 0) ? String(support.oldSupported) + "/" + String(support.oldTotal) : "N/A";
        while (oldStr.length() < 6) oldStr += " ";

        String transStr = (support.transitionTotal > 0)
                              ? String(support.transitionSupported) + "/" + String(support.transitionTotal)
                              : "N/A";
        while (transStr.length() < 10) transStr += " ";

        String newStr = (support.newTotal > 0) ? String(support.newSupported) + "/" + String(support.newTotal) : "N/A";
        while (newStr.length() < 5) newStr += " ";

        int totalDevices = support.oldTotal + support.transitionTotal + support.newTotal;
        int totalSupported = support.oldSupported + support.transitionSupported + support.newSupported;
        float percentage = (totalDevices > 0) ? (float) totalSupported / totalDevices * 100 : 0;

        String compatStr = String(totalSupported) + "/" + String(totalDevices) + " (" + String(percentage, 1) + "%)";

        Serial.println(nameStr + " | " + oldStr + " | " + transStr + " | " + newStr + " | " + compatStr);
    };

    printCommandLine("Ping", ping);
    printCommandLine("GetInfo", getInfo);
    printCommandLine("ReadDateTime", dateTime);
    printCommandLine("ReadStatus", readStatus);
    printCommandLine("ReadInstantValue", instantValue);

    Serial.println("\n💡 Рекомендации:");
    Serial.println("   • Ping, GetInfo, ReadDateTime - универсальные команды");
    Serial.println("   • ReadStatus - поддерживается всеми, но формат ответа отличается");
    Serial.println("   • ReadInstantValue - только для переходного и нового поколений");
    Serial.println("   • Всегда проверяйте поколение перед использованием команд");
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "scan" || command == "s") {
            Serial.println("🔍 Повторное сканирование...");
            discoveredDevices.clear();
            scanDevices();
            analyzeDevices();
        } else if (command == "analyze" || command == "a") {
            analyzeDevices();
        } else if (command == "test" || command == "t") {
            testCommandCompatibility();
        } else if (command == "list" || command == "l") {
            if (discoveredDevices.empty()) {
                Serial.println("📭 Устройства не найдены");
            } else {
                Serial.println("📋 Обнаруженные устройства:");
                for (size_t i = 0; i < discoveredDevices.size(); i++) {
                    const auto &device = discoveredDevices[i];
                    Serial.println("   " + String(i + 1) + ". 0x" + String(device.address, HEX) +
                                   " - " + device.generationName + " поколение");
                }
            }
        } else if (command.startsWith("detail ")) {
            int deviceNum = command.substring(7).toInt();
            if (deviceNum >= 1 && deviceNum <= (int) discoveredDevices.size()) {
                printDeviceDetails(discoveredDevices[deviceNum - 1]);
            } else {
                Serial.println("❌ Неверный номер устройства");
            }
        } else if (command == "help" || command == "h") {
            Serial.println("📖 Доступные команды:");
            Serial.println("   scan (s)      - повторное сканирование устройств");
            Serial.println("   analyze (a)   - анализ обнаруженных устройств");
            Serial.println("   test (t)      - тестирование совместимости команд");
            Serial.println("   list (l)      - список обнаруженных устройств");
            Serial.println("   detail N      - детальная информация об устройстве N");
            Serial.println("   help (h)      - показать справку");
        } else if (command.length() > 0) {
            Serial.println("❓ Неизвестная команда. Введите 'help' для справки");
        }
    }
}

void printDeviceDetails(const DeviceInfo &device) {
    Serial.println("\n📱 ДЕТАЛЬНАЯ ИНФОРМАЦИЯ ОБ УСТРОЙСТВЕ");
    Serial.println("======================================");
    Serial.println("Адрес: 0x" + String(device.address, HEX));
    Serial.println("Статус: " + String(device.isOnline ? "🟢 Онлайн" : "🔴 Оффлайн"));
    Serial.println("ID платы: 0x" + String(device.boardId, HEX));
    Serial.println("Версия прошивки: 0x" + String(device.firmwareVersion, HEX));
    Serial.println("Поколение: " + device.generationName);
    Serial.println("Поддержка 100А: " + String(device.supports100A ? "Да" : "Нет"));
    Serial.println("Управление освещением: " + String(device.hasStreetLighting ? "Да" : "Нет"));

    Serial.println("\nПоддерживаемые команды:");
    Serial.println("   Ping: " + String(device.supportsPing ? "✅" : "❌"));
    Serial.println("   GetInfo: " + String(device.supportsGetInfo ? "✅" : "❌"));
    Serial.println("   ReadDateTime: " + String(device.supportsReadDateTime ? "✅" : "❌"));
    Serial.println("   ReadStatus: " + String(device.supportsReadStatus ? "✅" : "❌"));
    Serial.println("   ReadInstantValue: " + String(device.supportsReadInstantValue ? "✅" : "❌"));
}
