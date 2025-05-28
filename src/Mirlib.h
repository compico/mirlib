#ifndef ELECTRIC_METER_PROTOCOL_H
#define ELECTRIC_METER_PROTOCOL_H

#include <Arduino.h>
#include <map>
#include <functional>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "ProtocolTypes.h"
#include "ProtocolUtils.h"
#include "Commands/BaseCommand.h"
#include "Commands/PingCommand.h"
#include "Commands/ReadStatusCommand.h"
#include "Commands/ReadInstantValueCommand.h"
#include "Commands/ReadDateTimeCommand.h"
#include "Commands/GetInfoCommand.h"

/**
 * @brief Основной класс для связи по протоколу электросчетчиков
 *
 * Этот класс предоставляет функциональность как клиента, так и сервера для связи
 * с электросчетчиками с использованием радиомодуля CC1101 и пользовательского протокола.
 *
 * Использует оригинальные настройки CC1101 и методы отправки/приёма пакетов
 * из первоначального проекта для максимальной совместимости.
 */
class Mirlib {
public:
    /**
     * @brief Режим работы устройства
     */
    enum Mode {
        CLIENT, ///< Режим клиента - отправляет запросы к счетчикам
        SERVER ///< Режим сервера - имитирует электросчетчик
    };

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
     * @param mode Режим работы устройства (CLIENT или SERVER)
     * @param deviceAddress Адрес устройства (0x0001-0xFDE8 для счетчиков, 0xFFFF для клиента)
     */
    Mirlib(Mode mode = CLIENT, uint16_t deviceAddress = 0xFFFF);

    /**
     * @brief Инициализация протокола и CC1101 с оригинальными настройками
     * @param csPin Пин Chip Select для CC1101
     * @param gdo0Pin Пин GDO0 для CC1101 (по умолчанию: 2)
     * @param gdo2Pin Пин GDO2 для CC1101 (опционально, -1 для отключения)
     * @return true если инициализация прошла успешно
     */
    bool begin(int csPin, int gdo0Pin = 2, int gdo2Pin = -1);

    /**
     * @brief Установить пароль устройства (для режима сервера)
     * @param password 4-байтный пароль
     */
    void setPassword(uint32_t password);

    /**
     * @brief Установить статус устройства (для ответов в режиме сервера)
     * @param status 4-байтный статус
     */
    void setStatus(uint32_t status);

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
     * @brief Установить поколение сервера для ответов
     * @param generation Поколение сервера для имитации
     */
    void setServerGeneration(Generation generation) { m_serverGeneration = generation; }

    /**
     * @brief Отправить команду целевому устройству (режим CLIENT)
     * @param command Команда для отправки
     * @param targetAddress Адрес целевого устройства
     * @param responseData Буфер для данных ответа (опционально)
     * @param responseSize Размер буфера ответа
     * @return true если команда отправлена и ответ получен успешно
     */
    bool sendCommand(BaseCommand *command, uint16_t targetAddress,
                     uint8_t *responseData = nullptr, size_t responseSize = 0);

    /**
     * @brief Обработать входящие пакеты (режим SERVER)
     * @return true если пакет был обработан
     */
    bool processIncomingPackets();

    /**
     * @brief Зарегистрировать обработчик команд для режима сервера
     * @param commandCode Код команды (0x01, 0x05, 0x30, и т.д.)
     * @param handler Функция обработчика команды
     */
    void registerCommandHandler(uint8_t commandCode,
                                std::function<bool(const PacketData &, PacketData &)> handler);

    /**
     * @brief Автоопределение поколения устройства с помощью команды GetInfo
     * @param targetAddress Адрес целевого устройства
     * @return true если определение прошло успешно
     */
    bool autoDetectGeneration(uint16_t targetAddress);

    /**
     * @brief Включить/отключить отладочный вывод
     * @param enable Включить отладочный вывод
     */
    void setDebugMode(bool enable) { m_debugMode = enable; }

    /**
     * @brief Получить последнее сообщение об ошибке
     * @return Строка сообщения об ошибке
     */
    const char *getLastError() const { return m_lastError; }

    /**
     * @brief Вывести статус CC1101 (для отладки)
     */
    void printCC1101Status();

    /**
     * @brief Сбросить и переинициализировать CC1101
     */
    void resetCC1101();

private:
    Mode m_mode;
    uint16_t m_deviceAddress;
    uint32_t m_password;
    uint32_t m_status;
    uint32_t m_timeout;
    Generation m_generation;
    Generation m_serverGeneration;
    bool m_debugMode;
    int m_gdo0Pin; ///< Пин GDO0 для использования в функциях
    char m_lastError[128];

    // Обработчики команд для режима сервера
    std::map<uint8_t, std::function<bool(const PacketData &, PacketData &)> > m_commandHandlers;

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
     * @brief Обработать полученный пакет в режиме сервера
     * @param packet Полученный пакет
     * @return true если пакет был обработан
     */
    bool handleServerPacket(const PacketData &packet);

    /**
     * @brief Отправить пакет ответа в режиме сервера
     * @param originalPacket Исходный пакет запроса
     * @param responseData Данные ответа
     * @return true если ответ отправлен
     */
    bool sendResponse(const PacketData &originalPacket, const PacketData &responseData);

    /**
     * @brief Зарегистрировать обработчики команд по умолчанию
     */
    void registerDefaultHandlers();

    /**
     * @brief Установить последнее сообщение об ошибке
     * @param error Сообщение об ошибке
     */
    void setError(const char *error);

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

#endif // ELECTRIC_METER_PROTOCOL_H
