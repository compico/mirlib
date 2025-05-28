#ifndef PING_COMMAND_H
#define PING_COMMAND_H

#include "BaseCommand.h"
#include "../ProtocolUtils.h"

/**
 * @brief Ping command request structure (empty)
 */
struct PingRequest {
    // No data for ping request
};

/**
 * @brief Ping command response structure
 */
struct PingResponse {
    uint16_t firmwareVersion; ///< Firmware version (2 bytes)
    uint16_t deviceAddress; ///< Device address (2 bytes)

    /**
     * @brief Parse from byte array
     * @param data Data array (4 bytes)
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 4) {
            return false;
        }

        firmwareVersion = ProtocolUtils::bytesToUint16(&data[0]);
        deviceAddress = ProtocolUtils::bytesToUint16(&data[2]);
        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array (must be at least 4 bytes)
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data) const {
        ProtocolUtils::uint16ToBytes(firmwareVersion, &data[0]);
        ProtocolUtils::uint16ToBytes(deviceAddress, &data[2]);
        return 4;
    }
};

/**
 * @brief Ping command implementation
 * 
 * Command 0x01 - Communication check
 * Supported by all device generations
 */
class PingCommand : public TypedCommand<PingRequest, PingResponse> {
public:
    /**
     * @brief Constructor
     */
    PingCommand() : TypedCommand(CMD_PING) {
    }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size (0 for ping)
     */
    size_t prepareRequest(uint8_t *requestData, size_t maxSize) override {
        // Ping has no request data
        return 0;
    }

    /**
     * @brief Parse response data
     * @param responseData Response data buffer
     * @param dataSize Response data size
     * @return true if parsing successful
     */
    bool parseResponse(const uint8_t *responseData, size_t dataSize) override {
        return m_response.fromBytes(responseData, dataSize);
    }

    /**
     * @brief Handle request (for server mode)
     * @param requestData Request data buffer (empty for ping)
     * @param dataSize Request data size (0 for ping)
     * @param responseData Buffer for response data
     * @param maxResponseSize Maximum response buffer size
     * @return Response data size
     */
    size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                         uint8_t *responseData, size_t maxResponseSize) override {
        if (maxResponseSize < 4) {
            return 0;
        }

        // For server mode, use configured values
        // These should be set by the server before handling requests
        return m_response.toBytes(responseData);
    }

    /**
     * @brief Get command name
     * @return Command name string
     */
    const char *getCommandName() const override {
        return "Ping";
    }

    /**
     * @brief Check if command is valid for device generation
     * @param boardId Board ID
     * @param role Role value
     * @return true (ping is supported by all generations)
     */
    bool isValidForGeneration(uint8_t boardId, uint8_t role) const override {
        // Ping is supported by all generations
        return true;
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
     * @param minSize Minimum response size (4 bytes)
     * @param maxSize Maximum response size (4 bytes)
     */
    void getResponseSizeRange(size_t &minSize, size_t &maxSize) const override {
        minSize = 4;
        maxSize = 4;
    }

    /**
     * @brief Set server response data
     * @param firmwareVersion Firmware version
     * @param deviceAddress Device address
     */
    void setServerResponse(uint16_t firmwareVersion, uint16_t deviceAddress) {
        m_response.firmwareVersion = firmwareVersion;
        m_response.deviceAddress = deviceAddress;
    }

    /**
     * @brief Get firmware version from response
     * @return Firmware version
     */
    uint16_t getFirmwareVersion() const {
        return m_response.firmwareVersion;
    }

    /**
     * @brief Get device address from response
     * @return Device address
     */
    uint16_t getDeviceAddress() const {
        return m_response.deviceAddress;
    }
};

#endif // PING_COMMAND_H
