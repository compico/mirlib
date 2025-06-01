#include "MirlibClient.h"

MirlibClient::MirlibClient(uint16_t deviceAddress) : MirlibBase(deviceAddress) {
}

bool MirlibClient::sendCommand(BaseCommand *command, uint16_t targetAddress,
                               uint8_t *responseData, size_t responseSize) {
    if (!command) {
        setError(ERR_COMMAND_IS_NULL);
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
        setError(ERR_FAIL_CREATE_PACKAGE);
        return false;
    }

#ifdef MIRLIB_DEBUG
        debugPrintPacket(requestPacket, "Отправка запроса");
#endif

    // Отправка пакета
    if (!sendPacketOriginalStyle(requestPacket)) {
        setError(ERR_FAIL_SEND_PACKAGE);
        return false;
    }

    // Ожидание ответа
    PacketData responsePacket;
    if (!receivePacketOriginalStyle(responsePacket, m_timeout)) {
        setError(ERR_FAIL_RECEIVE_PACKAGE);
        return false;
    }

#ifdef MIRLIB_DEBUG
        debugPrintPacket(responsePacket, "Получен ответ");
#endif

    // Проверка ответа
    if (responsePacket.command != command->getCommandCode()) {
        setError(ERR_RESPONSE_COMMANDS_DO_NOT_MATCH);
        return false;
    }

    if (responsePacket.srcAddress != targetAddress) {
        setError(ERR_RESPONSE_ADDRESS_DO_NOT_MATCH);
        return false;
    }

    if (responsePacket.destAddress != m_deviceAddress) {
        setError(ERR_RESPONSE_TARGET_DO_NOT_MATCH);
        return false;
    }

    if (!responsePacket.isResponse()) {
        setError(ERR_RESPONSE_IS_NOT_RESPONSE);
        return false;
    }

    // Разбор ответа
    if (!command->parseResponse(responsePacket.data, responsePacket.dataSize)) {
        setError(ERR_UNABLE_TO_PARSE_RESPONSE_DATA);
        return false;
    }

    // Копирование данных ответа при необходимости
    if (responseData && responseSize > 0) {
        size_t copySize = (responsePacket.dataSize < responseSize) ? responsePacket.dataSize : responseSize;
        memcpy(responseData, responsePacket.data, copySize);
    }

    return true;
}

bool MirlibClient::autoDetectGeneration(uint16_t targetAddress) {
    GetInfoCommand getInfoCmd;

    if (!sendCommand(&getInfoCmd, targetAddress)) {
        return false;bool MirlibClient::sendCommand(BaseCommand *command, uint16_t targetAddress,
                               uint8_t *responseData, size_t responseSize) {
    if (!command) {
        setError(ERR_COMMAND_IS_NULL);
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
        setError(ERR_FAIL_CREATE_PACKAGE);
        return false;
    }

#ifdef MIRLIB_DEBUG
        debugPrintPacket(requestPacket, "Отправка запроса");
#endif

    // Отправка пакета
    if (!sendPacketOriginalStyle(requestPacket)) {
        setError(ERR_FAIL_SEND_PACKAGE);
        return false;
    }

    // Ожидание ответа
    PacketData responsePacket;
    if (!receivePacketOriginalStyle(responsePacket, m_timeout)) {
        setError(ERR_FAIL_RECEIVE_PACKAGE);
        return false;
    }

#ifdef MIRLIB_DEBUG
        debugPrintPacket(responsePacket, "Получен ответ");
#endif

    // Проверка ответа
    if (responsePacket.command != command->getCommandCode()) {
        setError(ERR_RESPONSE_COMMANDS_DO_NOT_MATCH);
        return false;
    }

    if (responsePacket.srcAddress != targetAddress) {
        setError(ERR_RESPONSE_ADDRESS_DO_NOT_MATCH);
        return false;
    }

    if (responsePacket.destAddress != m_deviceAddress) {
        setError(ERR_RESPONSE_TARGET_DO_NOT_MATCH);
        return false;
    }

    if (!responsePacket.isResponse()) {
        setError(ERR_RESPONSE_IS_NOT_RESPONSE);
        return false;
    }

    // Разбор ответа
    if (!command->parseResponse(responsePacket.data, responsePacket.dataSize)) {
        setError(ERR_UNABLE_TO_PARSE_RESPONSE_DATA);
        return false;
    }

    // Копирование данных ответа при необходимости
    if (responseData && responseSize > 0) {
        size_t copySize = (responsePacket.dataSize < responseSize) ? responsePacket.dataSize : responseSize;
        memcpy(responseData, responsePacket.data, copySize);
    }

    return true;
}
    }

    GenerationInfo info = ProtocolUtils::determineGeneration(getInfoCmd.getBoardId(), 0x32);
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

#ifdef MIRLIB_DEBUG
        char msg[100];
        uint8_t boardId = getInfoCmd.getBoardId();

        snprintf(msg, sizeof(msg), "Обнаружено поколение: %s (ID платы: 0x%02X)",
                 ProtocolUtils::getBoardGenerationName(boardId), boardId);
        MIRLIB_DEBUG_PRINT(msg);
#endif

    return true;
}

bool MirlibClient::ping(uint16_t targetAddress, uint16_t *firmwareVersion, uint16_t *deviceAddress) {
    PingCommand cmd;

    if (!sendCommand(&cmd, targetAddress)) {
        return false;
    }

    if (firmwareVersion) {
        *firmwareVersion = cmd.getFirmwareVersion();
    }

    if (deviceAddress) {
        *deviceAddress = cmd.getDeviceAddress();
    }

    return true;
}

bool MirlibClient::getInfo(uint16_t targetAddress, GetInfoResponseBase *response) {
    GetInfoCommand cmd;

    if (!sendCommand(&cmd, targetAddress)) {
        return false;
    }

    if (response) {
        *response = cmd.getResponse();
    }

    return true;
}

bool MirlibClient::readDateTime(uint16_t targetAddress, ReadDateTimeResponse *response) {
    ReadDateTimeCommand cmd;

    if (!sendCommand(&cmd, targetAddress)) {
        return false;
    }

    if (response) {
        *response = cmd.getDateTime();
    }

    return true;
}

bool MirlibClient::readStatus(uint16_t targetAddress, EnergyType energyType,
                              ReadStatusResponseOld *oldResponse, ReadStatusResponseNew *newResponse) {
    ReadStatusCommand cmd;

    // Установить поколение для команды
    GenerationInfo info = getGenerationInfo();
    uint8_t boardId = (info.boardId != 0) ? info.boardId : 0x09; // По умолчанию новое поколение
    cmd.setGeneration(boardId, 0x32);

    cmd.setRequest(energyType);

    if (!sendCommand(&cmd, targetAddress)) {
        return false;
    }

    if (cmd.isOldGeneration() && oldResponse) {
        *oldResponse = cmd.getOldResponse();
    }

    if (!cmd.isOldGeneration() && newResponse) {
        *newResponse = cmd.getNewResponse();
    }

    return true;
}

bool MirlibClient::readInstantValue(uint16_t targetAddress, ParameterGroup group,
                                    ReadInstantValueResponseTransition *transResponse,
                                    ReadInstantValueResponseNewBasic *newResponse) {
    ReadInstantValueCommand cmd;

    // Установить поколение для команды
    GenerationInfo info = getGenerationInfo();
    uint8_t boardId = (info.boardId != 0) ? info.boardId : 0x09; // По умолчанию новое поколение
    cmd.setGeneration(boardId, 0x32);

    cmd.setRequest(group);

    if (!sendCommand(&cmd, targetAddress)) {
        return false;
    }

    if (cmd.isTransitionGeneration() && transResponse) {
        *transResponse = cmd.getTransitionResponse();
    }

    if (cmd.isNewGeneration() && newResponse) {
        *newResponse = cmd.getNewBasicResponse();
    }

    return true;
}

GenerationInfo MirlibClient::getGenerationInfo(uint8_t boardId, uint8_t role) {
    GenerationInfo info = {0};

    // Если поколение уже определено, используем его
    if (m_generation != UNKNOWN) {
        switch (m_generation) {
            case OLD_GENERATION:
                info.boardId = (boardId != 0) ? boardId : 0x01;
                info.role = role;
                info.isOldGeneration = true;
                break;
            case TRANSITION_GENERATION:
                info.boardId = (boardId != 0) ? boardId : 0x07;
                info.role = role;
                info.isTransitionGeneration = true;
                break;
            case NEW_GENERATION:
                info.boardId = (boardId != 0) ? boardId : 0x09;
                info.role = role;
                info.isNewGeneration = true;
                break;
            default:
                break;
        }
    }

    // Если boardId передан, используем его для определения
    if (boardId != 0) {
        info = ProtocolUtils::determineGeneration(boardId, role);
    }

    return info;
}
