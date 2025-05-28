#ifndef GET_INFO_COMMAND_H
#define GET_INFO_COMMAND_H

#include "BaseCommand.h"
#include "../ProtocolUtils.h"

/**
 * @brief GetInfo command request structure (empty)
 */
struct GetInfoRequest {
    // No data for GetInfo request
};

/**
 * @brief GetInfo command response structure (common fields)
 */
struct GetInfoResponseBase {
    uint8_t boardId; ///< Board ID (identifies generation)
    uint16_t firmwareVersion; ///< Firmware version (2 bytes)
    uint16_t firmwareCRC; ///< Firmware CRC16 (2 bytes)
    uint32_t workTime; ///< Work time in seconds (4 bytes)
    uint32_t sleepTime; ///< Sleep time in seconds (4 bytes)
    uint8_t groupId; ///< Group membership (1 byte)
    uint8_t flags; ///< Flags byte (bit 7 = 100A support, etc.)
    uint16_t activeTariffCRC; ///< Active tariff CRC16 (2 bytes)
    uint16_t plannedTariffCRC; ///< Planned tariff CRC16 (2 bytes)
    uint32_t timeSinceCorrection; ///< Time since time correction (4 bytes)
    uint16_t reserve; ///< Reserve bytes (2 bytes)
    uint8_t interface1Type; ///< Interface 1 type (UART1/Interface1)
    uint8_t interface2Type; ///< Interface 2 type (UART2/Interface2)

    // New generation additional fields
    uint8_t interface3Type; ///< Interface 3 type (new generation only)
    uint8_t interface4Type; ///< Interface 4 type (new generation only)
    uint16_t batteryVoltage; ///< Battery voltage (new generation only)

    /**
     * @brief Parse common fields from byte array
     * @param data Data array
     * @param size Data size
     * @param isNewGeneration True for new generation (28 or 31 bytes)
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size, bool isNewGeneration) {
        if (size < 27) return false;

        size_t index = 0;

        // Board ID
        boardId = data[index++];

        // Firmware version
        firmwareVersion = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Firmware CRC
        firmwareCRC = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Work time
        workTime = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Sleep time
        sleepTime = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Group ID
        groupId = data[index++];

        // Flags
        flags = data[index++];

        // Active tariff CRC
        activeTariffCRC = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Planned tariff CRC
        plannedTariffCRC = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Time since correction
        timeSinceCorrection = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Reserve
        reserve = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Interface types
        interface1Type = data[index++];
        interface2Type = data[index++];

        // New generation additional fields
        if (isNewGeneration && size >= 28) {
            interface3Type = data[index++];
            interface4Type = data[index++];

            if (size >= 31) {
                batteryVoltage = ProtocolUtils::bytesToUint16(&data[index]);
                index += 2;
            } else {
                batteryVoltage = 0;
            }
        } else {
            interface3Type = 0;
            interface4Type = 0;
            batteryVoltage = 0;
        }

        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array
     * @param isNewGeneration True for new generation format
     * @param includeBattery True to include battery voltage (31 bytes total)
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data, bool isNewGeneration, bool includeBattery = false) const {
        size_t index = 0;

        // Board ID
        data[index++] = boardId;

        // Firmware version
        ProtocolUtils::uint16ToBytes(firmwareVersion, &data[index]);
        index += 2;

        // Firmware CRC
        ProtocolUtils::uint16ToBytes(firmwareCRC, &data[index]);
        index += 2;

        // Work time
        ProtocolUtils::uint32ToBytes(workTime, &data[index]);
        index += 4;

        // Sleep time
        ProtocolUtils::uint32ToBytes(sleepTime, &data[index]);
        index += 4;

        // Group ID
        data[index++] = groupId;

        // Flags
        data[index++] = flags;

        // Active tariff CRC
        ProtocolUtils::uint16ToBytes(activeTariffCRC, &data[index]);
        index += 2;

        // Planned tariff CRC
        ProtocolUtils::uint16ToBytes(plannedTariffCRC, &data[index]);
        index += 2;

        // Time since correction
        ProtocolUtils::uint32ToBytes(timeSinceCorrection, &data[index]);
        index += 4;

        // Reserve
        ProtocolUtils::uint16ToBytes(reserve, &data[index]);
        index += 2;

        // Interface types
        data[index++] = interface1Type;
        data[index++] = interface2Type;

        // New generation additional fields
        if (isNewGeneration) {
            data[index++] = interface3Type;
            data[index++] = interface4Type;

            if (includeBattery) {
                ProtocolUtils::uint16ToBytes(batteryVoltage, &data[index]);
                index += 2;
            }
        }

        return index;
    }

    /**
     * @brief Check if device supports 100A
     * @return true if 100A support flag is set
     */
    bool supports100A() const {
        return (flags & 0x80) != 0;
    }

    /**
     * @brief Check if device has street lighting control (new generation)
     * @return true if street lighting flag is set
     */
    bool hasStreetLightingControl() const {
        return (flags & 0x40) != 0;
    }

    /**
     * @brief Set 100A support flag
     * @param enable Enable 100A support
     */
    void set100ASupport(bool enable) {
        if (enable) {
            flags |= 0x80;
        } else {
            flags &= ~0x80;
        }
    }

    /**
     * @brief Set street lighting control flag (new generation)
     * @param enable Enable street lighting control
     */
    void setStreetLightingControl(bool enable) {
        if (enable) {
            flags |= 0x40;
        } else {
            flags &= ~0x40;
        }
    }
};

/**
 * @brief GetInfo command implementation
 *
 * Command 0x30 - Read extended device information
 * Response size depends on device generation:
 * - Old/Transition: 27 bytes
 * - New: 28 or 31 bytes
 */
class GetInfoCommand : public TypedCommand<GetInfoRequest, GetInfoResponseBase> {
public:
    /**
     * @brief Constructor
     */
    GetInfoCommand() : TypedCommand(CMD_GET_INFO), m_generation(0), m_isNewGeneration(false) {
    }

    /**
     * @brief Set expected device generation (for response parsing)
     * @param boardId Board ID
     * @param role Role value
     */
    void setExpectedGeneration(uint8_t boardId, uint8_t role = 0) {
        GenerationInfo const info = ProtocolUtils::determineGeneration(boardId, role);
        m_isNewGeneration = info.isNewGeneration;
        m_generation = boardId;
    }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size (0 for GetInfo)
     */
    size_t prepareRequest(uint8_t *requestData, size_t maxSize) override {
        // GetInfo has no request data
        return 0;
    }

    /**
     * @brief Parse response data
     * @param responseData Response data buffer
     * @param dataSize Response data size
     * @return true if parsing successful
     */
    bool parseResponse(const uint8_t *responseData, size_t dataSize) override {
        // Try to auto-detect generation from response size if not set
        if (m_generation == 0) {
            if (dataSize >= 28) {
                m_isNewGeneration = true;
            } else {
                m_isNewGeneration = false;
            }
        }

        return m_response.fromBytes(responseData, dataSize, m_isNewGeneration);
    }

    /**
     * @brief Handle request (for server mode)
     * @param requestData Request data buffer (empty for GetInfo)
     * @param dataSize Request data size (0 for GetInfo)
     * @param responseData Buffer for response data
     * @param maxResponseSize Maximum response buffer size
     * @return Response data size
     */
    size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                         uint8_t *responseData, size_t maxResponseSize) override {
        // Determine response size based on generation
        size_t requiredSize = m_isNewGeneration ? (m_response.batteryVoltage != 0 ? 31 : 28) : 27;

        if (maxResponseSize < requiredSize) {
            return 0;
        }

        return m_response.toBytes(responseData, m_isNewGeneration,
                                  m_isNewGeneration && m_response.batteryVoltage != 0);
    }

    /**
     * @brief Get command name
     * @return Command name string
     */
    const char *getCommandName() const override {
        return "GetInfo";
    }

    /**
     * @brief Check if command is valid for device generation
     * @param boardId Board ID
     * @param role Role value
     * @return true (GetInfo is supported by all generations)
     */
    bool isValidForGeneration(uint8_t boardId, uint8_t role) const override {
        return true; // Supported by all generations
    }

    /**
     * @brief Get minimum required data size for request
     * @return 0 (no request data)
     */
    size_t getMinRequestSize() const override {
        return 0;
    }

    /**
     * @brief Get expected response size range
     * @param minSize Minimum response size (27 bytes)
     * @param maxSize Maximum response size (31 bytes)
     */
    void getResponseSizeRange(size_t &minSize, size_t &maxSize) const override {
        minSize = 27;
        maxSize = 31;
    }

    /**
     * @brief Get board ID from response
     * @return Board ID
     */
    uint8_t getBoardId() const {
        return m_response.boardId;
    }

    /**
     * @brief Get firmware version from response
     * @return Firmware version
     */
    uint16_t getFirmwareVersion() const {
        return m_response.firmwareVersion;
    }

    /**
     * @brief Get device generation info
     * @return Generation info structure
     */
    GenerationInfo getGenerationInfo() const {
        // For GetInfo, we don't have role, so we determine by board ID only
        return ProtocolUtils::determineGeneration(m_response.boardId, 0x32);
    }

    /**
     * @brief Check if device supports 100A
     * @return true if 100A support flag is set
     */
    bool supports100A() const {
        return m_response.supports100A();
    }

    /**
     * @brief Check if device has street lighting control
     * @return true if street lighting control is available
     */
    bool hasStreetLightingControl() const {
        return m_response.hasStreetLightingControl();
    }

    /**
     * @brief Set server response data
     * @param response Response data to set
     */
    void setServerResponse(const GetInfoResponseBase &response) {
        m_response = response;

        // Auto-detect generation from board ID
        GenerationInfo info = ProtocolUtils::determineGeneration(response.boardId, 0x32);
        m_isNewGeneration = info.isNewGeneration;
    }

    /**
     * @brief Check if using new generation format
     * @return true if new generation
     */
    bool isNewGeneration() const {
        return m_isNewGeneration;
    }

private:
    uint8_t m_generation;
    bool m_isNewGeneration;
};

#endif // GET_INFO_COMMAND_H
