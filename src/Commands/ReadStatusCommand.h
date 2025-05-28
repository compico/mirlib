#ifndef READ_STATUS_COMMAND_H
#define READ_STATUS_COMMAND_H

#include "BaseCommand.h"
#include "../ProtocolUtils.h"

/**
 * @brief Read Status command request structure
 */
struct ReadStatusRequest {
    EnergyType energyType; ///< Energy type (only for transition/new generation)

    /**
     * @brief Constructor for old generation (no energy type parameter)
     */
    ReadStatusRequest() : energyType(ACTIVE_FORWARD) {
    }

    /**
     * @brief Constructor for transition/new generation
     * @param type Energy type
     */
    ReadStatusRequest(EnergyType type) : energyType(type) {
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array
     * @param isOldGeneration True for old generation (no data)
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data, bool isOldGeneration) const {
        if (isOldGeneration) {
            return 0; // Old generation has no request data
        } else {
            data[0] = static_cast<uint8_t>(energyType);
            return 1;
        }
    }

    /**
     * @brief Parse from byte array
     * @param data Input data array
     * @param size Data size
     * @param isOldGeneration True for old generation
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size, bool isOldGeneration) {
        if (isOldGeneration) {
            energyType = ACTIVE_FORWARD; // Default for old generation
            return size == 0;
        } else {
            if (size < 1) return false;
            energyType = static_cast<EnergyType>(data[0]);
            return true;
        }
    }
};

/**
 * @brief Read Status command response structure for old generation
 */
struct ReadStatusResponseOld {
    uint32_t totalEnergy; ///< Total energy value (4 bytes)
    ConfigByte configByte; ///< Configuration byte
    uint8_t divisionCoeff; ///< Division coefficient
    uint8_t roleCode; ///< Role code (not used)
    uint32_t multiplicationCoeff; ///< Multiplication coefficient (3 bytes, always = 1)
    uint32_t tariffValues[4]; ///< Tariff values (16 bytes)

    /**
     * @brief Parse from byte array
     * @param data Data array (26 bytes)
     * @param size Data size
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 26) return false;

        size_t index = 0;

        // Total energy (4 bytes)
        totalEnergy = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Configuration byte
        configByte.fromByte(data[index++]);

        // Division coefficient
        divisionCoeff = data[index++];

        // Role code
        roleCode = data[index++];

        // Multiplication coefficient (3 bytes, but we store as uint32_t)
        // FIXED: explicit cast to uint32_t to avoid shift overflow warning
        multiplicationCoeff = (uint32_t)data[index] |
                             ((uint32_t)data[index + 1] << 8) |
                             ((uint32_t)data[index + 2] << 16);
        index += 3;

        // Tariff values (16 bytes = 4 x 4 bytes)
        for (int i = 0; i < 4; i++) {
            tariffValues[i] = ProtocolUtils::bytesToUint32(&data[index]);
            index += 4;
        }

        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array (must be at least 26 bytes)
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data) const {
        size_t index = 0;

        // Total energy (4 bytes)
        ProtocolUtils::uint32ToBytes(totalEnergy, &data[index]);
        index += 4;

        // Configuration byte
        data[index++] = configByte.toByte();

        // Division coefficient
        data[index++] = divisionCoeff;

        // Role code
        data[index++] = roleCode;

        // Multiplication coefficient (3 bytes)
        data[index++] = multiplicationCoeff & 0xFF;
        data[index++] = (multiplicationCoeff >> 8) & 0xFF;
        data[index++] = (multiplicationCoeff >> 16) & 0xFF;

        // Tariff values (16 bytes = 4 x 4 bytes)
        for (int i = 0; i < 4; i++) {
            ProtocolUtils::uint32ToBytes(tariffValues[i], &data[index]);
            index += 4;
        }

        return 26;
    }
};

/**
 * @brief Read Status command response structure for transition/new generation
 */
struct ReadStatusResponseNew {
    EnergyType energyType; ///< Energy type
    ConfigByte configByte; ///< Configuration byte
    uint16_t voltageTransformCoeff; ///< Voltage transformation coefficient
    uint16_t currentTransformCoeff; ///< Current transformation coefficient
    uint32_t totalFull; ///< Total full sum (4 bytes)
    uint32_t totalActive; ///< Total active sum (4 bytes)
    uint32_t tariffValues[4]; ///< Tariff values (16 bytes)

    /**
     * @brief Parse from byte array
     * @param data Data array (30 or 31 bytes for new generation)
     * @param size Data size
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 30) return false;

        size_t index = 0;

        // Energy type
        energyType = static_cast<EnergyType>(data[index++]);

        // Configuration byte
        configByte.fromByte(data[index++]);

        // Voltage transformation coefficient
        voltageTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Current transformation coefficient
        currentTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // Total full sum
        totalFull = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Total active sum
        totalActive = ProtocolUtils::bytesToUint32(&data[index]);
        index += 4;

        // Tariff values (16 bytes = 4 x 4 bytes)
        for (int i = 0; i < 4; i++) {
            tariffValues[i] = ProtocolUtils::bytesToUint32(&data[index]);
            index += 4;
        }

        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array (must be at least 30 bytes)
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data) const {
        size_t index = 0;

        // Energy type
        data[index++] = static_cast<uint8_t>(energyType);

        // Configuration byte
        data[index++] = configByte.toByte();

        // Voltage transformation coefficient
        ProtocolUtils::uint16ToBytes(voltageTransformCoeff, &data[index]);
        index += 2;

        // Current transformation coefficient
        ProtocolUtils::uint16ToBytes(currentTransformCoeff, &data[index]);
        index += 2;

        // Total full sum
        ProtocolUtils::uint32ToBytes(totalFull, &data[index]);
        index += 4;

        // Total active sum
        ProtocolUtils::uint32ToBytes(totalActive, &data[index]);
        index += 4;

        // Tariff values (16 bytes = 4 x 4 bytes)
        for (int i = 0; i < 4; i++) {
            ProtocolUtils::uint32ToBytes(tariffValues[i], &data[index]);
            index += 4;
        }

        return 30;
    }
};

/**
 * @brief Read Status command implementation
 *
 * Command 0x05 - Read counter status
 * Behavior differs between device generations
 */
class ReadStatusCommand : public BaseCommand {
public:
    /**
     * @brief Constructor
     */
    ReadStatusCommand() : BaseCommand(CMD_READ_STATUS), m_generation(0), m_isOldGeneration(true) {
    }

    /**
     * @brief Set device generation info
     * @param boardId Board ID
     * @param role Role value
     */
    void setGeneration(uint8_t boardId, uint8_t role) {
        GenerationInfo info = ProtocolUtils::determineGeneration(boardId, role);
        m_isOldGeneration = info.isOldGeneration;
        m_generation = boardId;
    }

    /**
     * @brief Set request parameters
     * @param energyType Energy type (ignored for old generation)
     */
    void setRequest(EnergyType energyType = ACTIVE_FORWARD) {
        m_request = ReadStatusRequest(energyType);
    }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size
     */
    size_t prepareRequest(uint8_t *requestData, size_t maxSize) override {
        return m_request.toBytes(requestData, m_isOldGeneration);
    }

    /**
     * @brief Parse response data
     * @param responseData Response data buffer
     * @param dataSize Response data size
     * @return true if parsing successful
     */
    bool parseResponse(const uint8_t *responseData, size_t dataSize) override {
        if (m_isOldGeneration) {
            return m_responseOld.fromBytes(responseData, dataSize);
        } else {
            return m_responseNew.fromBytes(responseData, dataSize);
        }
    }

    /**
     * @brief Handle request (for server mode)
     * @param requestData Request data buffer
     * @param dataSize Request data size
     * @param responseData Buffer for response data
     * @param maxResponseSize Maximum response buffer size
     * @return Response data size
     */
    size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                         uint8_t *responseData, size_t maxResponseSize) override {
        // Parse request
        ReadStatusRequest request;
        if (!request.fromBytes(requestData, dataSize, m_isOldGeneration)) {
            return 0;
        }

        // Generate response based on generation
        if (m_isOldGeneration) {
            if (maxResponseSize < 26) return 0;
            return m_responseOld.toBytes(responseData);
        } else {
            if (maxResponseSize < 30) return 0;
            return m_responseNew.toBytes(responseData);
        }
    }

    /**
     * @brief Get command name
     * @return Command name string
     */
    const char *getCommandName() const override {
        return "ReadStatus";
    }

    /**
     * @brief Check if command is valid for device generation
     * @param boardId Board ID
     * @param role Role value
     * @return true (ReadStatus is supported by all generations)
     */
    bool isValidForGeneration(uint8_t boardId, uint8_t role) const override {
        return true; // Supported by all generations
    }

    /**
     * @brief Get minimum required data size for request
     * @return 0 for old generation, 1 for new/transition
     */
    size_t getMinRequestSize() const override {
        return m_isOldGeneration ? 0 : 1;
    }

    /**
     * @brief Get expected response size range
     * @param minSize Minimum response size
     * @param maxSize Maximum response size
     */
    void getResponseSizeRange(size_t &minSize, size_t &maxSize) const override {
        if (m_isOldGeneration) {
            minSize = 26;
            maxSize = 26;
        } else {
            minSize = 30;
            maxSize = 31; // New generation can have 31 bytes
        }
    }

    /**
     * @brief Get old generation response
     * @return Old generation response data
     */
    const ReadStatusResponseOld &getOldResponse() const {
        return m_responseOld;
    }

    /**
     * @brief Get new generation response
     * @return New generation response data
     */
    const ReadStatusResponseNew &getNewResponse() const {
        return m_responseNew;
    }

    /**
     * @brief Check if using old generation format
     * @return true if old generation
     */
    bool isOldGeneration() const {
        return m_isOldGeneration;
    }

    /**
     * @brief Set server response data for old generation
     */
    void setServerResponseOld(const ReadStatusResponseOld &response) {
        m_responseOld = response;
    }

    /**
     * @brief Set server response data for new generation
     */
    void setServerResponseNew(const ReadStatusResponseNew &response) {
        m_responseNew = response;
    }

private:
    ReadStatusRequest m_request;
    ReadStatusResponseOld m_responseOld;
    ReadStatusResponseNew m_responseNew;
    uint8_t m_generation;
    bool m_isOldGeneration;
};

#endif // READ_STATUS_COMMAND_H
