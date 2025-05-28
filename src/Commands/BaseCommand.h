#ifndef BASE_COMMAND_H
#define BASE_COMMAND_H

#include <Arduino.h>
#include "../ProtocolTypes.h"

/**
 * @brief Base class for all protocol commands
 * 
 * This class provides a template for implementing protocol commands.
 * Each command should inherit from this class and implement the required methods.
 */
class BaseCommand {
public:
    /**
     * @brief Constructor
     * @param commandCode Command code (0x01, 0x05, 0x30, etc.)
     */
    explicit BaseCommand(uint8_t commandCode) : m_commandCode(commandCode) {
    }

    /**
     * @brief Virtual destructor
     */
    virtual ~BaseCommand() = default;

    /**
     * @brief Get command code
     * @return Command code
     */
    uint8_t getCommandCode() const { return m_commandCode; }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size, 0 if error
     */
    virtual size_t prepareRequest(uint8_t *requestData, size_t maxSize) = 0;

    /**
     * @brief Parse response data
     * @param responseData Response data buffer
     * @param dataSize Response data size
     * @return true if parsing successful
     */
    virtual bool parseResponse(const uint8_t *responseData, size_t dataSize) = 0;

    /**
     * @brief Handle request (for server mode)
     * @param requestData Request data buffer
     * @param dataSize Request data size
     * @param responseData Buffer for response data
     * @param maxResponseSize Maximum response buffer size
     * @return Response data size, 0 if error
     */
    virtual size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                                 uint8_t *responseData, size_t maxResponseSize) = 0;

    /**
     * @brief Get command name
     * @return Command name string
     */
    virtual const char *getCommandName() const = 0;

    /**
     * @brief Check if command is valid for device generation
     * @param boardId generation Device generation
     * @return true if command is supported
     */
    virtual bool isValidForGeneration(uint8_t boardId, uint8_t role) const = 0;

    /**
     * @brief Get minimum required data size for request
     * @return Minimum data size
     */
    virtual size_t getMinRequestSize() const = 0;

    /**
     * @brief Get expected response size range
     * @param minSize Minimum response size
     * @param maxSize Maximum response size
     */
    virtual void getResponseSizeRange(size_t &minSize, size_t &maxSize) const = 0;

    /**
     * @brief Check if command requires password
     * @return true if password required
     */
    virtual bool requiresPassword() const { return false; }

    /**
     * @brief Validate request data
     * @param dataSize Data size
     * @return true if request is valid
     */
    virtual bool validateRequest(size_t dataSize) const {
        return dataSize >= getMinRequestSize();
    }

    /**
     * @brief Validate response data
     * @param dataSize Data size
     * @return true if response is valid
     */
    virtual bool validateResponse(size_t dataSize) const {
        size_t minSize, maxSize;
        getResponseSizeRange(minSize, maxSize);
        return dataSize >= minSize && dataSize <= maxSize;
    }

protected:
    uint8_t m_commandCode; ///< Command code
};

/**
 * @brief Template base class for typed commands
 *
 * This template provides type-safe access to request and response data.
 */
template<typename RequestType, typename ResponseType>
class TypedCommand : public BaseCommand {
public:
    explicit TypedCommand(uint8_t commandCode) : BaseCommand(commandCode) {
    }

    /**
     * @brief Set request data
     * @param request Request data
     */
    void setRequest(const RequestType &request) {
        m_request = request;
    }

    /**
     * @brief Get request data
     * @return Request data
     */
    const RequestType &getRequest() const {
        return m_request;
    }

    /**
     * @brief Get response data
     * @return Response data
     */
    const ResponseType &getResponse() const {
        return m_response;
    }

    /**
     * @brief Get mutable response data
     * @return Mutable response data
     */
    ResponseType &getResponseMutable() {
        return m_response;
    }

protected:
    RequestType m_request; ///< Request data
    ResponseType m_response; ///< Response data
};

#endif // BASE_COMMAND_H
