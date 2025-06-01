#ifndef MIRLIB_BASE_H
#define MIRLIB_BASE_H

#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include "MirlibErrors.h"
#include "ProtocolTypes.h"
#include "ProtocolUtils.h"

/**
 * @brief Базовый класс для Mirlib с общей функциональностью
 */
class MirlibBase {
public:
    /**
     * @brief Тип поколения устройства (автоопределяется из GetInfo)
     */
    enum Generation {
        UNKNOWN = 0,
        OLD_GENERATION, ///< Старое поколение (Role < 0x32, ID: 0x01,0x02,0x03,0x04,0x0C,0x0D,0x11,0x12)
        TRANSITION_GENERATION, ///< Переходное поколение (Role >= 0x32, ID: 0x07,0x08,0x0A,0x0B)
        NEW_GENERATION ///< Новое поколение (Role >= 0x32, ID: 0x09,0x0E,0x0F,0x10,0x20,0x21,0x22)
    };

    /**
     * @brief Конструктор
     * @param deviceAddress Адрес устройства
     */
    explicit MirlibBase(uint16_t deviceAddress = 0xFFFF);

    /**
     * @brief Деструктор
     */
    virtual ~MirlibBase() = default;

    /**
     * @brief Инициализация протокола и CC1101 с оригинальными настройками
     * @param csPin Пин Chip Select для CC1101
     * @param gdo0Pin Пин GDO0 для CC1101 (по умолчанию: 2)
     * @param gdo2Pin Пин GDO2 для CC1101 (опционально, -1 для отключения)
     * @return true если инициализация прошла успешно
     */
    bool begin(int csPin, int gdo0Pin = 2, int gdo2Pin = -1);

    /**
     * @brief Установить пароль устройства
     * @param password 4-байтный пароль
     */
    void setPassword(uint32_t password) { m_password = password; }

    /**
     * @brief Установить статус устройства
     * @param status 4-байтный статус
     */
    void setStatus(uint32_t status) { m_status = status; }

    /**
     * @brief Получить определенное поколение устройства
     * @return Тип поколения
     */
    Generation getGeneration() const { return m_generation; }

    /**
     * @brief Получить адрес устройства
     * @return Адрес устройства
     */
    uint16_t getDeviceAddress() const { return m_deviceAddress; }

    /**
     * @brief Установить таймаут приема в миллисекундах
     * @param timeout Таймаут в мс (по умолчанию: 5000)
     */
    void setTimeout(uint32_t timeout) { m_timeout = timeout; }

    /**
     * @brief Получить последнее сообщение об ошибке
     * @return Строка сообщения об ошибке
     */
    const ErrorCode getLastError() const { return m_lastError; }

    /**
     * @brief Вывести статус CC1101 (для отладки)
     */
    void printCC1101Status();

    /**
     * @brief Сбросить и переинициализировать CC1101
     */
    void resetCC1101();

protected:
    uint16_t m_deviceAddress;
    uint32_t m_password;
    uint32_t m_status;
    uint32_t m_timeout;
    Generation m_generation;
    int m_gdo0Pin; ///< Пин GDO0 для использования в функциях
    ErrorCode m_lastError;

    /**
     * @brief Инициализация CC1101 с оригинальными настройками rfSettings
     * @return true если инициализация успешна
     */
    bool initializeCC1101();

    /**
     * @brief Отправить пакет в стиле оригинального кода
     * Использует последовательность команд CC1101 как в исходном проекте
     * @param packet Пакет для отправки
     * @return true если отправлен успешно
     */
    bool sendPacketOriginalStyle(const PacketData &packet);

    /**
     * @brief Получить пакет в стиле оригинального кода
     * Использует CheckReceiveFlag и обработку как в исходном проекте
     * @param packet Буфер для полученного пакета
     * @param timeout Таймаут в мс
     * @return true если пакет получен
     */
    bool receivePacketOriginalStyle(PacketData &packet, uint32_t timeout = 0);

    /**
     * @brief Установить последнее сообщение об ошибке
     * @param error Сообщение об ошибке
     */
    void setError(const ErrorCode error);

    /**
     * @brief Функция отладочного вывода
     * @param message Сообщение для вывода
     */
    void debugPrint(const char *message);

    /**
     * @brief Отладочный вывод пакета
     * @param packet Пакет для вывода
     * @param title Заголовок для отладочного вывода
     */
    void debugPrintPacket(const PacketData &packet, const char *title);
};

#endif // MIRLIB_BASE_H