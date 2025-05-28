# Библиотека Mirlib

Arduino библиотека для связи с электросчетчиками через радиомодуль CC1101 с использованием протокола Mirtek.

## Возможности

- **Двухрежимная работа**: Клиентский режим для считывания счетчиков, серверный режим для имитации счетчиков
- **Поддержка нескольких поколений**: Автоматическое определение и поддержка старого, переходного и нового поколений счетчиков
- **Поддержка отладки**: Детальное логирование и анализ пакетов
- **Чистый API**: Фокус только на протоколе связи, без внешних интеграций

## Поддерживаемое оборудование

- **ESP32/ESP8266**: Основные целевые платформы
- **Arduino Uno/Nano**: Базовая функциональность (ограничения по памяти)
- **Радиомодуль CC1101**: Связь на частоте 868МГц в ISM диапазоне

## Установка

### Arduino IDE
1. Скачайте ZIP файл библиотеки
2. В Arduino IDE: Скетч → Подключить библиотеку → Добавить .ZIP библиотеку
3. Выберите скачанный ZIP файл

## Быстрый старт

### Клиентский режим (чтение счетчиков)

```cpp
#include <Mirlib.h>

// Инициализация протокола в клиентском режиме
Mirlib protocol(Mirlib::CLIENT, 0xFFFF);

void setup() {
    Serial.begin(115200);
    
    // Инициализация CC1101 (пин CS = 5)
    if (!protocol.begin(5)) {
        Serial.println("Ошибка инициализации CC1101");
        return;
    }
    
    protocol.setDebugMode(true);
    protocol.setTimeout(5000);
}

void loop() {
    uint16_t meterAddress = 0x1234;
    
    // Пинг счетчика
    PingCommand pingCmd;
    if (protocol.sendCommand(&pingCmd, meterAddress)) {
        Serial.print("Версия прошивки счетчика: 0x");
        Serial.println(pingCmd.getFirmwareVersion(), HEX);
        Serial.print("Адрес устройства: 0x");
        Serial.println(pingCmd.getDeviceAddress(), HEX);
    }
    
    // Получение информации о счетчике и автоопределение поколения
    GetInfoCommand infoCmd;
    if (protocol.sendCommand(&infoCmd, meterAddress)) {
        Serial.print("ID платы: 0x");
        Serial.println(infoCmd.getBoardId(), HEX);
        
        GenerationInfo genInfo = infoCmd.getGenerationInfo();
        Serial.print("Поколение: ");
        if (genInfo.isOldGeneration) Serial.println("Старое");
        else if (genInfo.isTransitionGeneration) Serial.println("Переходное");
        else if (genInfo.isNewGeneration) Serial.println("Новое");
        
        Serial.print("Поддержка 100А: ");
        Serial.println(infoCmd.supports100A() ? "Да" : "Нет");
    }
    
    // Чтение текущей даты и времени
    ReadDateTimeCommand dateTimeCmd;
    if (protocol.sendCommand(&dateTimeCmd, meterAddress)) {
        char timeStr[20];
        dateTimeCmd.formatDateTime(timeStr);
        Serial.print("Время счетчика: ");
        Serial.println(timeStr);
        Serial.print("День недели: ");
        Serial.println(dateTimeCmd.getDayOfWeekName());
    }
    
    // Чтение состояния счетчика
    ReadStatusCommand statusCmd;
    statusCmd.setGeneration(infoCmd.getBoardId(), 0x32);
    statusCmd.setRequest(ACTIVE_FORWARD);
    
    if (protocol.sendCommand(&statusCmd, meterAddress)) {
        if (statusCmd.isOldGeneration()) {
            auto response = statusCmd.getOldResponse();
            Serial.print("Общая энергия: ");
            Serial.println(response.totalEnergy);
            Serial.print("Активный тариф: ");
            Serial.println(response.configByte.activeTariff);
        } else {
            auto response = statusCmd.getNewResponse();
            Serial.print("Общая активная: ");
            Serial.println(response.totalActive);
            Serial.print("Общая полная: ");
            Serial.println(response.totalFull);
        }
    }
    
    // Чтение мгновенных значений (только новое/переходное поколение)
    if (!statusCmd.isOldGeneration()) {
        ReadInstantValueCommand instantCmd;
        instantCmd.setGeneration(infoCmd.getBoardId(), 0x32);
        instantCmd.setRequest(GROUP_BASIC);
        
        if (protocol.sendCommand(&instantCmd, meterAddress)) {
            if (instantCmd.isTransitionGeneration()) {
                auto response = instantCmd.getTransitionResponse();
                Serial.print("Напряжение A: ");
                Serial.print(response.getVoltageA());
                Serial.println(" В");
                Serial.print("Ток A: ");
                Serial.print(response.getCurrentA());
                Serial.println(" А");
                Serial.print("Частота: ");
                Serial.print(response.getFrequencyHz());
                Serial.println(" Гц");
            } else if (instantCmd.isNewGeneration()) {
                auto response = instantCmd.getNewBasicResponse();
                Serial.print("Напряжение A: ");
                Serial.print(response.getVoltageA());
                Serial.println(" В");
                Serial.print("Ток A: ");
                Serial.print(response.getCurrentA());
                Serial.println(" А");
                Serial.print("Активная мощность: ");
                Serial.print(response.getActivePowerKW());
                Serial.println(" кВт");
            }
        }
    }
    
    delay(10000);
}
```

### Серверный режим (имитация счетчика)

```cpp
#include <Mirlib.h>

// Инициализация протокола в серверном режиме (адрес счетчика 0x1234)
Mirlib protocol(Mirlib::SERVER, 0x1234);

void setup() {
    Serial.begin(115200);
    
    if (!protocol.begin(5)) {
        Serial.println("Ошибка инициализации CC1101");
        return;
    }
    
    protocol.setDebugMode(true);
    protocol.setPassword(0x12345678);
    protocol.setStatus(0x00000000);
    
    // Установка поколения сервера (OLD_GENERATION, TRANSITION_GENERATION, NEW_GENERATION)
    protocol.setServerGeneration(Mirlib::NEW_GENERATION);
    
    Serial.println("Имитация счетчика запущена");
}

void loop() {
    // Обработка входящих запросов
    protocol.processIncomingPackets();
    delay(10);
}
```

### Интеграция с внешними системами

Библиотека предоставляет чистый API без встроенных интеграций. Примеры интеграции с внешними системами:

#### MQTT

```cpp
#include <Mirlib.h>
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Mirlib protocol(Mirlib::CLIENT, 0xFFFF);

void publishMeterData(uint16_t meterAddress, const ReadStatusResponseNew& data) {
    String topic = "meters/" + String(meterAddress) + "/data";
    String payload = "{";
    payload += "\"totalActive\":" + String(data.totalActive) + ",";
    payload += "\"totalFull\":" + String(data.totalFull) + ",";
    payload += "\"energyType\":" + String(data.energyType);
    payload += "}";
    
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void loop() {
    uint16_t meterAddress = 0x1234;
    
    ReadStatusCommand cmd;
    cmd.setGeneration(0x09, 0x32); // Новое поколение
    cmd.setRequest(ACTIVE_FORWARD);
    
    if (protocol.sendCommand(&cmd, meterAddress)) {
        if (!cmd.isOldGeneration()) {
            publishMeterData(meterAddress, cmd.getNewResponse());
        }
    }
    
    delay(60000); // Раз в минуту
}
```

#### InfluxDB

```cpp
#include <Mirlib.h>
#include <WiFi.h>
#include <HTTPClient.h>

Mirlib protocol(Mirlib::CLIENT, 0xFFFF);

void sendToInfluxDB(uint16_t meterAddress, const ReadInstantValueResponseNewBasic& data) {
    HTTPClient http;
    http.begin("http://influxdb:8086/write?db=meters");
    
    String payload = "power,meter=" + String(meterAddress);
    payload += " voltage_a=" + String(data.getVoltageA());
    payload += ",current_a=" + String(data.getCurrentA());
    payload += ",active_power=" + String(data.getActivePowerKW());
    
    http.POST(payload);
    http.end();
}

void loop() {
    uint16_t meterAddress = 0x1234;
    
    ReadInstantValueCommand cmd;
    cmd.setGeneration(0x09, 0x32);
    cmd.setRequest(GROUP_BASIC);
    
    if (protocol.sendCommand(&cmd, meterAddress)) {
        if (cmd.isNewGeneration()) {
            sendToInfluxDB(meterAddress, cmd.getNewBasicResponse());
        }
    }
    
    delay(30000); // Раз в 30 секунд
}
```

## Поддерживаемые команды

### Команда Ping (0x01)
- **Назначение**: Проверка связи
- **Запрос**: Нет данных
- **Ответ**: Версия прошивки (2 байта) + Адрес устройства (2 байта)
- **Поддерживается**: Всеми поколениями

```cpp
PingCommand cmd;
if (protocol.sendCommand(&cmd, 0x1234)) {
    Serial.println("Прошивка: 0x" + String(cmd.getFirmwareVersion(), HEX));
    Serial.println("Адрес: 0x" + String(cmd.getDeviceAddress(), HEX));
}
```

### Команда ReadStatus (0x05)
- **Назначение**: Чтение счетчиков энергии и конфигурации
- **Запрос**: Тип энергии (1 байт, только для нового/переходного поколения)
- **Ответ**: Значения энергии и конфигурация (26-31 байт)
- **Поддерживается**: Всеми поколениями

```cpp
ReadStatusCommand cmd;
cmd.setGeneration(boardId, role);
cmd.setRequest(ACTIVE_FORWARD); // Только для нового/переходного поколения

if (protocol.sendCommand(&cmd, 0x1234)) {
    if (cmd.isOldGeneration()) {
        auto resp = cmd.getOldResponse();
        Serial.println("Общая энергия: " + String(resp.totalEnergy));
    } else {
        auto resp = cmd.getNewResponse();
        Serial.println("Общая активная: " + String(resp.totalActive));
    }
}
```

### Команда ReadDateTime (0x1C)
- **Назначение**: Чтение даты и времени устройства
- **Запрос**: Нет данных
- **Ответ**: Дата и время (7 байт): секунды, минуты, часы, день_недели, день, месяц, год
- **Поддерживается**: Всеми поколениями

```cpp
ReadDateTimeCommand cmd;
if (protocol.sendCommand(&cmd, 0x1234)) {
    char timeStr[20];
    cmd.formatDateTime(timeStr);
    Serial.println("Время: " + String(timeStr));
    Serial.println("День: " + String(cmd.getDayOfWeekName()));
}
```

### Команда ReadInstantValue (0x2B)
- **Назначение**: Чтение мгновенных электрических величин
- **Запрос**: Группа параметров (1 байт)
- **Ответ**: Мгновенные значения (25-30 байт в зависимости от поколения)
- **Поддерживается**: Только переходным и новым поколениями

```cpp
ReadInstantValueCommand cmd;
cmd.setGeneration(boardId, role);
cmd.setRequest(GROUP_BASIC);

if (protocol.sendCommand(&cmd, 0x1234)) {
    if (cmd.isTransitionGeneration()) {
        auto resp = cmd.getTransitionResponse();
        Serial.println("Напряжение A: " + String(resp.getVoltageA()) + " В");
        Serial.println("Ток A: " + String(resp.getCurrentA()) + " А");
    } else if (cmd.isNewGeneration()) {
        auto resp = cmd.getNewBasicResponse();
        Serial.println("Мощность: " + String(resp.getActivePowerKW()) + " кВт");
    }
}
```

### Команда GetInfo (0x30)
- **Назначение**: Получение расширенной информации об устройстве
- **Запрос**: Нет данных
- **Ответ**: Информация об устройстве включая ID платы, прошивку, интерфейсы (27-31 байт)
- **Поддерживается**: Всеми поколениями

```cpp
GetInfoCommand cmd;
if (protocol.sendCommand(&cmd, 0x1234)) {
    Serial.println("ID платы: 0x" + String(cmd.getBoardId(), HEX));
    Serial.println("Прошивка: 0x" + String(cmd.getFirmwareVersion(), HEX));
    Serial.println("Поддержка 100А: " + String(cmd.supports100A() ? "Да" : "Нет"));
    
    GenerationInfo info = cmd.getGenerationInfo();
    Serial.println("Поколение: " + String(info.isNewGeneration ? "Новое" : "Старое/Переходное"));
}
```

## Группы параметров для ReadInstantValue

```cpp
enum ParameterGroup : uint8_t {
    GROUP_BASIC = 0x00,        // Основные значения (напряжение, ток, мощность, частота, cos)
    GROUP_PHASE_ANGLES = 0x10, // Фазовые углы и мощность по фазам + температура
    GROUP_TIME_ANGLES = 0x11,  // Время + углы и мощность по фазам + частота
    GROUP_TOTAL_POWER = 0x12   // Общая мощность + основные значения + мощность по фазам
};
```

## Типы энергии для ReadStatus

```cpp
enum EnergyType : uint8_t {
    ACTIVE_FORWARD = 0x00,    // Активная энергия прямого направления
    ACTIVE_REVERSE = 0x01,    // Активная энергия обратного направления
    REACTIVE_FORWARD = 0x02,  // Реактивная энергия прямого направления
    REACTIVE_REVERSE = 0x03,  // Реактивная энергия обратного направления
    ACTIVE_ABSOLUTE = 0x04,   // Активная энергия абсолютная
    REACTIVE_ABSOLUTE = 0x05, // Реактивная энергия абсолютная
    REACTIVE_Q1 = 0x06,       // Реактивная Q1 (только новое поколение)
    REACTIVE_Q2 = 0x07,       // Реактивная Q2 (только новое поколение)
    REACTIVE_Q3 = 0x08,       // Реактивная Q3 (только новое поколение)
    REACTIVE_Q4 = 0x09        // Реактивная Q4 (только новое поколение)
};
```

## Поколения устройств

### Старое поколение
- **ID плат**: 0x01, 0x02, 0x03, 0x04, 0x0C, 0x0D, 0x11, 0x12
- **Особенности**: Базовые измерения энергии, 4 тарифа
- **ReadStatus**: 26-байтный ответ, без параметра типа энергии
- **ReadInstantValue**: Не поддерживается
- **Роль**: Любое значение

### Переходное поколение
- **ID плат**: 0x07, 0x08, 0x0A, 0x0B (при роли ≥ 0x32)
- **Особенности**: Улучшенные измерения, коэффициенты трансформации, мгновенные значения
- **ReadStatus**: 30-байтный ответ, параметр типа энергии
- **ReadInstantValue**: 25-28 байтный ответ (поддержка режима 100А)
- **Роль**: Должна быть ≥ 0x32

### Новое поколение
- **ID плат**: 0x09, 0x0E, 0x0F, 0x10, 0x20, 0x21, 0x22 (при роли ≥ 0x32)
- **Особенности**: Расширенные функции, множественные интерфейсы, мониторинг батареи, расширенные мгновенные значения
- **ReadStatus**: 30-31 байтный ответ, расширенные типы энергии
- **ReadInstantValue**: 30-байтный ответ с 3-байтными значениями мощности
- **Роль**: Должна быть ≥ 0x32

## Справочник API

### Класс Mirlib

#### Конструктор
```cpp
Mirlib(Mode mode = CLIENT, uint16_t deviceAddress = 0xFFFF)
```

#### Основные методы
```cpp
bool begin(int csPin, int gdo0Pin = -1, int gdo2Pin = -1);
bool sendCommand(BaseCommand* command, uint16_t targetAddress);
bool processIncomingPackets(); // Серверный режим
bool autoDetectGeneration(uint16_t targetAddress);
void setDebugMode(bool enable);
const char* getLastError();
```

#### Конфигурация
```cpp
void setPassword(uint32_t password);              // Серверный режим
void setStatus(uint32_t status);                  // Серверный режим  
void setTimeout(uint32_t timeout);                // Клиентский режим
void setServerGeneration(Generation generation);  // Серверный режим
```

#### Определение поколения
```cpp
Generation getGeneration() const;
uint16_t getDeviceAddress() const;
```

### Классы команд

Все команды наследуются от `BaseCommand` и предоставляют типобезопасные интерфейсы:

```cpp
class PingCommand : public TypedCommand<PingRequest, PingResponse>;
class GetInfoCommand : public TypedCommand<GetInfoRequest, GetInfoResponseBase>;
class ReadDateTimeCommand : public TypedCommand<ReadDateTimeRequest, ReadDateTimeResponse>;
class ReadStatusCommand : public BaseCommand; // Обрабатывает форматы старого и нового поколений
class ReadInstantValueCommand : public BaseCommand; // Обрабатывает форматы переходного и нового поколений
```

## Устранение неполадок

### Частые проблемы

1. **CC1101 не обнаружен**
   - Проверьте подключения проводов (VCC=3.3В, GND, MOSI, MISO, SCK, CS)
   - Проверьте конфигурацию пина CS
   - Убедитесь в питании 3.3В (не 5В!)

2. **Нет ответа от счетчика**
   - Проверьте настройки частоты (868.0 МГц)
   - Проверьте адрес счетчика и диапазон (0x0001-0xFDE8)
   - Увеличьте значение таймаута
   - Включите режим отладки для анализа пакетов
   - Проверьте подключение антенны

3. **Ошибки CRC**
   - Проверьте электромагнитные помехи
   - Проверьте целостность пакетов с помощью отладочного вывода
   - Убедитесь в правильном заземлении
   - Проверьте качество модуля CC1101

4. **ReadInstantValue не работает**
   - Проверьте определение поколения (не поддерживается старым поколением)
   - Проверьте значение группы параметров
   - Убедитесь в правильном значении роли (≥ 0x32 для переходного/нового)

### Отладочный вывод

Включите режим отладки для просмотра детальной информации о пакетах:

```cpp
protocol.setDebugMode(true);
```

Пример вывода:
```
[Mirlib] Отправка запроса: 73 55 20 00 34 12 FF FF 01 78 56 34 12 A5 55
[Mirlib] Получен ответ: 73 55 24 00 FF FF 34 12 01 00 00 00 00 00 01 00 34 12 B2 55
[Mirlib] Обнаружено поколение: Новое (ID платы: 0x09)
```

## Примеры

Библиотека включает полные примеры:

- **BasicClient.ino** - Простое чтение счетчика со всеми командами
- **MeterSimulator.ino** - Полная реализация серверного режима
- **AdvancedClient.ino** - Опрос нескольких счетчиков с определением поколения
- **GenerationDetection.ino** - Автоопределение и совместимость команд

## Заметки о производительности

### Использование памяти
- **ESP32**: ~12KB Flash, ~1.5KB RAM
- **ESP8266**: ~10KB Flash, ~1KB RAM
- **Arduino Uno**: ~7KB Flash, ~600B RAM (ограниченная функциональность)

## Лицензия

MIT License - см. файл LICENSE для деталей.

## Участие в разработке

1. Сделайте fork репозитория
2. Для linux - используйте команду ./find_arduino_paths.sh чтобы найти библиотеки зависимости
3. Создайте ветку для новой функции (`git checkout -b feature/new-command`)
4. Добавьте тесты для новой функциональности
5. Обновите документацию
6. Отправьте pull request

## Changelog

### v0.0.1
- Поддержка всех трех поколений счетчиков
- Реализация протокола и некоторые команды
- Клиентский и серверный режимы
- Автоматическое определение поколения
- Примеры и документация

### v0.0.2
- Убрал STL
- Исправил название библиотеки

### v0.0.3
- Поправил overflow по int'у у arduino

### v0.0.4
- Исправил версию в метафайлах

### v0.0.5
- Убрал правила сборки