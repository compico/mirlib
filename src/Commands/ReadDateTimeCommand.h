#ifndef READ_DATE_TIME_COMMAND_H
#define READ_DATE_TIME_COMMAND_H

#include "BaseCommand.h"
#include "../ProtocolUtils.h"

/**
 * @brief ReadDateTime command request structure
 */
struct ReadDateTimeRequest {
    /**
     * @brief Convert to byte array
     * @return Always 0 (no data)
     */
    size_t toBytes(uint8_t *data) const {
        return 0;
    }

    /**
     * @brief Parse from byte array
     * @return true if parsing successful (always true for empty request)
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        return size == 0;
    }
};

/**
 * @brief ReadDateTime command response structure (7 байт)
 */
struct ReadDateTimeResponse {
    uint8_t seconds; ///< Секунды (0-59)
    uint8_t minutes; ///< Минуты (0-59)
    uint8_t hours; ///< Часы (0-23)
    uint8_t dayOfWeek; ///< День недели (0=Вс, 1=Пн, 2=Вт, 3=Ср, 4=Чт, 5=Пт, 6=Сб)
    uint8_t day; ///< День (1-31)
    uint8_t month; ///< Месяц (1-12)
    uint8_t year; ///< Год (последние 2 цифры, например 24 для 2024)

    /**
     * @brief Parse from byte array (7 байт)
     * @param data Data array
     * @param size Data size (должен быть 7)
     * @return true if parsing successful
     */
    bool fromBytes(const uint8_t *data, size_t size) {
        if (size != 7) return false;

        seconds = data[0];
        minutes = data[1];
        hours = data[2];
        dayOfWeek = data[3];
        day = data[4];
        month = data[5];
        year = data[6];

        return true;
    }

    /**
     * @brief Convert to byte array (7 bytes)
     * @param data Output array (must be at least 7 bytes)
     * @return Number of bytes written (always 7)
     */
    size_t toBytes(uint8_t *data) const {
        data[0] = seconds;
        data[1] = minutes;
        data[2] = hours;
        data[3] = dayOfWeek;
        data[4] = day;
        data[5] = month;
        data[6] = year;

        return 7;
    }

    /**
     * @brief Get day of week name
     * @return Day name in Russian
     */
    const char *getDayOfWeekName() const {
        switch (dayOfWeek) {
            case 0: return "Воскресенье";
            case 1: return "Понедельник";
            case 2: return "Вторник";
            case 3: return "Среда";
            case 4: return "Четверг";
            case 5: return "Пятница";
            case 6: return "Суббота";
            default: return "Неизвестно";
        }
    }

    /**
     * @brief Format as string "DD.MM.YY HH:MM:SS"
     * @param buffer Output buffer (minimum 18 chars)
     */
    void formatDateTime(char *buffer) const {
        sprintf(buffer, "%02d.%02d.%02d %02d:%02d:%02d",
                day, month, year, hours, minutes, seconds);
    }

    /**
     * @brief Validate date and time values
     * @return true if all values are in valid ranges
     */
    bool isValid() const {
        return (seconds <= 59) &&
               (minutes <= 59) &&
               (hours <= 23) &&
               (dayOfWeek <= 6) &&
               (day >= 1 && day <= 31) &&
               (month >= 1 && month <= 12) &&
               (year <= 99); // Assuming 2000-2099 range
    }
};

/**
 * @brief ReadDateTime command implementation
 *
 * Command 0x1C - Read device date and time
 * Supported by all device generations with identical format
 */
class ReadDateTimeCommand : public TypedCommand<ReadDateTimeRequest, ReadDateTimeResponse> {
public:
    /**
     * @brief Constructor
     */
    ReadDateTimeCommand() : TypedCommand(CMD_READ_DATE_TIME) {
    }

    /**
     * @brief Prepare request data
     * @param requestData Buffer for request data
     * @param maxSize Maximum buffer size
     * @return Actual request data size (always 0)
     */
    size_t prepareRequest(uint8_t *requestData, size_t maxSize) override {
        // ReadDateTime не имеет входных данных
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
     * @param requestData Request data buffer (empty)
     * @param dataSize Request data size (0)
     * @param responseData Buffer for response data
     * @param maxResponseSize Maximum response buffer size
     * @return Response data size (7 bytes)
     */
    size_t handleRequest(const uint8_t *requestData, size_t dataSize,
                         uint8_t *responseData, size_t maxResponseSize) override {
        // Проверяем что запрос пустой
        if (dataSize != 0) {
            return 0;
        }

        // Проверяем что есть место для ответа
        if (maxResponseSize < 7) {
            return 0;
        }

        // Генерируем ответ с текущим временем (или используем установленное)
        return m_response.toBytes(responseData);
    }

    /**
     * @brief Get command name
     * @return Command name string
     */
    const char *getCommandName() const override {
        return "ReadDateTime";
    }

    /**
     * @brief Check if command is valid for device generation
     * @param boardId Board ID
     * @param role Role value
     * @return true (ReadDateTime supported by all generations)
     */
    bool isValidForGeneration(uint8_t boardId, uint8_t role) const override {
        // Поддерживается всеми поколениями!
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
     * @param minSize Minimum response size (7 bytes)
     * @param maxSize Maximum response size (7 bytes)
     */
    void getResponseSizeRange(size_t &minSize, size_t &maxSize) const override {
        minSize = 7;
        maxSize = 7;
    }

    /**
     * @brief Get parsed date and time
     * @return DateTime response structure
     */
    const ReadDateTimeResponse &getDateTime() const {
        return m_response;
    }

    /**
     * @brief Set server response data (for server mode)
     * @param dateTime DateTime to set
     */
    void setServerResponse(const ReadDateTimeResponse &dateTime) {
        m_response = dateTime;
    }

    /**
     * @brief Set server response from current system time
     */
    void setServerResponseFromSystemTime() {
        // Можно использовать millis() или RTC для получения времени
        // Здесь простой пример:
        m_response.seconds = 30;
        m_response.minutes = 45;
        m_response.hours = 14;
        m_response.dayOfWeek = 2; // Вторник
        m_response.day = 27;
        m_response.month = 5;
        m_response.year = 25; // 2025
    }

    /**
     * @brief Format date/time as string
     * @param buffer Output buffer (minimum 18 chars)
     */
    void formatDateTime(char *buffer) const {
        m_response.formatDateTime(buffer);
    }

    /**
     * @brief Get day of week name
     */
    const char *getDayOfWeekName() const {
        return m_response.getDayOfWeekName();
    }

    /**
     * @brief Validate response data
     */
    bool isDateTimeValid() const {
        return m_response.isValid();
    }
};

#endif // READ_DATE_TIME_COMMAND_H
