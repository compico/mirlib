#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <Arduino.h>

/**
 * @brief Protocol constants
 */
namespace ProtocolConstants {
    const uint8_t START1 = 0x73; ///< First start byte
    const uint8_t START2 = 0x55; ///< Second start byte
    const uint8_t STOP = 0x55; ///< Stop byte
    const uint8_t RESERVE = 0x00; ///< Reserve byte (always 0)

    const uint8_t CRC_POLYNOMIAL = 0xA9; ///< CRC8 polynomial
    const uint8_t CRC_INITIAL = 0x00; ///< CRC8 initial value

    const uint8_t STUFF_MARKER = 0x73; ///< Byte stuffing marker
    const uint8_t STUFF_0x55 = 0x11; ///< Replacement for 0x55 in stuffing
    const uint8_t STUFF_0x73 = 0x22; ///< Replacement for 0x73 in stuffing

    const size_t MAX_DATA_SIZE = 31; ///< Maximum data field size
    const size_t MAX_PACKET_SIZE = 64; ///< Maximum packet size after stuffing
    const size_t MIN_PACKET_SIZE = 10; ///< Minimum packet size

    // Special addresses
    const uint16_t ADDR_PRODUCTION = 0x0000; ///< Production address
    const uint16_t ADDR_CLIENT = 0xFFFF; ///< Client/broadcast address
    const uint16_t ADDR_METER_MIN = 0x0001; ///< Minimum meter address
    const uint16_t ADDR_METER_MAX = 0xFDE8; ///< Maximum meter address
    const uint16_t ADDR_SPECIAL_MIN = 0xFFDB; ///< Special addresses min
    const uint16_t ADDR_SPECIAL_MAX = 0xFFFE; ///< Special addresses max
}

/**
 * @brief Parameters field structure
 */
struct Parameters {
    uint8_t dataLength: 5; ///< Data length (L4...L0)
    uint8_t direction: 1; ///< Direction (D): 1=request, 0=response
    uint8_t version: 1; ///< Version (V0): 0=simple devices, 1=complex devices
    uint8_t encoding: 1; ///< Encoding (C): 0=not encoded, 1=encoded

    /**
     * @brief Convert to byte value
     */
    uint8_t toByte() const {
        return (encoding << 7) | (version << 6) | (direction << 5) | dataLength;
    }

    /**
     * @brief Initialize from byte value
     */
    void fromByte(uint8_t value) {
        dataLength = value & 0x1F;
        direction = (value >> 5) & 0x01;
        version = (value >> 6) & 0x01;
        encoding = (value >> 7) & 0x01;
    }
};

/**
 * @brief Configuration byte structure for meters
 */
struct ConfigByte {
    uint8_t decimalPoint: 2; ///< Decimal point position (0-3)
    uint8_t activeTariff: 2; ///< Active tariff (0-3)
    uint8_t displayDigits: 2; ///< Display digits count (0=6, 1=7, 2=8, 3=8)
    uint8_t enabledTariffs: 2; ///< Enabled tariffs (0=1, 1=2, 2=3, 3=4)

    /**
     * @brief Convert to byte value
     */
    uint8_t toByte() const {
        return (enabledTariffs << 6) | (displayDigits << 4) | (activeTariff << 2) | decimalPoint;
    }

    /**
     * @brief Initialize from byte value
     */
    void fromByte(uint8_t value) {
        decimalPoint = value & 0x03;
        activeTariff = (value >> 2) & 0x03;
        displayDigits = (value >> 4) & 0x03;
        enabledTariffs = (value >> 6) & 0x03;
    }
};

/**
 * @brief Energy types for ReadStatus command
 */
enum EnergyType : uint8_t {
    ACTIVE_FORWARD = 0x00, ///< Active forward energy
    ACTIVE_REVERSE = 0x01, ///< Active reverse energy
    REACTIVE_FORWARD = 0x02, ///< Reactive forward energy
    REACTIVE_REVERSE = 0x03, ///< Reactive reverse energy
    ACTIVE_ABSOLUTE = 0x04, ///< Active absolute energy
    REACTIVE_ABSOLUTE = 0x05, ///< Reactive absolute energy
    REACTIVE_Q1 = 0x06, ///< Reactive quadrant 1 (new generation only)
    REACTIVE_Q2 = 0x07, ///< Reactive quadrant 2 (new generation only)
    REACTIVE_Q3 = 0x08, ///< Reactive quadrant 3 (new generation only)
    REACTIVE_Q4 = 0x09 ///< Reactive quadrant 4 (new generation only)
};

/**
 * @brief Command codes
 */
enum CommandCode : uint8_t {
    CMD_PING = 0x01, ///< Ping command
    CMD_READ_STATUS = 0x05, ///< Read status counter
    CMD_READ_INSTANT_VALUE = 0x2B, ///< Read instant values (transition/new generation)
    CMD_READ_DATE_TIME = 0x1C, ///< Read date and time
    CMD_GET_INFO = 0x30 ///< Get device info
};

/**
 * @brief Board IDs for different meter generations
 */
enum BoardID : uint8_t {
    // Old generation
    BOARD_OLD_01 = 0x01,
    BOARD_OLD_02 = 0x02,
    BOARD_OLD_03 = 0x03,
    BOARD_OLD_04 = 0x04,
    BOARD_OLD_0C = 0x0C,
    BOARD_OLD_0D = 0x0D,
    BOARD_OLD_11 = 0x11,
    BOARD_OLD_12 = 0x12,

    // Transition generation
    BOARD_TRANS_07 = 0x07,
    BOARD_TRANS_08 = 0x08,
    BOARD_TRANS_0A = 0x0A,
    BOARD_TRANS_0B = 0x0B,

    // New generation
    BOARD_NEW_09 = 0x09,
    BOARD_NEW_0E = 0x0E,
    BOARD_NEW_0F = 0x0F,
    BOARD_NEW_10 = 0x10,
    BOARD_NEW_20 = 0x20,
    BOARD_NEW_21 = 0x21,
    BOARD_NEW_22 = 0x22
};

/**
 * @brief Packet data structure
 */
struct PacketData {
    // Header
    Parameters params; ///< Parameters field
    uint16_t destAddress; ///< Destination address
    uint16_t srcAddress; ///< Source address
    uint8_t command; ///< Command code
    uint32_t passwordOrStatus; ///< Password (request) or Status (response)

    // Data
    uint8_t data[ProtocolConstants::MAX_DATA_SIZE]; ///< Data field
    uint8_t dataSize; ///< Actual data size

    // CRC
    uint8_t crc; ///< CRC8 checksum

    // Raw packet (with stuffing)
    uint8_t rawPacket[ProtocolConstants::MAX_PACKET_SIZE]; ///< Raw packet bytes
    size_t rawSize; ///< Raw packet size

    /**
     * @brief Constructor
     */
    PacketData() : dataSize(0), rawSize(0) {
        memset(data, 0, sizeof(data));
        memset(rawPacket, 0, sizeof(rawPacket));
        params.dataLength = 0;
        params.direction = 0;
        params.version = 0;
        params.encoding = 0;
        destAddress = 0;
        srcAddress = 0;
        command = 0;
        passwordOrStatus = 0;
        crc = 0;
    }

    /**
     * @brief Check if packet is valid
     */
    bool isValid() const {
        return rawSize >= ProtocolConstants::MIN_PACKET_SIZE &&
               rawSize <= ProtocolConstants::MAX_PACKET_SIZE &&
               dataSize <= ProtocolConstants::MAX_DATA_SIZE;
    }

    /**
     * @brief Check if this is a request packet
     */
    bool isRequest() const {
        return params.direction == 1;
    }

    /**
     * @brief Check if this is a response packet
     */
    bool isResponse() const {
        return params.direction == 0;
    }

    /**
     * @brief Clear packet data
     */
    void clear() {
        dataSize = 0;
        rawSize = 0;
        memset(data, 0, sizeof(data));
        memset(rawPacket, 0, sizeof(rawPacket));
        params.dataLength = 0;
        params.direction = 0;
        params.version = 0;
        params.encoding = 0;
        destAddress = 0;
        srcAddress = 0;
        command = 0;
        passwordOrStatus = 0;
        crc = 0;
    }
};

/**
 * @brief Device generation detection result
 */
struct GenerationInfo {
    uint8_t boardId; ///< Board ID from GetInfo
    uint8_t role; ///< Role value
    uint16_t firmwareVersion; ///< Firmware version
    bool isOldGeneration; ///< True if old generation
    bool isTransitionGeneration; ///< True if transition generation
    bool isNewGeneration; ///< True if new generation
};

#endif // PROTOCOL_TYPES_H
