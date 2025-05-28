#ifndef PROTOCOL_UTILS_H
#define PROTOCOL_UTILS_H

#include <Arduino.h>
#include "ProtocolTypes.h"

/**
 * @brief Utility functions for protocol operations
 */
class ProtocolUtils {
public:
    /**
     * @brief Calculate CRC8 checksum
     * @param data Pointer to data
     * @param length Data length
     * @return CRC8 value
     */
    static uint8_t calculateCRC8(const uint8_t *data, size_t length);

    /**
     * @brief Perform byte stuffing on data
     * @param input Input data
     * @param inputSize Input data size
     * @param output Output buffer
     * @param outputMaxSize Maximum output buffer size
     * @return Actual output size, 0 if error
     */
    static size_t byteStuffing(const uint8_t *input, size_t inputSize,
                               uint8_t *output, size_t outputMaxSize);

    /**
     * @brief Perform reverse byte stuffing (unstuffing)
     * @param input Input stuffed data
     * @param inputSize Input data size
     * @param output Output buffer
     * @param outputMaxSize Maximum output buffer size
     * @return Actual output size, 0 if error
     */
    static size_t byteUnstuffing(const uint8_t *input, size_t inputSize,
                                 uint8_t *output, size_t outputMaxSize);

    /**
     * @brief Pack packet into raw bytes
     * @param packet Packet structure
     * @return true if packing successful
     */
    static bool packPacket(PacketData &packet);

    /**
     * @brief Unpack raw bytes into packet structure
     * @param rawData Raw packet data
     * @param rawSize Raw data size
     * @param packet Output packet structure
     * @return true if unpacking successful
     */
    static bool unpackPacket(const uint8_t *rawData, size_t rawSize, PacketData &packet);

    /**
     * @brief Encode data (simple XOR encoding)
     * @param data Data to encode
     * @param size Data size
     * @param key Encoding key
     */
    static void encodeData(uint8_t *data, size_t size, uint8_t key);

    /**
     * @brief Decode data (simple XOR decoding)
     * @param data Data to decode
     * @param size Data size
     * @param key Decoding key
     */
    static void decodeData(uint8_t *data, size_t size, uint8_t key);

    /**
     * @brief Convert 16-bit value to little-endian bytes
     * @param value 16-bit value
     * @param bytes Output byte array (2 bytes)
     */
    static void uint16ToBytes(uint16_t value, uint8_t *bytes);

    /**
     * @brief Convert 32-bit value to little-endian bytes
     * @param value 32-bit value
     * @param bytes Output byte array (4 bytes)
     */
    static void uint32ToBytes(uint32_t value, uint8_t *bytes);

    /**
     * @brief Convert little-endian bytes to 16-bit value
     * @param bytes Input byte array (2 bytes)
     * @return 16-bit value
     */
    static uint16_t bytesToUint16(const uint8_t *bytes);

    /**
     * @brief Convert little-endian bytes to 32-bit value
     * @param bytes Input byte array (4 bytes)
     * @return 32-bit value
     */
    static uint32_t bytesToUint32(const uint8_t *bytes);

    /**
     * @brief Determine device generation from board ID
     * @param boardId Board ID
     * @param role Role value
     * @return Generation info structure
     */
    static GenerationInfo determineGeneration(uint8_t boardId, uint8_t role);

    /**
     * @brief Validate packet structure
     * @param packet Packet to validate
     * @return true if packet is valid
     */
    static bool validatePacket(const PacketData &packet);

    /**
     * @brief Print packet in hex format (for debugging)
     * @param packet Packet to print
     * @param title Title for output
     */
    static void printPacketHex(const PacketData &packet, const char *title = "Packet");

    /**
     * @brief Print raw data in hex format (for debugging)
     * @param data Data to print
     * @param size Data size
     * @param title Title for output
     */
    static void printHex(const uint8_t *data, size_t size, const char *title = "Data");

    /**
     * @brief Create request packet
     * @param command Command code
     * @param destAddr Destination address
     * @param srcAddr Source address
     * @param password Password (4 bytes)
     * @param data Data payload
     * @param dataSize Data size
     * @param packet Output packet
     * @return true if packet created successfully
     */
    static bool createRequestPacket(uint8_t command, uint16_t destAddr, uint16_t srcAddr,
                                    uint32_t password, const uint8_t *data, uint8_t dataSize,
                                    PacketData &packet);

    /**
     * @brief Create response packet
     * @param originalRequest Original request packet
     * @param status Status value (4 bytes)
     * @param data Response data
     * @param dataSize Data size
     * @param packet Output packet
     * @return true if packet created successfully
     */
    static bool createResponsePacket(const PacketData &originalRequest, uint32_t status,
                                     const uint8_t *data, uint8_t dataSize, PacketData &packet);

    /**
     * @brief Get command name string
     * @param commandCode Command code
     * @return Command name
     */
    static const char *getCommandName(uint8_t commandCode);

    /**
     * @brief Get energy type name
     * @param energyType Energy type code
     * @return Energy type name
     */
    static const char *getEnergyTypeName(uint8_t energyType);

    /**
     * @brief Get board generation name
     * @param boardId Board ID
     * @return Generation name
     */
    static const char *getBoardGenerationName(uint8_t boardId);
};

#endif // PROTOCOL_UTILS_H
