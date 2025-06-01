#ifndef MIRLIB_SERVER_H
#define MIRLIB_SERVER_H

#include "MirlibBase.h"
#include "Commands/BaseCommand.h"
#include "Commands/PingCommand.h"
#include "Commands/ReadStatusCommand.h"
#include "Commands/ReadInstantValueCommand.h"
#include "Commands/ReadDateTimeCommand.h"
#include "Commands/GetInfoCommand.h"

/**
 * @brief Простая структура для хранения обработчиков команд без STL
 */
struct CommandHandler {
    uint8_t commandCode;
    bool (*handlerFunc)(const PacketData &request, PacketData &response, void *context);
    void *context;
    CommandHandler *next;

    CommandHandler() : commandCode(0), handlerFunc(nullptr), context(nullptr), next(nullptr) {}
};

/**
 * @brief Серверная часть Mirlib для имитации электросчетчика
 *
 * Этот класс предоставляет функциональность сервера для имитации
 * электросчетчика и ответов на команды клиентов.
 */
class MirlibServer : public MirlibBase {
public:
    /**
     * @brief Конструктор
     * @param deviceAddress Адрес счетчика (0x0001-0xFDE8)
     * @param serverGeneration Поколение для имитации (по умолчанию NEW_GENERATION)
     */
    explicit MirlibServer(uint16_t deviceAddress = 0x0001, Generation serverGeneration = NEW_GENERATION);

    /**
     * @brief Деструктор
     */
    virtual ~MirlibServer();

    /**
     * @brief Обработать входящие пакеты
     * @return true если пакет был обработан
     */
    bool processIncomingPackets();

    /**
     * @brief Зарегистрировать обработчик команд
     * @param commandCode Код команды (0x01, 0x05, 0x30, и т.д.)
     * @param handlerFunc Функция обработчика команды
     * @param context Контекст для передачи в обработчик
     */
    void registerCommandHandler(uint8_t commandCode,
                                bool (*handlerFunc)(const PacketData &, PacketData &, void *),
                                void *context = nullptr);

    /**
     * @brief Установить поколение сервера для ответов
     * @param generation Поколение сервера для имитации
     */
    void setServerGeneration(Generation generation) { m_serverGeneration = generation; }

    /**
     * @brief Получить поколение сервера
     * @return Поколение сервера
     */
    Generation getServerGeneration() const { return m_serverGeneration; }

private:
    Generation m_serverGeneration;

    // Связный список обработчиков команд вместо std::map
    CommandHandler *m_commandHandlers;

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
     * @brief Найти обработчик команды
     * @param commandCode Код команды
     * @return Указатель на обработчик или nullptr
     */
    CommandHandler *findCommandHandler(uint8_t commandCode);

    /**
     * @brief Освободить память обработчиков
     */
    void clearCommandHandlers();

    /**
     * @brief Зарегистрировать обработчики команд по умолчанию
     */
    void registerDefaultHandlers();

    // Статические обработчики для команд по умолчанию
    static bool handlePingCommand(const PacketData &request, PacketData &response, void *context);
    static bool handleGetInfoCommand(const PacketData &request, PacketData &response, void *context);
    static bool handleReadDateTimeCommand(const PacketData &request, PacketData &response, void *context);
    static bool handleReadStatusCommand(const PacketData &request, PacketData &response, void *context);
    static bool handleReadInstantValueCommand(const PacketData &request, PacketData &response, void *context);
};

#endif // MIRLIB_SERVER_H