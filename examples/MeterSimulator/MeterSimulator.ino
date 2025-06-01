/*
 * MeterSimulator.ino
 * 
 * Полная реализация серверного режима - имитация электросчетчика
 * Демонстрирует возможности библиотеки Mirlib в качестве сервера для тестирования клиентов
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

#include <MirlibServer.h>

// Конфигурация
const int CS_PIN = 5; // Пин Chip Select для CC1101
const int GDO0_PIN = 2; // Пин GDO0 (опционально)
const int GDO2_PIN = 4; // Пин GDO2 (опционально)
const uint16_t DEVICE_ADDRESS = 0x1234; // Адрес имитируемого счетчика
const uint32_t DEVICE_PASSWORD = 0x12345678; // Пароль счетчика
const uint32_t DEVICE_STATUS = 0x00000000; // Статус устройства

// Инициализация протокола в серверном режиме
MirlibServer protocol(DEVICE_ADDRESS);

// Переменные для имитации счетчика
unsigned long lastEnergyUpdate = 0;
unsigned long lastInstantUpdate = 0;
uint32_t totalEnergy = 12345678; // Общая энергия (увеличивается со временем)
float currentPowerKW = 2.5; // Текущая мощность в кВт
float voltageA = 230.0, voltageB = 231.0, voltageC = 229.0; // Напряжения по фазам
float currentA = 5.35, currentB = 5.42, currentC = 5.28; // Токи по фазам
float frequency = 50.0; // Частота сети
float cosPhi = 0.85; // Коэффициент мощности

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Mirlib MeterSimulator Example ===");
    Serial.println("🔧 Инициализация симулятора счетчика...");

    // Инициализация CC1101
    if (!protocol.begin(CS_PIN, GDO0_PIN, GDO2_PIN)) {
        Serial.println("❌ Ошибка инициализации CC1101!");
        Serial.println("Проверьте подключение модуля:");
        Serial.println("- VCC = 3.3V (НЕ 5V!)");
        Serial.println("- Подключение проводов SPI");
        Serial.println("- Пин CS = " + String(CS_PIN));
        while (true) {
            delay(1000);
        }
    }

    Serial.println("✅ CC1101 инициализирован успешно");

    protocol.setPassword(DEVICE_PASSWORD);
    protocol.setStatus(DEVICE_STATUS);

    // Выбор поколения для имитации (можно изменить)
    // OLD_GENERATION, TRANSITION_GENERATION, NEW_GENERATION
    protocol.setServerGeneration(MirlibServer::NEW_GENERATION);

    // Регистрация дополнительных обработчиков команд
    setupCustomHandlers();

    Serial.println("🏭 Поколение: НОВОЕ");
    Serial.println("📍 Адрес устройства: 0x" + String(DEVICE_ADDRESS, HEX));
    Serial.println("🔑 Пароль: 0x" + String(DEVICE_PASSWORD, HEX));
    Serial.println("📡 Симулятор счетчика запущен и готов к приему команд");
    Serial.println("=====================================");

    // Инициализация времени для обновления данных
    lastEnergyUpdate = millis();
    lastInstantUpdate = millis();

    printSimulationStatus();
}

void loop() {
    // Обработка входящих пакетов
    if (protocol.processIncomingPackets()) {
        // Пакет был успешно обработан
        printSimulationStatus();
    }

    // Обновление имитируемых данных
    updateSimulatedData();

    // Небольшая задержка для стабильности
    delay(10);
}

void setupCustomHandlers() {
    Serial.println("🔧 Настройка пользовательских обработчиков команд...");

    // Пример дополнительного обработчика для команды Ping
    protocol.registerCommandHandler(CMD_PING,
    [](const PacketData &request, PacketData &response, void * /*context*/) -> bool {
        Serial.println("📡 Обработка команды Ping");

        PingCommand cmd;
        cmd.setServerResponse(0x0120, DEVICE_ADDRESS); // Версия прошивки 1.20

        uint8_t responseData[4];
        size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                                responseData, sizeof(responseData));
        if (responseSize == 0) {
            return false;
        }

        response.dataSize = responseSize;
        memcpy(response.data, responseData, responseSize);
        return true;
    });
}

void updateSimulatedData() {
    unsigned long currentTime = millis();

    // Обновление энергии каждые 5 секунд
    if (currentTime - lastEnergyUpdate >= 5000) {
        // Имитируем потребление энергии
        // При мощности 2.5 кВт за 5 секунд = 2.5 * (5/3600) = 0.00347 кВт*ч
        totalEnergy += 35; // Примерное увеличение в условных единицах
        lastEnergyUpdate = currentTime;
    }

    // Обновление мгновенных значений каждые 2 секунды
    if (currentTime - lastInstantUpdate >= 2000) {
        // Имитируем небольшие колебания значений
        voltageA = 230.0 + random(-50, 50) / 10.0; // ±5В колебания
        voltageB = 231.0 + random(-50, 50) / 10.0;
        voltageC = 229.0 + random(-50, 50) / 10.0;

        currentA = 5.35 + random(-20, 20) / 100.0; // ±0.2А колебания
        currentB = 5.42 + random(-20, 20) / 100.0;
        currentC = 5.28 + random(-20, 20) / 100.0;

        frequency = 50.0 + random(-10, 10) / 100.0; // ±0.1Гц колебания
        cosPhi = 0.85 + random(-5, 5) / 100.0; // ±0.05 колебания

        // Обновляем мощность на основе токов и напряжений
        currentPowerKW = (voltageA * currentA + voltageB * currentB + voltageC * currentC) *
                         cosPhi * sqrt(3) / 1000.0;

        lastInstantUpdate = currentTime;
    }
}

void printSimulationStatus() {
    Serial.println("\n📊 Состояние симулятора:");
    Serial.println("   ⚡ Общая энергия: " + String(totalEnergy));
    Serial.println("   🔋 Текущая мощность: " + String(currentPowerKW, 3) + " кВт");
    Serial.println(
        "   🔌 Напряжения: A=" + String(voltageA, 1) + "В, B=" + String(voltageB, 1) + "В, C=" + String(voltageC, 1) +
        "В");
    Serial.println(
        "   ⚡ Токи: A=" + String(currentA, 3) + "А, B=" + String(currentB, 3) + "А, C=" + String(currentC, 3) + "А");
    Serial.println("   📈 Частота: " + String(frequency, 2) + " Гц");
    Serial.println("   📐 cos φ: " + String(cosPhi, 3));
    Serial.println("   🕐 Время работы: " + String(millis() / 1000) + " сек");
    Serial.println("---------------------------------------------");
}

// Дополнительные функции для тестирования

void printReceivedCommand(uint8_t command, uint16_t srcAddr) {
    Serial.println("\n📨 Получена команда: " + String(ProtocolUtils::getCommandName(command)) +
                   " (0x" + String(command, HEX) + ") от устройства 0x" + String(srcAddr, HEX));
}

// Функция для изменения поколения во время работы (для тестирования)
void changeGeneration() {
    static int currentGen = 2; // 0=OLD, 1=TRANSITION, 2=NEW
    currentGen = (currentGen + 1) % 3;

    switch (currentGen) {
        case 0:
            protocol.setServerGeneration(MirlibServer::OLD_GENERATION);
            Serial.println("🔄 Переключено на СТАРОЕ поколение");
            break;
        case 1:
            protocol.setServerGeneration(MirlibServer::TRANSITION_GENERATION);
            Serial.println("🔄 Переключено на ПЕРЕХОДНОЕ поколение");
            break;
        case 2:
            protocol.setServerGeneration(MirlibServer::NEW_GENERATION);
            Serial.println("🔄 Переключено на НОВОЕ поколение");
            break;
    }
}

// Обработка команд через серийный порт для интерактивного тестирования
void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "status") {
            printSimulationStatus();
        } else if (command == "generation") {
            changeGeneration();
        } else if (command == "reset") {
            totalEnergy = 12345678;
            Serial.println("🔄 Данные энергии сброшены");
        } else if (command == "help") {
            Serial.println("📖 Доступные команды:");
            Serial.println("   status     - показать состояние симулятора");
            Serial.println("   generation - переключить поколение");
            Serial.println("   reset      - сбросить данные энергии");
            Serial.println("   help       - показать эту справку");
        } else if (command.length() > 0) {
            Serial.println("❓ Неизвестная команда: " + command);
            Serial.println("Введите 'help' для списка команд");
        }
    }
}
