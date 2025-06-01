/*
 * AdvancedClient.ino
 * 
 * Продвинутый пример клиента для опроса нескольких счетчиков с автоматическим
 * определением поколения и адаптивным выбором команд
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

// Список счетчиков для опроса
struct MeterInfo {
    uint16_t address;
    String name;
    uint32_t password;
    MirlibClient::Generation generation;
    bool isOnline;
    unsigned long lastSuccessfulPoll;
    uint32_t pollCount;
    uint32_t errorCount;
};

MeterInfo meters[] = {
    {0x1234, "Счетчик кухни", 0x12345678, Mirlib::UNKNOWN, false, 0, 0, 0},
    {0x5678, "Счетчик зала", 0x87654321, Mirlib::UNKNOWN, false, 0, 0, 0},
    {0x9ABC, "Счетчик спальни", 0x11111111, Mirlib::UNKNOWN, false, 0, 0, 0},
    {0xDEF0, "Счетчик гаража", 0x22222222, Mirlib::UNKNOWN, false, 0, 0, 0}
};

const int METER_COUNT = sizeof(meters) / sizeof(meters[0]);

// Инициализация протокола в клиентском режиме
MirlibClient protocol(0xFFFF);

// Настройки опроса
const unsigned long POLL_INTERVAL = 30000; // Интервал между циклами опроса (30 сек)
const unsigned long METER_TIMEOUT = 3000; // Таймаут для каждого счетчика (3 сек)
const unsigned long OFFLINE_THRESHOLD = 300000; // Счетчик считается offline после 5 мин

unsigned long lastPollCycle = 0;
int currentMeterIndex = 0;
bool discoveryMode = true;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Mirlib AdvancedClient Example ===");
    Serial.println("🔍 Продвинутый клиент с множественным опросом");

    // Инициализация CC1101
    if (!protocol.begin(CS_PIN, GDO0_PIN, GDO2_PIN)) {
        Serial.println("❌ Ошибка инициализации CC1101!");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("✅ CC1101 инициализирован");

    protocol.setTimeout(METER_TIMEOUT);

    Serial.println("📋 Список счетчиков для опроса:");
    for (int i = 0; i < METER_COUNT; i++) {
        Serial.println("   " + String(i + 1) + ". " + meters[i].name +
                       " (0x" + String(meters[i].address, HEX) + ")");
    }
    Serial.println();

    // Запуск режима обнаружения
    Serial.println("🔍 Запуск режима обнаружения счетчиков...");
    performDiscovery();

    Serial.println("🚀 Начинаем регулярный опрос счетчиков");
    Serial.println("======================================");
}

void loop() {
    unsigned long currentTime = millis();

    // Проверяем, пора ли начать новый цикл опроса
    if (currentTime - lastPollCycle >= POLL_INTERVAL) {
        startPollCycle();
        lastPollCycle = currentTime;
        currentMeterIndex = 0;
    }

    // Опрашиваем следующий счетчик в списке
    if (currentMeterIndex < METER_COUNT) {
        pollMeter(currentMeterIndex);
        currentMeterIndex++;
        delay(1000); // Пауза между счетчиками
    }

    // Обработка команд через Serial
    handleSerialCommands();

    delay(100);
}

void performDiscovery() {
    Serial.println("🔎 Обнаружение и определение поколений счетчиков...");

    for (int i = 0; i < METER_COUNT; i++) {
        Serial.print("   Проверка " + meters[i].name + " (0x" + String(meters[i].address, HEX) + ")... ");

        // Попытка Ping
        PingCommand pingCmd;
        if (protocol.sendCommand(&pingCmd, meters[i].address)) {
            Serial.print("✅ Онлайн ");
            meters[i].isOnline = true;
            meters[i].lastSuccessfulPoll = millis();

            // Определение поколения через GetInfo
            if (determineGeneration(i)) {
                Serial.println("(" + getGenerationName(meters[i].generation) + ")");
            } else {
                Serial.println("(поколение не определено)");
            }
        } else {
            Serial.println("❌ Недоступен");
            meters[i].isOnline = false;
        }

        delay(500); // Пауза между проверками
    }

    Serial.println("🏁 Обнаружение завершено\n");
}

bool determineGeneration(int meterIndex) {
    GetInfoCommand infoCmd;

    if (protocol.sendCommand(&infoCmd, meters[meterIndex].address)) {
        GenerationInfo genInfo = infoCmd.getGenerationInfo();

        if (genInfo.isOldGeneration) {
            meters[meterIndex].generation = MirlibClient::OLD_GENERATION;
        } else if (genInfo.isTransitionGeneration) {
            meters[meterIndex].generation = MirlibClient::TRANSITION_GENERATION;
        } else if (genInfo.isNewGeneration) {
            meters[meterIndex].generation = MirlibClient::NEW_GENERATION;
        } else {
            meters[meterIndex].generation = MirlibClient::UNKNOWN;
            return false;
        }

        return true;
    }

    return false;
}

void startPollCycle() {
    Serial.println("\n🔄 Начало нового цикла опроса");
    Serial.println("Время: " + String(millis() / 1000) + " сек");
    printMeterStatistics();
}

void pollMeter(int index) {
    MeterInfo &meter = meters[index];

    Serial.println("\n📊 Опрос: " + meter.name + " (0x" + String(meter.address, HEX) + ")");

    meter.pollCount++;
    bool success = false;

    // 1. Ping для проверки связи
    PingCommand pingCmd;
    if (protocol.sendCommand(&pingCmd, meter.address)) {
        Serial.println("   ✅ Ping OK");
        success = true;

        // 2. Чтение даты/времени (поддерживается всеми)
        readDateTime(index);

        // 3. Чтение статуса с учетом поколения
        readStatus(index);

        // 4. Чтение мгновенных значений (если поддерживается)
        if (meter.generation != MirlibClient::OLD_GENERATION) {
            readInstantValues(index);
        }

        meter.isOnline = true;
        meter.lastSuccessfulPoll = millis();
    } else {
        Serial.println("   ❌ Нет связи: " + String(protocol.getLastError()));
        meter.errorCount++;

        // Проверяем, не ушел ли счетчик в offline
        if (millis() - meter.lastSuccessfulPoll > OFFLINE_THRESHOLD) {
            if (meter.isOnline) {
                Serial.println("   ⚠️ Счетчик перешел в offline");
                meter.isOnline = false;
            }
        }
    }

    Serial.println("   📈 Статистика: " + String(meter.pollCount) + " опросов, " +
                   String(meter.errorCount) + " ошибок, последний успех: " +
                   String((millis() - meter.lastSuccessfulPoll) / 1000) + " сек назад");
}

void readDateTime(int index) {
    ReadDateTimeCommand cmd;

    if (protocol.sendCommand(&cmd, meters[index].address)) {
        char timeStr[20];
        cmd.formatDateTime(timeStr);
        Serial.println("   🕐 Время: " + String(timeStr));
    } else {
        Serial.println("   ❌ Ошибка чтения времени");
    }
}

void readStatus(int index) {
    MeterInfo &meter = meters[index];

    ReadStatusCommand cmd;

    // Настройка команды в зависимости от поколения
    if (meter.generation == MirlibClient::OLD_GENERATION) {
        cmd.setGeneration(0x01, 0x30); // Пример ID и роли для старого поколения
    } else {
        cmd.setGeneration(0x09, 0x32); // Пример ID и роли для нового поколения
        cmd.setRequest(ACTIVE_FORWARD);
    }

    if (protocol.sendCommand(&cmd, meter.address)) {
        if (cmd.isOldGeneration()) {
            auto response = cmd.getOldResponse();
            Serial.println("   ⚡ Энергия: " + String(response.totalEnergy) +
                           " (тариф " + String(response.configByte.activeTariff + 1) + ")");
        } else {
            auto response = cmd.getNewResponse();
            Serial.println("   ⚡ Энергия активная: " + String(response.totalActive));
            Serial.println("   ⚡ Энергия полная: " + String(response.totalFull));
        }
    } else {
        Serial.println("   ❌ Ошибка чтения статуса");
    }
}

void readInstantValues(int index) {
    MeterInfo &meter = meters[index];

    ReadInstantValueCommand cmd;
    cmd.setGeneration(0x09, 0x32); // Предполагаем новое поколение
    cmd.setRequest(GROUP_BASIC);

    if (protocol.sendCommand(&cmd, meter.address)) {
        if (cmd.isTransitionGeneration()) {
            auto response = cmd.getTransitionResponse();
            Serial.println("   🔌 U: A=" + String(response.getVoltageA(), 1) + "В, B=" +
                           String(response.getVoltageB(), 1) + "В, C=" + String(response.getVoltageC(), 1) + "В");
            Serial.println("   ⚡ I: A=" + String(response.getCurrentA(), 2) + "А, B=" +
                           String(response.getCurrentB(), 2) + "А, C=" + String(response.getCurrentC(), 2) + "А");
            Serial.println(
                "   📈 f=" + String(response.getFrequencyHz(), 2) + "Гц, cos φ=" + String(
                    response.getCosPhiValue(), 3));
        } else if (cmd.isNewGeneration()) {
            auto response = cmd.getNewBasicResponse();
            Serial.println("   🔌 U: A=" + String(response.getVoltageA(), 1) + "В, B=" +
                           String(response.getVoltageB(), 1) + "В, C=" + String(response.getVoltageC(), 1) + "В");
            Serial.println("   ⚡ I: A=" + String(response.getCurrentA(), 2) + "А, B=" +
                           String(response.getCurrentB(), 2) + "А, C=" + String(response.getCurrentC(), 2) + "А");
            Serial.println("   🔋 P=" + String(response.getActivePowerKW(), 3) + "кВт, Q=" +
                           String(response.getReactivePowerKvar(), 3) + "квар");
        }
    } else {
        Serial.println("   ❌ Ошибка чтения мгновенных значений");
    }
}

void printMeterStatistics() {
    Serial.println("📊 Статистика счетчиков:");

    int onlineCount = 0;
    for (int i = 0; i < METER_COUNT; i++) {
        if (meters[i].isOnline) onlineCount++;

        String status = meters[i].isOnline ? "🟢 Online" : "🔴 Offline";
        String generation = getGenerationName(meters[i].generation);
        float successRate = (meters[i].pollCount > 0)
                                ? (float) (meters[i].pollCount - meters[i].errorCount) / meters[i].pollCount * 100
                                : 0;

        Serial.println("   " + meters[i].name + ": " + status + " | " + generation +
                       " | Успешность: " + String(successRate, 1) + "%");
    }

    Serial.println("📈 Общая статистика: " + String(onlineCount) + "/" + String(METER_COUNT) + " онлайн");
}

String getGenerationName(Mirlib::Generation gen) {
    switch (gen) {
        case MirlibClient::OLD_GENERATION: return "Старое";
        case MirlibClient::TRANSITION_GENERATION: return "Переходное";
        case MirlibClient::NEW_GENERATION: return "Новое";
        default: return "Неизвестно";
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "status" || command == "s") {
            printMeterStatistics();
        } else if (command == "discovery" || command == "d") {
            Serial.println("🔍 Повторное обнаружение...");
            performDiscovery();
        } else if (command == "reset" || command == "r") {
            Serial.println("🔄 Сброс статистики...");
            for (int i = 0; i < METER_COUNT; i++) {
                meters[i].pollCount = 0;
                meters[i].errorCount = 0;
            }
        } else if (command == "poll" || command == "p") {
            Serial.println("🚀 Принудительный опрос всех счетчиков...");
            for (int i = 0; i < METER_COUNT; i++) {
                pollMeter(i);
                delay(1000);
            }
        } else if (command.startsWith("poll ")) {
            int meterNum = command.substring(5).toInt();
            if (meterNum >= 1 && meterNum <= METER_COUNT) {
                Serial.println("🎯 Опрос счетчика " + String(meterNum) + "...");
                pollMeter(meterNum - 1);
            } else {
                Serial.println("❌ Неверный номер счетчика (1-" + String(METER_COUNT) + ")");
            }
        } else if (command == "help" || command == "h") {
            Serial.println("📖 Доступные команды:");
            Serial.println("   status (s)    - показать статистику");
            Serial.println("   discovery (d) - повторное обнаружение");
            Serial.println("   reset (r)     - сбросить статистику");
            Serial.println("   poll (p)      - опросить все счетчики");
            Serial.println("   poll N        - опросить счетчик N");
            Serial.println("   help (h)      - показать справку");
        } else if (command.length() > 0) {
            Serial.println("❓ Неизвестная команда. Введите 'help' для справки");
        }
    }
}
