#include "Mirlib.h"

Mirlib::Mirlib(Mode mode, uint16_t deviceAddress)
    : m_mode(mode)
      , m_deviceAddress(deviceAddress)
      , m_password(0)
      , m_status(0)
      , m_timeout(5000)
      , m_generation(UNKNOWN)
      , m_serverGeneration(NEW_GENERATION)
      , m_debugMode(false)
      , m_gdo0Pin(2) // По умолчанию пин 2 для GDO0
      , m_commandHandlers(nullptr)
{
    memset(m_lastError, 0, sizeof(m_lastError));

    // Регистрация обработчиков команд по умолчанию для режима сервера
    if (m_mode == SERVER) {
        registerDefaultHandlers();
    }
}

Mirlib::~Mirlib() {
    clearCommandHandlers();
}

bool Mirlib::begin(int csPin, int gdo0Pin, int gdo2Pin) {
    // Сохраняем пин GDO0 для использования в функциях
    if (gdo0Pin >= 0) {
        m_gdo0Pin = gdo0Pin;
    }

    // Устанавливаем пин GDO0 (как в оригинале)
    ELECHOUSE_cc1101.setGDO0(m_gdo0Pin);

    // Настройка SPI пинов (правильно - только 4 параметра: SCK, MISO, MOSI, SS)
    // В большинстве случаев используются стандартные пины SPI, поэтому передаем только CS
    ELECHOUSE_cc1101.setSpiPin(-1, -1, -1, csPin); // SCK, MISO, MOSI = -1 (стандартные), SS = csPin

    // Проверка подключения CC1101 (как в оригинале)
    if (!ELECHOUSE_cc1101.getCC1101()) {
        setError("SPI Connection CC1101 Error");
        return false;
    }

    if (m_debugMode) {
        debugPrint("SPI Connection CC1101 OK");
    }

    // Инициализация CC1101 с оригинальными настройками
    if (!initializeCC1101()) {
        setError("CC1101 initialization failed");
        return false;
    }

    if (m_debugMode) {
        debugPrint("CC1101 инициализирован с оригинальными настройками");
    }

    return true;
}

bool Mirlib::initializeCC1101() {
    // Оригинальные настройки rfSettings из старого проекта
    const byte rfSettings[] = {
        0x0D, // IOCFG2              GDO2 Output Pin Configuration
        0x2E, // IOCFG1              GDO1 Output Pin Configuration
        0x06, // IOCFG0              GDO0 Output Pin Configuration
        0x4F, // FIFOTHR             RX FIFO and TX FIFO Thresholds
        0xD3, // SYNC1               Sync Word, High Byte
        0x91, // SYNC0               Sync Word, Low Byte
        0x3C, // PKTLEN              Packet Length
        0x00, // PKTCTRL1            Packet Automation Control
        0x41, // PKTCTRL0            Packet Automation Control
        0x00, // ADDR                Device Address
        0x16, // CHANNR              Channel Number
        0x0F, // FSCTRL1             Frequency Synthesizer Control
        0x00, // FSCTRL0             Frequency Synthesizer Control
        0x10, // FREQ2               Frequency Control Word, High Byte
        0x8B, // FREQ1               Frequency Control Word, Middle Byte
        0x54, // FREQ0               Frequency Control Word, Low Byte
        0xD9, // MDMCFG4             Modem Configuration
        0x83, // MDMCFG3             Modem Configuration
        0x13, // MDMCFG2             Modem Configuration
        0xD2, // MDMCFG1             Modem Configuration
        0xAA, // MDMCFG0             Modem Configuration
        0x31, // DEVIATN             Modem Deviation Setting
        0x07, // MCSM2               Main Radio Control State Machine Configuration
        0x0C, // MCSM1               Main Radio Control State Machine Configuration
        0x08, // MCSM0               Main Radio Control State Machine Configuration
        0x16, // FOCCFG              Frequency Offset Compensation Configuration
        0x6C, // BSCFG               Bit Synchronization Configuration
        0x03, // AGCCTRL2            AGC Control
        0x40, // AGCCTRL1            AGC Control
        0x91, // AGCCTRL0            AGC Control
        0x87, // WOREVT1             High Byte Event0 Timeout
        0x6B, // WOREVT0             Low Byte Event0 Timeout
        0xF8, // WORCTRL             Wake On Radio Control
        0x56, // FREND1              Front End RX Configuration
        0x10, // FREND0              Front End TX Configuration
        0xE9, // FSCAL3              Frequency Synthesizer Calibration
        0x2A, // FSCAL2              Frequency Synthesizer Calibration
        0x00, // FSCAL1              Frequency Synthesizer Calibration
        0x1F, // FSCAL0              Frequency Synthesizer Calibration
        0x41, // RCCTRL1             RC Oscillator Configuration
        0x00, // RCCTRL0             RC Oscillator Configuration
        0x59, // FSTEST              Frequency Synthesizer Calibration Control
        0x59, // PTEST               Production Test
        0x3F, // AGCTEST             AGC Test
        0x81, // TEST2               Various Test Settings
        0x35, // TEST1               Various Test Settings
        0x09, // TEST0               Various Test Settings
    };

    // Сброс CC1101 (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x30); // SRES - Reset chip
    delay(1);

    // Запись настроек в регистры CC1101 (как в оригинале)
    ELECHOUSE_cc1101.SpiWriteBurstReg(0x00, const_cast<byte *>(rfSettings), 0x2F);

    // Калибровка частотного синтезатора (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x33); // SCAL - Calibrate frequency synthesizer and turn it off
    delay(1);

    // Очистка FIFO буферов (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer

    // Переход в режим приема (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

    if (m_debugMode) {
        debugPrint("CC1101 настроен с оригинальными параметрами");
        debugPrint("Режим: RX");
    }

    return true;
}

void Mirlib::setPassword(uint32_t password) {
    m_password = password;
}

void Mirlib::setStatus(uint32_t status) {
    m_status = status;
}

bool Mirlib::sendCommand(BaseCommand *command, uint16_t targetAddress,
                         uint8_t *responseData, size_t responseSize) {
    if (m_mode != CLIENT) {
        setError("Устройство не в режиме клиента");
        return false;
    }

    if (!command) {
        setError("Команда равна null");
        return false;
    }

    // Подготовка данных запроса
    uint8_t requestData[ProtocolConstants::MAX_DATA_SIZE];
    size_t requestDataSize = command->prepareRequest(requestData, sizeof(requestData));

    // Создание пакета запроса
    PacketData requestPacket;
    if (!ProtocolUtils::createRequestPacket(command->getCommandCode(), targetAddress,
                                            m_deviceAddress, m_password, requestData,
                                            requestDataSize, requestPacket)) {
        setError("Не удалось создать пакет запроса");
        return false;
    }

    if (m_debugMode) {
        debugPrintPacket(requestPacket, "Отправка запроса");
    }

    // Отправка пакета (улучшенная версия оригинальной функции)
    if (!sendPacketOriginalStyle(requestPacket)) {
        setError("Не удалось отправить пакет");
        return false;
    }

    // Ожидание ответа (улучшенная версия оригинальной функции)
    PacketData responsePacket;
    if (!receivePacketOriginalStyle(responsePacket, m_timeout)) {
        setError("Ответ не получен");
        return false;
    }

    if (m_debugMode) {
        debugPrintPacket(responsePacket, "Получен ответ");
    }

    // Проверка ответа
    if (responsePacket.command != command->getCommandCode()) {
        setError("Код команды ответа не совпадает");
        return false;
    }

    if (responsePacket.srcAddress != targetAddress) {
        setError("Адрес источника ответа не совпадает");
        return false;
    }

    if (responsePacket.destAddress != m_deviceAddress) {
        setError("Адрес назначения ответа не совпадает");
        return false;
    }

    if (!responsePacket.isResponse()) {
        setError("Полученный пакет не является ответом");
        return false;
    }

    // Разбор ответа
    if (!command->parseResponse(responsePacket.data, responsePacket.dataSize)) {
        setError("Не удалось разобрать данные ответа");
        return false;
    }

    // Копирование данных ответа при необходимости
    if (responseData && responseSize > 0) {
        size_t copySize = (responsePacket.dataSize < responseSize) ? responsePacket.dataSize : responseSize;
        memcpy(responseData, responsePacket.data, copySize);
    }

    return true;
}

bool Mirlib::processIncomingPackets() {
    if (m_mode != SERVER) {
        setError("Устройство не в режиме сервера");
        return false;
    }

    PacketData packet;
    if (!receivePacketOriginalStyle(packet, 100)) {
        // Короткий таймаут для неблокирующей работы
        return false; // Пакет не получен (не является ошибкой)
    }

    if (m_debugMode) {
        debugPrintPacket(packet, "Получен запрос");
    }

    return handleServerPacket(packet);
}

bool Mirlib::sendPacketOriginalStyle(const PacketData &packet) {
    if (!packet.isValid()) {
        if (m_debugMode) {
            debugPrint("Невалидный пакет для отправки");
        }
        return false;
    }

    // Калибровка частотного синтезатора (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x33); // SCAL - Calibrate frequency synthesizer and turn it off
    delay(1);

    // Очистка TX FIFO и выход из RX/TX режима (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer

    // Установка мощности передачи 10dB (как в оригинале)
    ELECHOUSE_cc1101.SpiWriteReg(0x3E, 0xC4); // PATABLE - выставляем мощность 10dB

    // Отправка пакета (как в оригинале)
    ELECHOUSE_cc1101.SendData(const_cast<uint8_t *>(packet.rawPacket), packet.rawSize);

    if (m_debugMode) {
        debugPrint("Пакет отправлен");
        char msg[50];
        snprintf(msg, sizeof(msg), "Размер пакета: %d байт", packet.rawSize);
        debugPrint(msg);
    }

    // Очистка RX FIFO и переход в режим приема (как в оригинале)
    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

    return true;
}

bool Mirlib::receivePacketOriginalStyle(PacketData &packet, uint32_t timeout) {
    packet.clear();

    uint32_t startTime = millis();
    timeout = (timeout == 0) ? m_timeout : timeout;

    if (m_debugMode) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Ожидание пакета (таймаут: %lu мс)", timeout);
        debugPrint(msg);
    }

    while (millis() - startTime < timeout) {
        // Проверка флага приема (как в оригинале)
        if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
            uint8_t buffer[ProtocolConstants::MAX_PACKET_SIZE];

            // Получение данных (как в оригинале)
            int len = ELECHOUSE_cc1101.ReceiveData(buffer);

            if (len > 0 && len <= ProtocolConstants::MAX_PACKET_SIZE) {
                if (m_debugMode) {
                    char msg[50];
                    snprintf(msg, sizeof(msg), "Получен пакет, размер: %d байт", len);
                    debugPrint(msg);
                    ProtocolUtils::printHex(buffer, len, "Сырые данные");
                }

                // Разбор пакета
                if (ProtocolUtils::unpackPacket(buffer, len, packet)) {
                    if (m_debugMode) {
                        debugPrint("Пакет успешно разобран");
                    }

                    // Очистка RX FIFO и перезапуск приема (как в оригинале)
                    ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer
                    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
                    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
                    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

                    return true;
                } else {
                    if (m_debugMode) {
                        debugPrint("Ошибка разбора пакета");
                    }
                }
            } else {
                if (m_debugMode) {
                    char msg[50];
                    snprintf(msg, sizeof(msg), "Неверный размер пакета: %d", len);
                    debugPrint(msg);
                }
            }

            // Очистка RX FIFO и перезапуск приема при ошибке
            ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer
            ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
            ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
            ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX
        }

        delay(1); // Небольшая задержка для стабильности
    }

    if (m_debugMode) {
        debugPrint("Таймаут приема пакета");
    }

    return false;
}

void Mirlib::registerCommandHandler(uint8_t commandCode,
                                    bool (*handlerFunc)(const PacketData &, PacketData &, void *),
                                    void *context) {
    // Создаем новый обработчик
    CommandHandler *newHandler = new CommandHandler();
    newHandler->commandCode = commandCode;
    newHandler->handlerFunc = handlerFunc;
    newHandler->context = context;
    newHandler->next = m_commandHandlers;

    // Добавляем в начало списка
    m_commandHandlers = newHandler;
}

CommandHandler *Mirlib::findCommandHandler(uint8_t commandCode) {
    CommandHandler *current = m_commandHandlers;
    while (current != nullptr) {
        if (current->commandCode == commandCode) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void Mirlib::clearCommandHandlers() {
    CommandHandler *current = m_commandHandlers;
    while (current != nullptr) {
        CommandHandler *next = current->next;
        delete current;
        current = next;
    }
    m_commandHandlers = nullptr;
}

bool Mirlib::autoDetectGeneration(uint16_t targetAddress) {
    GetInfoCommand getInfoCmd;

    if (!sendCommand(&getInfoCmd, targetAddress)) {
        return false;
    }

    uint8_t boardId = getInfoCmd.getBoardId();
    GenerationInfo info = ProtocolUtils::determineGeneration(boardId, 0x32);
    // Предполагаем role >= 0x32 для определения

    if (info.isOldGeneration) {
        m_generation = OLD_GENERATION;
    } else if (info.isTransitionGeneration) {
        m_generation = TRANSITION_GENERATION;
    } else if (info.isNewGeneration) {
        m_generation = NEW_GENERATION;
    } else {
        m_generation = UNKNOWN;
        return false;
    }

    if (m_debugMode) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Обнаружено поколение: %s (ID платы: 0x%02X)",
                 ProtocolUtils::getBoardGenerationName(boardId), boardId);
        debugPrint(msg);
    }

    return true;
}

bool Mirlib::handleServerPacket(const PacketData &packet) {
    // Проверка пакета
    if (!packet.isRequest()) {
        setError("Полученный пакет не является запросом");
        return false;
    }

    // Проверка адресации пакета (для нас или широковещательный)
    if (packet.destAddress != m_deviceAddress &&
        packet.destAddress != ProtocolConstants::ADDR_CLIENT) {
        // Пакет не для нас, игнорируем молча
        return false;
    }

    // Поиск обработчика команды
    CommandHandler *handler = findCommandHandler(packet.command);
    if (!handler || !handler->handlerFunc) {
        setError("Нет обработчика для команды");
        return false;
    }

    // Подготовка пакета ответа
    PacketData responsePacket;

    // Вызов обработчика команды
    if (!handler->handlerFunc(packet, responsePacket, handler->context)) {
        setError("Обработчик команды не сработал");
        return false;
    }

    // Отправка ответа (если это не широковещательная команда)
    if (packet.destAddress != ProtocolConstants::ADDR_CLIENT) {
        if (!sendResponse(packet, responsePacket)) {
            setError("Не удалось отправить ответ");
            return false;
        }

        if (m_debugMode) {
            debugPrintPacket(responsePacket, "Отправлен ответ");
        }
    }

    return true;
}

bool Mirlib::sendResponse(const PacketData &originalPacket, const PacketData &responseData) {
    PacketData responsePacket;

    if (!ProtocolUtils::createResponsePacket(originalPacket, m_status,
                                             responseData.data, responseData.dataSize,
                                             responsePacket)) {
        return false;
    }

    return sendPacketOriginalStyle(responsePacket);
}

void Mirlib::setError(const char *error) {
    strncpy(m_lastError, error, sizeof(m_lastError) - 1);
    m_lastError[sizeof(m_lastError) - 1] = '\0';

    if (m_debugMode) {
        Serial.print("Ошибка: ");
        Serial.println(m_lastError);
    }
}

void Mirlib::debugPrint(const char *message) {
    if (m_debugMode) {
        Serial.print("[Mirlib] ");
        Serial.println(message);
    }
}

void Mirlib::debugPrintPacket(const PacketData &packet, const char *title) {
    if (m_debugMode) {
        ProtocolUtils::printPacketHex(packet, title);
    }
}

// Статические обработчики команд
bool Mirlib::handlePingCommand(const PacketData &request, PacketData &response, void *context) {
    Mirlib *mirlib = (Mirlib *)context;

    PingCommand cmd;
    cmd.setServerResponse(0x0100, mirlib->m_deviceAddress); // Пример версии прошивки

    uint8_t responseData[4];
    size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                            responseData, sizeof(responseData));
    if (responseSize == 0) {
        return false;
    }

    response.dataSize = responseSize;
    memcpy(response.data, responseData, responseSize);
    return true;
}

bool Mirlib::handleGetInfoCommand(const PacketData &request, PacketData &response, void *context) {
    Mirlib *mirlib = (Mirlib *)context;

    GetInfoCommand cmd;

    // Создание ответа GetInfo по умолчанию на основе поколения сервера
    GetInfoResponseBase info;

    // Установка ID платы на основе поколения сервера
    switch (mirlib->m_serverGeneration) {
        case OLD_GENERATION:
            info.boardId = 0x01; // Плата старого поколения
            break;
        case TRANSITION_GENERATION:
            info.boardId = 0x07; // Плата переходного поколения
            break;
        case NEW_GENERATION:
        default:
            info.boardId = 0x09; // Плата нового поколения
            break;
    }

    info.firmwareVersion = 0x0100;
    info.firmwareCRC = 0x1234;
    info.workTime = millis() / 1000;
    info.sleepTime = 0;
    info.groupId = 0;
    info.flags = 0x80; // Поддержка 100A
    info.activeTariffCRC = 0x5678;
    info.plannedTariffCRC = 0x9ABC;
    info.timeSinceCorrection = millis() / 1000;
    info.reserve = 0;
    info.interface1Type = 1;
    info.interface2Type = 2;
    info.interface3Type = 3;
    info.interface4Type = 4;
    info.batteryVoltage = 3300; // 3.3В в мВ

    cmd.setServerResponse(info);

    uint8_t responseData[31];
    size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                            responseData, sizeof(responseData));
    if (responseSize == 0) {
        return false;
    }

    response.dataSize = responseSize;
    memcpy(response.data, responseData, responseSize);
    return true;
}

bool Mirlib::handleReadDateTimeCommand(const PacketData &request, PacketData &response, void *context) {
    ReadDateTimeCommand cmd;

    ReadDateTimeResponse dateTime;
    dateTime.seconds = (millis() / 1000) % 60;
    dateTime.minutes = (millis() / 60000) % 60;
    dateTime.hours = 14;
    dateTime.dayOfWeek = 2; // Вторник
    dateTime.day = 27;
    dateTime.month = 5;
    dateTime.year = 25; // 2025

    cmd.setServerResponse(dateTime);

    uint8_t responseData[7];
    size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                            responseData, sizeof(responseData));
    if (responseSize == 0) {
        return false;
    }

    response.dataSize = responseSize;
    memcpy(response.data, responseData, responseSize);
    return true;
}

bool Mirlib::handleReadStatusCommand(const PacketData &request, PacketData &response, void *context) {
    Mirlib *mirlib = (Mirlib *)context;

    ReadStatusCommand cmd;

    // Определение поколения на основе настроек сервера
    bool isOldGeneration = (mirlib->m_serverGeneration == OLD_GENERATION);

    // Установка поколения для команды на основе поколения сервера
    uint8_t boardId = 0x09; // По умолчанию новое поколение
    switch (mirlib->m_serverGeneration) {
        case OLD_GENERATION:
            boardId = 0x01;
            break;
        case TRANSITION_GENERATION:
            boardId = 0x07;
            break;
        case NEW_GENERATION:
        default:
            boardId = 0x09;
            break;
    }

    cmd.setGeneration(boardId, 0x32);

    if (cmd.isOldGeneration()) {
        // Создание ответа старого поколения
        ReadStatusResponseOld oldResponse;
        oldResponse.totalEnergy = 12345678;
        oldResponse.configByte.fromByte(0x03); // Пример конфигурации
        oldResponse.divisionCoeff = 1;
        oldResponse.roleCode = 0x32;
        oldResponse.multiplicationCoeff = 1;
        for (int i = 0; i < 4; i++) {
            oldResponse.tariffValues[i] = 1000000 * (i + 1);
        }
        cmd.setServerResponseOld(oldResponse);
    } else {
        // Создание ответа нового поколения
        ReadStatusResponseNew newResponse;
        newResponse.energyType = ACTIVE_FORWARD;
        if (request.dataSize > 0) {
            newResponse.energyType = static_cast<EnergyType>(request.data[0]);
        }
        newResponse.configByte.fromByte(0x03);
        newResponse.voltageTransformCoeff = 1;
        newResponse.currentTransformCoeff = 1;
        newResponse.totalFull = 87654321;
        newResponse.totalActive = 87654321;
        for (int i = 0; i < 4; i++) {
            newResponse.tariffValues[i] = 2000000 * (i + 1);
        }
        cmd.setServerResponseNew(newResponse);
    }

    uint8_t responseData[31];
    size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                            responseData, sizeof(responseData));
    if (responseSize == 0) {
        return false;
    }

    response.dataSize = responseSize;
    memcpy(response.data, responseData, responseSize);
    return true;
}

bool Mirlib::handleReadInstantValueCommand(const PacketData &request, PacketData &response, void *context) {
    Mirlib *mirlib = (Mirlib *)context;

    // Старое поколение не поддерживает эту команду
    if (mirlib->m_serverGeneration == OLD_GENERATION) {
        return false;
    }

    ReadInstantValueCommand cmd;

    // Определение поколения и ID платы на основе настроек сервера
    uint8_t boardId;
    switch (mirlib->m_serverGeneration) {
        case TRANSITION_GENERATION:
            boardId = 0x07; // Плата переходного поколения
            break;
        case NEW_GENERATION:
        default:
            boardId = 0x09; // Плата нового поколения
            break;
    }

    cmd.setGeneration(boardId, 0x32);

    // Разбор запроса для получения группы параметров
    ParameterGroup group = GROUP_BASIC; // По умолчанию
    if (request.dataSize > 0) {
        group = static_cast<ParameterGroup>(request.data[0]);
    }
    cmd.setRequest(group);

    if (cmd.isTransitionGeneration()) {
        // Создание ответа переходного поколения
        ReadInstantValueResponseTransition transResponse;
        transResponse.group = group;
        transResponse.voltageTransformCoeff = 1;
        transResponse.currentTransformCoeff = 5; // Трансформация 5A
        transResponse.activePower = 1234; // Пример: 12.34 кВт
        transResponse.reactivePower = 567; // Пример: 5.67 квар
        transResponse.frequency = 5000; // Пример: 50.00 Гц
        transResponse.cosPhi = 850; // Пример: cos φ = 0.850
        transResponse.voltageA = 23000; // Пример: 230.00 В
        transResponse.voltageB = 23100; // Пример: 231.00 В
        transResponse.voltageC = 22900; // Пример: 229.00 В

        // Проверка поддержки 100A (из флагов или конфигурации)
        transResponse.is100ASupport = true; // Предполагаем поддержку 100A для примера

        if (transResponse.is100ASupport) {
            // 3-байтные токи (разделить на 1000 для А)
            transResponse.currentA = 5350; // Пример: 5.350 А
            transResponse.currentB = 5420; // Пример: 5.420 А
            transResponse.currentC = 5280; // Пример: 5.280 А
        } else {
            // 2-байтные токи (разделить на 1000 для А)
            transResponse.currentA = 5350; // Пример: 5.350 А
            transResponse.currentB = 5420; // Пример: 5.420 А
            transResponse.currentC = 5280; // Пример: 5.280 А
        }

        cmd.setServerResponseTransition(transResponse);
    } else if (cmd.isNewGeneration()) {
        // Создание ответа нового поколения
        ReadInstantValueResponseNewBasic newResponse;
        newResponse.group = group;
        newResponse.voltageTransformCoeff = 1;
        newResponse.currentTransformCoeff = 5; // Трансформация 5A
        newResponse.activePower = 12340; // Пример: 12.340 кВт (3-байтное значение)
        newResponse.reactivePower = 5670; // Пример: 5.670 квар (3-байтное значение)
        newResponse.frequency = 5000; // Пример: 50.00 Гц
        newResponse.cosPhi = 850; // Пример: cos φ = 0.850
        newResponse.voltageA = 23000; // Пример: 230.00 В
        newResponse.voltageB = 23100; // Пример: 231.00 В
        newResponse.voltageC = 22900; // Пример: 229.00 В

        // 3-байтные токи для нового поколения (разделить на 1000 для А)
        newResponse.currentA = 5350; // Пример: 5.350 А
        newResponse.currentB = 5420; // Пример: 5.420 А
        newResponse.currentC = 5280; // Пример: 5.280 А

        cmd.setServerResponseNewBasic(newResponse);
    }

    uint8_t responseData[32]; // Максимальный размер для любого поколения
    size_t responseSize = cmd.handleRequest(request.data, request.dataSize,
                                            responseData, sizeof(responseData));
    if (responseSize == 0) {
        return false;
    }

    response.dataSize = responseSize;
    memcpy(response.data, responseData, responseSize);
    return true;
}

void Mirlib::registerDefaultHandlers() {
    // Регистрация обработчиков команд по умолчанию
    registerCommandHandler(CMD_PING, handlePingCommand, this);
    registerCommandHandler(CMD_GET_INFO, handleGetInfoCommand, this);
    registerCommandHandler(CMD_READ_DATE_TIME, handleReadDateTimeCommand, nullptr);
    registerCommandHandler(CMD_READ_STATUS, handleReadStatusCommand, this);
    registerCommandHandler(CMD_READ_INSTANT_VALUE, handleReadInstantValueCommand, this);
}

// Дополнительные утилитарные методы для диагностики CC1101

void Mirlib::printCC1101Status() {
    if (!m_debugMode) return;

    debugPrint("=== Статус CC1101 ===");

    // Чтение регистра статуса
    uint8_t status = ELECHOUSE_cc1101.SpiReadStatus(0xF5); // MARCSTATE
    char msg[50];
    snprintf(msg, sizeof(msg), "MARCSTATE: 0x%02X", status);
    debugPrint(msg);

    // Проверка FIFO
    uint8_t rxBytes = ELECHOUSE_cc1101.SpiReadStatus(0xFB); // RXBYTES
    uint8_t txBytes = ELECHOUSE_cc1101.SpiReadStatus(0xFA); // TXBYTES
    snprintf(msg, sizeof(msg), "RX FIFO: %d байт, TX FIFO: %d байт", rxBytes & 0x7F, txBytes & 0x7F);
    debugPrint(msg);

    debugPrint("==================");
}

void Mirlib::resetCC1101() {
    if (m_debugMode) {
        debugPrint("Выполняется сброс CC1101...");
    }

    // Повторная инициализация
    initializeCC1101();

    if (m_debugMode) {
        debugPrint("Сброс CC1101 завершен");
        printCC1101Status();
    }
}
