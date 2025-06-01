#include "ProtocolUtils.h"

uint8_t ProtocolUtils::calculateCRC8(const uint8_t *data, size_t length)
{
    uint8_t crc = ProtocolConstants::CRC_INITIAL;

    for (size_t i = 0; i < length; i++) {
        uint8_t dataByte = data[i];

        for (int bit = 0; bit < 8; bit++) {
            if (((dataByte ^ crc) & 0x80) == 0) {
                crc = (crc << 1);
            } else {
                crc = ((crc << 1) ^ ProtocolConstants::CRC_POLYNOMIAL);
            }
            dataByte = (dataByte << 1);
        }
    }

    return crc;
}

size_t ProtocolUtils::byteStuffing(
    const uint8_t *input,
    const size_t inputSize,
    uint8_t *output,
    const size_t outputMaxSize
) {
    if ((input == nullptr) || (output == nullptr) || inputSize == 0) {
        return 0;
    }

    size_t outputIndex = 0;

    for (size_t i = 0; i < inputSize; i++) {
        if (outputIndex >= outputMaxSize - 1) {
            // Reserve space for potential stuffing
            return 0; // Not enough space
        }

        uint8_t const currentByte = input[i];

        if (currentByte == 0x55) {
            // Replace 0x55 with 0x73 0x11
            output[outputIndex++] = ProtocolConstants::STUFF_MARKER;
            if (outputIndex >= outputMaxSize) {
                return 0;
            }
            output[outputIndex++] = ProtocolConstants::STUFF_0x55;
        } else if (currentByte == 0x73) {
            // Replace 0x73 with 0x73 0x22
            output[outputIndex++] = ProtocolConstants::STUFF_MARKER;
            if (outputIndex >= outputMaxSize) {
                return 0;
            }
            output[outputIndex++] = ProtocolConstants::STUFF_0x73;
        } else {
            // Regular byte, copy as is
            output[outputIndex++] = currentByte;
        }
    }

    return outputIndex;
}

size_t ProtocolUtils::byteUnstuffing(
    const uint8_t *input,
    size_t inputSize,
    uint8_t *output,
    size_t outputMaxSize
) {
    if ((input == nullptr) || (output == nullptr) || inputSize == 0) {
        return 0;
    }

    size_t outputIndex = 0;
    size_t inputIndex = 0;

    while (inputIndex < inputSize && outputIndex < outputMaxSize) {
        uint8_t const currentByte = input[inputIndex++];

        if (currentByte == ProtocolConstants::STUFF_MARKER && inputIndex < inputSize) {
            uint8_t const nextByte = input[inputIndex++];

            if (nextByte == ProtocolConstants::STUFF_0x55) {
                // 0x73 0x11 -> 0x55
                output[outputIndex++] = 0x55;
            } else if (nextByte == ProtocolConstants::STUFF_0x73) {
                // 0x73 0x22 -> 0x73
                output[outputIndex++] = 0x73;
            } else {
                // Invalid stuffing, treat as error or copy both bytes
                if (outputIndex < outputMaxSize - 1) {
                    output[outputIndex++] = currentByte;
                    output[outputIndex++] = nextByte;
                } else {
                    return 0;
                }
            }
        } else {
            // Regular byte, copy as is
            output[outputIndex++] = currentByte;
        }
    }

    return outputIndex;
}

bool ProtocolUtils::packPacket(PacketData &packet) {
    // Create packet without start/stop bytes and without stuffing
    uint8_t tempPacket[ProtocolConstants::MAX_PACKET_SIZE];
    size_t tempIndex = 0;

    // Parameters + Length
    packet.params.dataLength = packet.dataSize;
    tempPacket[tempIndex++] = packet.params.toByte();

    // Reserve
    tempPacket[tempIndex++] = ProtocolConstants::RESERVE;

    // Destination address (little-endian)
    uint16ToBytes(packet.destAddress, &tempPacket[tempIndex]);
    tempIndex += 2;

    // Source address (little-endian)
    uint16ToBytes(packet.srcAddress, &tempPacket[tempIndex]);
    tempIndex += 2;

    // Command
    tempPacket[tempIndex++] = packet.command;

    // Password/Status (little-endian)
    uint32ToBytes(packet.passwordOrStatus, &tempPacket[tempIndex]);
    tempIndex += 4;

    // Data
    if (packet.dataSize > 0) {
        memcpy(&tempPacket[tempIndex], packet.data, packet.dataSize);
        tempIndex += packet.dataSize;
    }

    // Calculate CRC for all bytes from Parameters to Data
    packet.crc = calculateCRC8(tempPacket, tempIndex);
    tempPacket[tempIndex++] = packet.crc;

    // Now perform byte stuffing on the packet (excluding start/stop)
    uint8_t stuffedPacket[ProtocolConstants::MAX_PACKET_SIZE];
    size_t stuffedSize = byteStuffing(tempPacket, tempIndex, stuffedPacket, sizeof(stuffedPacket));

    if (stuffedSize == 0 || stuffedSize + 3 > ProtocolConstants::MAX_PACKET_SIZE) {
        return false;
    }

    // Build final packet with start/stop bytes
    packet.rawSize = 0;
    packet.rawPacket[packet.rawSize++] = ProtocolConstants::START1;
    packet.rawPacket[packet.rawSize++] = ProtocolConstants::START2;

    memcpy(&packet.rawPacket[packet.rawSize], stuffedPacket, stuffedSize);
    packet.rawSize += stuffedSize;

    packet.rawPacket[packet.rawSize++] = ProtocolConstants::STOP;

    return true;
}

bool ProtocolUtils::unpackPacket(const uint8_t *rawData, size_t rawSize, PacketData &packet) {
    packet.clear();

    if ((rawData == nullptr) || rawSize < ProtocolConstants::MIN_PACKET_SIZE) {
        return false;
    }

    // Check start and stop bytes
    if (rawData[0] != ProtocolConstants::START1 ||
        rawData[1] != ProtocolConstants::START2 ||
        rawData[rawSize - 1] != ProtocolConstants::STOP) {
        return false;
    }

    // Extract stuffed data (without start/stop bytes)
    const uint8_t *stuffedData = rawData + 2;
    size_t const stuffedSize = rawSize - 3;

    // Perform reverse byte stuffing
    uint8_t unstuffedData[ProtocolConstants::MAX_PACKET_SIZE];
    size_t const unstuffedSize = byteUnstuffing(stuffedData, stuffedSize, unstuffedData, sizeof(unstuffedData));

    if (unstuffedSize < 8) {
        // Minimum: params(1) + reserve(1) + dest(2) + src(2) + cmd(1) + pass/stat(4) + crc(1)
        return false;
    }

    // Parse unstuffed packet
    size_t index = 0;

    // Parameters
    packet.params.fromByte(unstuffedData[index++]);

    // Reserve (skip)
    index++;

    // Destination address
    packet.destAddress = bytesToUint16(&unstuffedData[index]);
    index += 2;

    // Source address
    packet.srcAddress = bytesToUint16(&unstuffedData[index]);
    index += 2;

    // Command
    packet.command = unstuffedData[index++];

    // Password/Status
    packet.passwordOrStatus = bytesToUint32(&unstuffedData[index]);
    index += 4;

    // Data
    packet.dataSize = packet.params.dataLength;
    if (packet.dataSize > 0) {
        if (index + packet.dataSize >= unstuffedSize) {
            return false; // Not enough data
        }
        memcpy(packet.data, &unstuffedData[index], packet.dataSize);
        index += packet.dataSize;
    }

    // CRC
    if (index >= unstuffedSize) {
        return false;
    }
    packet.crc = unstuffedData[index++];

    // Verify CRC
    uint8_t calculatedCRC = calculateCRC8(unstuffedData, index - 1);
    if (calculatedCRC != packet.crc) {
        return false;
    }

    // Store raw packet
    memcpy(packet.rawPacket, rawData, rawSize);
    packet.rawSize = rawSize;

    return true;
}

void ProtocolUtils::encodeData(uint8_t *data, size_t size, uint8_t key) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= key;
    }
}

void ProtocolUtils::decodeData(uint8_t *data, size_t size, uint8_t key) {
    // XOR encoding is symmetric
    encodeData(data, size, key);
}

void ProtocolUtils::uint16ToBytes(uint16_t value, uint8_t *bytes) {
    bytes[0] = value & 0xFF; // Low byte first (little-endian)
    bytes[1] = (value >> 8) & 0xFF; // High byte second
}

void ProtocolUtils::uint32ToBytes(uint32_t value, uint8_t *bytes) {
    bytes[0] = value & 0xFF; // Byte 0 (lowest)
    bytes[1] = (value >> 8) & 0xFF; // Byte 1
    bytes[2] = (value >> 16) & 0xFF; // Byte 2
    bytes[3] = (value >> 24) & 0xFF; // Byte 3 (highest)
}

uint16_t ProtocolUtils::bytesToUint16(const uint8_t *bytes) {
    return (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);
}

uint32_t ProtocolUtils::bytesToUint32(const uint8_t *bytes) {
    return (uint32_t) bytes[0] |
           ((uint32_t) bytes[1] << 8) |
           ((uint32_t) bytes[2] << 16) |
           ((uint32_t) bytes[3] << 24);
}

GenerationInfo ProtocolUtils::determineGeneration(uint8_t boardId, uint8_t role) {
    GenerationInfo info = {0};
    info.boardId = boardId;
    info.role = role;

    // Old generation boards
    if (boardId == BOARD_OLD_01 || boardId == BOARD_OLD_02 || boardId == BOARD_OLD_03 ||
        boardId == BOARD_OLD_04 || boardId == BOARD_OLD_0C || boardId == BOARD_OLD_0D ||
        boardId == BOARD_OLD_11 || boardId == BOARD_OLD_12) {
        info.isOldGeneration = true;
        return info;
    }

    // For transition and new generation, also check role
    if (role >= 0x32) {
        // Transition generation boards
        if (boardId == BOARD_TRANS_07 || boardId == BOARD_TRANS_08 ||
            boardId == BOARD_TRANS_0A || boardId == BOARD_TRANS_0B) {
            info.isTransitionGeneration = true;
            return info;
        }

        // New generation boards
        if (boardId == BOARD_NEW_09 || boardId == BOARD_NEW_0E || boardId == BOARD_NEW_0F ||
            boardId == BOARD_NEW_10 || boardId == BOARD_NEW_20 || boardId == BOARD_NEW_21 ||
            boardId == BOARD_NEW_22) {
            info.isNewGeneration = true;
            return info;
        }
    }

    return info; // Unknown generation
}

bool ProtocolUtils::validatePacket(const PacketData &packet) {
    // Check basic packet validity
    if (!packet.isValid()) {
        return false;
    }

    // Check data size matches parameters
    if (packet.dataSize != packet.params.dataLength) {
        return false;
    }

    // Check address ranges
    if (packet.destAddress == 0 && packet.destAddress != ProtocolConstants::ADDR_PRODUCTION) {
        return false;
    }

    return true;
}

void ProtocolUtils::printPacketHex(const PacketData &packet, const char *title) {
    Serial.print(title);
    Serial.print(F(": "));
    printHex(packet.rawPacket, packet.rawSize, "");
}

void ProtocolUtils::printHex(const uint8_t *data, size_t size, const char *title) {
    if (title && strlen(title) > 0) {
        Serial.print(title);
        Serial.print(F(": "));
    }

    for (size_t i = 0; i < size; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

bool ProtocolUtils::createRequestPacket(uint8_t command, uint16_t destAddr, uint16_t srcAddr,
                                        uint32_t password, const uint8_t *data, uint8_t dataSize,
                                        PacketData &packet) {
    packet.clear();

    // Set parameters
    packet.params.direction = 1; // Request
    packet.params.version = 0; // Simple devices
    packet.params.encoding = 0; // Not encoded
    packet.params.dataLength = dataSize;

    // Set addresses and command
    packet.destAddress = destAddr;
    packet.srcAddress = srcAddr;
    packet.command = command;
    packet.passwordOrStatus = password;

    // Set data
    packet.dataSize = dataSize;
    if (dataSize > 0 && data) {
        memcpy(packet.data, data, dataSize);
    }

    return packPacket(packet);
}

bool ProtocolUtils::createResponsePacket(const PacketData &originalRequest, uint32_t status,
                                         const uint8_t *data, uint8_t dataSize, PacketData &packet) {
    packet.clear();

    // Set parameters
    packet.params.direction = 0; // Response
    packet.params.version = originalRequest.params.version;
    packet.params.encoding = originalRequest.params.encoding;
    packet.params.dataLength = dataSize;

    // Swap addresses for response
    packet.destAddress = originalRequest.srcAddress;
    packet.srcAddress = originalRequest.destAddress;
    packet.command = originalRequest.command;
    packet.passwordOrStatus = status;

    // Set data
    packet.dataSize = dataSize;
    if (dataSize > 0 && data) {
        memcpy(packet.data, data, dataSize);
    }

    return packPacket(packet);
}

const char *ProtocolUtils::getCommandName(uint8_t commandCode) {
    switch (commandCode) {
        case CMD_PING: return "Ping";
        case CMD_READ_STATUS: return "ReadStatus";
        case CMD_READ_DATE_TIME: return "ReadDateTime";
        case CMD_READ_INSTANT_VALUE: return "ReadInstanceValue";
        case CMD_GET_INFO: return "GetInfo";
        default: return "Unknown";
    }
}

const char *ProtocolUtils::getEnergyTypeName(uint8_t energyType) {
    switch (energyType) {
        case ACTIVE_FORWARD: return "ActiveForward";
        case ACTIVE_REVERSE: return "ActiveReverse";
        case REACTIVE_FORWARD: return "ReactiveForward";
        case REACTIVE_REVERSE: return "ReactiveReverse";
        case ACTIVE_ABSOLUTE: return "ActiveAbsolute";
        case REACTIVE_ABSOLUTE: return "ReactiveAbsolute";
        case REACTIVE_Q1: return "ReactiveQ1";
        case REACTIVE_Q2: return "ReactiveQ2";
        case REACTIVE_Q3: return "ReactiveQ3";
        case REACTIVE_Q4: return "ReactiveQ4";
        default: return "Unknown";
    }
}

const char *ProtocolUtils::getBoardGenerationName(uint8_t boardId) {
    switch (boardId) {
        case BOARD_OLD_01:
        case BOARD_OLD_02:
        case BOARD_OLD_03:
        case BOARD_OLD_04:
        case BOARD_OLD_0C:
        case BOARD_OLD_0D:
        case BOARD_OLD_11:
        case BOARD_OLD_12:
            return "Old";
        case BOARD_TRANS_07:
        case BOARD_TRANS_08:
        case BOARD_TRANS_0A:
        case BOARD_TRANS_0B:
            return "Transition";
        case BOARD_NEW_09:
        case BOARD_NEW_0E:
        case BOARD_NEW_0F:
        case BOARD_NEW_10:
        case BOARD_NEW_20:
        case BOARD_NEW_21:
        case BOARD_NEW_22:
            return "New";
        default:
            return "Unknown";
    }
}
