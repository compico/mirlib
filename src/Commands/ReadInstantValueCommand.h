#ifndef READ_INSTANT_VALUE_COMMAND_H
#define READ_INSTANT_VALUE_COMMAND_H

#include "BaseCommand.h"
#include "../ProtocolUtils.h"

/**
 * @brief Parameter groups for ReadInstantValue
 */
enum ParameterGroup : uint8_t {
    GROUP_BASIC = 0x00, ///< Basic instant values (voltage, current, power, freq, cos)
    GROUP_PHASE_ANGLES = 0x10, ///< Phase angles and power per phase + temperature
    GROUP_TIME_ANGLES = 0x11, ///< Time + angles and power per phase + frequency
    GROUP_TOTAL_POWER = 0x12 ///< Total power + basic values + power per phase
};

/**
 * @brief ReadInstantValue request structure
 */
struct ReadInstantValueRequest {
    ParameterGroup group; ///< Parameter group

    /**
     * @brief Constructor
     * @param paramGroup Parameter group (default: GROUP_BASIC)
     */
    ReadInstantValueRequest(ParameterGroup paramGroup = GROUP_BASIC) : group(paramGroup) {
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array
     * @return Number of bytes written (1)
     */
    size_t toBytes(uint8_t *data) const {
        data[0] = static_cast<uint8_t>(group);
        return 1;
    }

    /**
     * @brief Parse from byte array
     * @param data Input data array
     * @param size Data size
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 1) return false;
        group = static_cast<ParameterGroup>(data[0]);
        return true;
    }
};

/**
 * @brief ReadInstantValue response for transition generation
 */
struct ReadInstantValueResponseTransition {
    ParameterGroup group; ///< Parameter group (always 0x00)
    uint16_t voltageTransformCoeff; ///< Voltage transformation coefficient
    uint16_t currentTransformCoeff; ///< Current transformation coefficient
    uint16_t activePower; ///< Active power
    uint16_t reactivePower; ///< Reactive power
    uint16_t frequency; ///< Frequency (divide by 100 for Hz)
    uint16_t cosPhi; ///< cos φ
    uint16_t voltageA; ///< Voltage phase A (divide by 100 for V)
    uint16_t voltageB; ///< Voltage phase B (divide by 100 for V)
    uint16_t voltageC; ///< Voltage phase C (divide by 100 for V)
    uint32_t currentA; ///< Current phase A (2 or 3 bytes, divide by 1000 for A)
    uint32_t currentB; ///< Current phase B (2 or 3 bytes, divide by 1000 for A)
    uint32_t currentC; ///< Current phase C (2 or 3 bytes, divide by 1000 for A)
    bool is100ASupport; ///< True if 3-byte currents (100A support)

    /**
     * @brief Parse from byte array
     * @param data Data array
     * @param size Data size (25 or 28 bytes)
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 25) return false;

        is100ASupport = (size == 28);
        size_t index = 0;

        group = static_cast<ParameterGroup>(data[index++]);
        voltageTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        currentTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        activePower = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        reactivePower = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        frequency = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        cosPhi = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageA = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageB = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageC = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        if (is100ASupport) {
            // 3-byte currents for 100A support
            // FIXED: explicit cast to uint32_t to avoid shift overflow warning
            currentA = (uint32_t)data[index] |
                      ((uint32_t)data[index + 1] << 8) |
                      ((uint32_t)data[index + 2] << 16);
            index += 3;
            currentB = (uint32_t)data[index] |
                      ((uint32_t)data[index + 1] << 8) |
                      ((uint32_t)data[index + 2] << 16);
            index += 3;
            currentC = (uint32_t)data[index] |
                      ((uint32_t)data[index + 1] << 8) |
                      ((uint32_t)data[index + 2] << 16);
            index += 3;
        } else {
            // 2-byte currents
            currentA = ProtocolUtils::bytesToUint16(&data[index]);
            index += 2;
            currentB = ProtocolUtils::bytesToUint16(&data[index]);
            index += 2;
            currentC = ProtocolUtils::bytesToUint16(&data[index]);
            index += 2;
        }

        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array
     * @return Number of bytes written
     */
    size_t toBytes(uint8_t *data) const {
        size_t index = 0;

        data[index++] = static_cast<uint8_t>(group);
        ProtocolUtils::uint16ToBytes(voltageTransformCoeff, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(currentTransformCoeff, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(activePower, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(reactivePower, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(frequency, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(cosPhi, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageA, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageB, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageC, &data[index]);
        index += 2;

        if (is100ASupport) {
            // 3-byte currents
            data[index++] = currentA & 0xFF;
            data[index++] = (currentA >> 8) & 0xFF;
            data[index++] = (currentA >> 16) & 0xFF;
            data[index++] = currentB & 0xFF;
            data[index++] = (currentB >> 8) & 0xFF;
            data[index++] = (currentB >> 16) & 0xFF;
            data[index++] = currentC & 0xFF;
            data[index++] = (currentC >> 8) & 0xFF;
            data[index++] = (currentC >> 16) & 0xFF;
        } else {
            // 2-byte currents
            ProtocolUtils::uint16ToBytes(currentA, &data[index]);
            index += 2;
            ProtocolUtils::uint16ToBytes(currentB, &data[index]);
            index += 2;
            ProtocolUtils::uint16ToBytes(currentC, &data[index]);
            index += 2;
        }

        return index;
    }

    /**
     * @brief Get frequency in Hz
     * @return Frequency in Hz
     */
    float getFrequencyHz() const {
        return frequency / 100.0f;
    }

    /**
     * @brief Get cos φ value
     * @return cos φ value (-1.000 to +1.000)
     */
    float getCosPhiValue() const {
        if (cosPhi >= 0x8000) {
            return (cosPhi - 0x8000) / -1000.0f;
        } else {
            return cosPhi / 1000.0f;
        }
    }

    /**
     * @brief Get voltage values in volts
     */
    float getVoltageA() const { return voltageA / 100.0f; }
    float getVoltageB() const { return voltageB / 100.0f; }
    float getVoltageC() const { return voltageC / 100.0f; }

    /**
     * @brief Get current values in amperes
     */
    float getCurrentA() const { return currentA / 1000.0f; }
    float getCurrentB() const { return currentB / 1000.0f; }
    float getCurrentC() const { return currentC / 1000.0f; }
};

/**
 * @brief ReadInstantValue response for new generation (group 0x00)
 */
struct ReadInstantValueResponseNewBasic {
    ParameterGroup group; ///< Parameter group
    uint16_t voltageTransformCoeff; ///< Voltage transformation coefficient
    uint16_t currentTransformCoeff; ///< Current transformation coefficient
    uint32_t activePower; ///< Active power in kW (divide by 1000)
    uint32_t reactivePower; ///< Reactive power in kvar (divide by 1000)
    uint16_t frequency; ///< Frequency in Hz (divide by 100)
    uint16_t cosPhi; ///< cos φ (special format)
    uint16_t voltageA; ///< Voltage phase A (divide by 100 for V)
    uint16_t voltageB; ///< Voltage phase B (divide by 100 for V)
    uint16_t voltageC; ///< Voltage phase C (divide by 100 for V)
    uint32_t currentA; ///< Current phase A (divide by 1000 for A)
    uint32_t currentB; ///< Current phase B (divide by 1000 for A)
    uint32_t currentC; ///< Current phase C (divide by 1000 for A)

    /**
     * @brief Parse from byte array
     * @param data Data array (30 bytes)
     * @param size Data size
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size < 30) return false;

        size_t index = 0;

        group = static_cast<ParameterGroup>(data[index++]);
        voltageTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        currentTransformCoeff = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // 3-byte values for power
        // FIXED: explicit cast to uint32_t to avoid shift overflow warning
        activePower = (uint32_t)data[index] |
                     ((uint32_t)data[index + 1] << 8) |
                     ((uint32_t)data[index + 2] << 16);
        index += 3;
        reactivePower = (uint32_t)data[index] |
                       ((uint32_t)data[index + 1] << 8) |
                       ((uint32_t)data[index + 2] << 16);
        index += 3;

        frequency = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        cosPhi = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageA = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageB = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;
        voltageC = ProtocolUtils::bytesToUint16(&data[index]);
        index += 2;

        // 3-byte currents
        // FIXED: explicit cast to uint32_t to avoid shift overflow warning
        currentA = (uint32_t)data[index] |
                  ((uint32_t)data[index + 1] << 8) |
                  ((uint32_t)data[index + 2] << 16);
        index += 3;
        currentB = (uint32_t)data[index] |
                  ((uint32_t)data[index + 1] << 8) |
                  ((uint32_t)data[index + 2] << 16);
        index += 3;
        currentC = (uint32_t)data[index] |
                  ((uint32_t)data[index + 1] << 8) |
                  ((uint32_t)data[index + 2] << 16);
        index += 3;

        return true;
    }

    /**
     * @brief Convert to byte array
     * @param data Output data array
     * @return Number of bytes written (30)
     */
    size_t toBytes(uint8_t *data) const {
        size_t index = 0;

        data[index++] = static_cast<uint8_t>(group);
        ProtocolUtils::uint16ToBytes(voltageTransformCoeff, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(currentTransformCoeff, &data[index]);
        index += 2;

        // 3-byte power values
        data[index++] = activePower & 0xFF;
        data[index++] = (activePower >> 8) & 0xFF;
        data[index++] = (activePower >> 16) & 0xFF;
        data[index++] = reactivePower & 0xFF;
        data[index++] = (reactivePower >> 8) & 0xFF;
        data[index++] = (reactivePower >> 16) & 0xFF;

        ProtocolUtils::uint16ToBytes(frequency, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(cosPhi, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageA, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageB, &data[index]);
        index += 2;
        ProtocolUtils::uint16ToBytes(voltageC, &data[index]);
        index += 2;

        // 3-byte currents
        data[index++] = currentA & 0xFF;
        data[index++] = (currentA >> 8) & 0xFF;
        data[index++] = (currentA >> 16) & 0xFF;
        data[index++] = currentB & 0xFF;
        data[index++] = (currentB >> 8) & 0xFF;
        data[index++] = (currentB >> 16) & 0xFF;
        data[index++] = currentC & 0xFF;
        data[index++] = (currentC >> 8) & 0xFF;
        data[index++] = (currentC >> 16) & 0xFF;

        return 30;
    }

    /**
     * @brief Get frequency in Hz
     */
    float getFrequencyHz() const {
        return frequency / 100.0f;
    }

    /**
     * @brief Get cos φ value (-1.000 to +1.000)
     */
    float getCosPhiValue() const {
        if (cosPhi >= 0x8000) {
            return (cosPhi - 0x8000) / -1000.0f;
        } else {
            return cosPhi / 1000.0f;
        }
    }

    /**
     * @brief Get voltage values in volts
     */
    float getVoltageA() const { return voltageA / 100.0f; }
    float getVoltageB() const { return voltageB / 100.0f; }
    float getVoltageC() const { return voltageC / 100.0f; }

    /**
     * @brief Get current values in amperes
     */
    float getCurrentA() const { return currentA / 1000.0f; }
    float getCurrentB() const { return currentB / 1000.0f; }
    float getCurrentC() const { return currentC / 1000.0f; }

    /**
     * @brief Get power values in kW/kvar
     */
    float getActivePowerKW() const { return activePower / 1000.0f; }
    float getReactivePowerKvar() const { return reactivePower / 1000.0f; }
};

/**
 * @brief ReadInstantValue command implementation
 */
class ReadInstantValueCommand : public BaseCommand {
public:
    /**
     * @brief Constructor
     */
    ReadInstantValueCommand() : BaseCommand(CMD_READ_INSTANT_VALUE), m_generation(0), m_isOldGeneration(true) {
    }

    /**
     * @brief Set device generation info
     * @param boardId Board ID
     * @param role Role value
     */
    void setGeneration(uint8_t boardId, uint8_t role) {
        GenerationInfo info = ProtocolUtils::determineGeneration(boardId, role);
        m_isOldGeneration = info.isOldGeneration;
        m_isTransitionGeneration = info.isTransitionGeneration;
        m_isNewGeneration = info.isNewGeneration;
        m_generation = boardId;
    }

    /**
     * @brief Set request parameters
     * @param group Parameter group
     */
    void setRequest(ParameterGroup group = GROUP_BASIC) {
        m_request = ReadInstantValueRequest(group);
    }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size
     */
    size_t prepareRequest(uint8_t *requestData, size_t maxSize) override {
        if (m_isOldGeneration) {
            return 0; // Old generation doesn't support this command
        }
        return m_request.toBytes(requestData);
    }

    /**
     * @brief Parse response data
     * @param responseData Response data buffer
     * @param dataSize Response data size
     * @return true if parsing successful
     */
    bool parseResponse(const uint8_t *responseData, size_t dataSize) override {
        if (m_isOldGeneration) {
            return false; // Not supported
        } else if (m_isTransitionGeneration) {
            return m_responseTransition.fromBytes(responseData, dataSize);
        } else if (m_isNewGeneration) {
            return m_responseNewBasic.fromBytes(responseData, dataSize);
        }
        return false;
    }

    /**
     * @brief Handle request (for server mode)
     */
    size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                         uint8_t *responseData, size_t maxResponseSize) override {
        if (m_isOldGeneration) {
            return 0; // Not supported
        }

        // Parse request
        ReadInstantValueRequest request;
        if (!request.fromBytes(requestData, dataSize)) {
            return 0;
        }

        if (m_isTransitionGeneration) {
            if (maxResponseSize < 25) return 0;
            return m_responseTransition.toBytes(responseData);
        } else if (m_isNewGeneration) {
            if (maxResponseSize < 30) return 0;
            return m_responseNewBasic.toBytes(responseData);
        }

        return 0;
    }

    /**
     * @brief Get command name
     */
    const char *getCommandName() const override {
        return "ReadInstantValue";
    }

    /**
     * @brief Check if command is valid for device generation
     */
    bool isValidForGeneration(uint8_t boardId, uint8_t role) const override {
        GenerationInfo info = ProtocolUtils::determineGeneration(boardId, role);
        return !info.isOldGeneration; // Not supported by old generation
    }

    /**
     * @brief Get minimum required data size for request
     */
    size_t getMinRequestSize() const override {
        return m_isOldGeneration ? 0 : 1;
    }

    /**
     * @brief Get expected response size range
     */
    void getResponseSizeRange(size_t &minSize, size_t &maxSize) const override {
        if (m_isOldGeneration) {
            minSize = 0;
            maxSize = 0;
        } else if (m_isTransitionGeneration) {
            minSize = 25;
            maxSize = 28;
        } else {
            minSize = 30;
            maxSize = 30;
        }
    }

    /**
     * @brief Get response data for different generations
     */
    const ReadInstantValueResponseTransition &getTransitionResponse() const {
        return m_responseTransition;
    }

    const ReadInstantValueResponseNewBasic &getNewBasicResponse() const {
        return m_responseNewBasic;
    }

    /**
     * @brief Check generation types
     */
    bool isOldGeneration() const { return m_isOldGeneration; }
    bool isTransitionGeneration() const { return m_isTransitionGeneration; }
    bool isNewGeneration() const { return m_isNewGeneration; }

    /**
     * @brief Set server response data
     */
    void setServerResponseTransition(const ReadInstantValueResponseTransition &response) {
        m_responseTransition = response;
    }

    void setServerResponseNewBasic(const ReadInstantValueResponseNewBasic &response) {
        m_responseNewBasic = response;
    }

private:
    ReadInstantValueRequest m_request;
    ReadInstantValueResponseTransition m_responseTransition;
    ReadInstantValueResponseNewBasic m_responseNewBasic;
    uint8_t m_generation;
    bool m_isOldGeneration;
    bool m_isTransitionGeneration;
    bool m_isNewGeneration;
};

#endif // READ_INSTANT_VALUE_COMMAND_H
