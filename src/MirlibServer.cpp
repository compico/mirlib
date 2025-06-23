#include "MirlibServer.h"

MirlibServer::MirlibServer(uint16_t deviceAddress, Generation serverGeneration)
    : MirlibBase(deviceAddress), m_serverGeneration(serverGeneration), m_commandHandlers(nullptr) {
    registerDefaultHandlers();
}

MirlibServer::~MirlibServer() {
    clearCommandHandlers();
}

bool MirlibServer::processIncomingPackets() {
    PacketData packet;
    if (!receivePacketOriginalStyle(packet, 100)) {
        // Короткий таймаут для неблокирующей работы
        return false; // Пакет не получен (не является ошибкой)
    }

    #ifdef MIRLIB_DEBUG
        debugPrintPacket(packet, "Получен запрос");
    #endif

    return handleServerPacket(packet);
}

void MirlibServer::registerCommandHandler(
    uint8_t commandCode,
    bool (*handlerFunc)(const PacketData &, PacketData &, void *),
    void *context
  ) {
    auto *newHandler = new CommandHandler();
    newHandler->commandCode = commandCode;
    newHandler->handlerFunc = handlerFunc;
    newHandler->context = context;
    newHandler->next = m_commandHandlers;

    // Добавляем в начало списка
    m_commandHandlers = newHandler;
}

CommandHandler *MirlibServer::findCommandHandler(uint8_t commandCode) {
    CommandHandler *current = m_commandHandlers;
    while (current != nullptr) {
        if (current->commandCode == commandCode) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void MirlibServer::clearCommandHandlers() {
    CommandHandler *current = m_commandHandlers;
    while (current != nullptr) {
        CommandHandler *next = current->next;
        delete current;
        current = next;
    }
    m_commandHandlers = nullptr;
}

bool MirlibServer::handleServerPacket(const PacketData &packet) {
    // Проверка пакета
    if (!packet.isRequest()) {
        setError(ERR_PACKAGE_IS_NO_REQUEST);
        return false;
    }

    // Проверка адресации пакета (для нас или широковещательный)
    if (packet.destAddress != m_deviceAddress &&
        packet.destAddress != ProtocolConstants::ADDR_CLIENT) {
        // Пакет не для нас, игнорируем молча
        return false;
    }

    // Поиск обработчика команды
    const CommandHandler *handler = findCommandHandler(packet.command);
    if ((handler == nullptr) || (handler->handlerFunc == nullptr)) {
        setError(ERR_NO_HAVE_HANDLER_FOR_THIS_COMMAND);
        return false;
    }

    // Подготовка пакета ответа
    PacketData responsePacket;

    // Вызов обработчика команды
    if (!handler->handlerFunc(packet, responsePacket, handler->context)) {
        setError(ERR_COMMAND_HANDLER_FAILED);
        return false;
    }

    // Отправка ответа (если это не широковещательная команда)
    if (packet.destAddress != ProtocolConstants::ADDR_CLIENT) {
        if (!sendResponse(packet, responsePacket)) {
            setError(ERR_FAILED_TO_SEND_RESPONSE);
            return false;
        }

        #ifdef MIRLIB_DEBUG
            debugPrintPacket(responsePacket, "Отправлен ответ");
        #endif
    }

    return true;
}

bool MirlibServer::sendResponse(const PacketData &originalPacket, const PacketData &responseData) {
    PacketData responsePacket;

    if (!ProtocolUtils::createResponsePacket(originalPacket, m_status,
                                             responseData.data, responseData.dataSize,
                                             responsePacket)) {
        return false;
    }

    return sendPacketOriginalStyle(responsePacket);
}

void MirlibServer::registerDefaultHandlers() {
    // Регистрация обработчиков команд по умолчанию
    registerCommandHandler(CMD_PING, handlePingCommand, this);
    registerCommandHandler(CMD_GET_INFO, handleGetInfoCommand, this);
    registerCommandHandler(CMD_READ_DATE_TIME, handleReadDateTimeCommand, nullptr);
    registerCommandHandler(CMD_READ_STATUS, handleReadStatusCommand, this);
    registerCommandHandler(CMD_READ_INSTANT_VALUE, handleReadInstantValueCommand, this);
}

// Статические обработчики команд
bool MirlibServer::handlePingCommand(const PacketData &request, PacketData &response, void *context) {
    MirlibServer *mirlibServer = (MirlibServer *)context;

    PingCommand cmd;
    cmd.setServerResponse(0x0100, mirlibServer->m_deviceAddress); // Пример версии прошивки

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

bool MirlibServer::handleGetInfoCommand(const PacketData &request, PacketData &response, void *context) {
    MirlibServer *mirlibServer = (MirlibServer *)context;

    GetInfoCommand cmd;

    // Создание ответа GetInfo по умолчанию на основе поколения сервера
    GetInfoResponseBase info;

    // Установка ID платы на основе поколения сервера
    switch (mirlibServer->m_serverGeneration) {
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

bool MirlibServer::handleReadDateTimeCommand(const PacketData &request, PacketData &response, void *context) {
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

bool MirlibServer::handleReadStatusCommand(const PacketData &request, PacketData &response, void *context) {
    MirlibServer *mirlibServer = (MirlibServer *)context;

    ReadStatusCommand cmd;

    // Установка поколения для команды на основе поколения сервера
    uint8_t boardId = 0x09; // По умолчанию новое поколение
    switch (mirlibServer->m_serverGeneration) {
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

bool MirlibServer::handleReadInstantValueCommand(const PacketData &request, PacketData &response, void *context) {
    MirlibServer *mirlibServer = (MirlibServer *)context;

    // Старое поколение не поддерживает эту команду
    if (mirlibServer->m_serverGeneration == OLD_GENERATION) {
        return false;
    }

    ReadInstantValueCommand cmd;

    // Определение поколения и ID платы на основе настроек сервера
    uint8_t boardId;
    switch (mirlibServer->m_serverGeneration) {
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