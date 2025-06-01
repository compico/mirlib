#ifndef MIRLIB_CLIENT_H
#define MIRLIB_CLIENT_H

#include "MirlibBase.h"
#include "Commands/BaseCommand.h"
#include "Commands/PingCommand.h"
#include "Commands/ReadStatusCommand.h"
#include "Commands/ReadInstantValueCommand.h"
#include "Commands/ReadDateTimeCommand.h"
#include "Commands/GetInfoCommand.h"

/**
 * @brief Клиентская часть Mirlib для отправки команд счетчикам
 *
 * Этот класс предоставляет функциональность клиента для связи
 * с электросчетчиками с использованием радиомодуля CC1101.
 */
class MirlibClient : public MirlibBase {
public:
    /**
     * @brief Конструктор
     * @param deviceAddress Адрес клиента (по умолчанию 0xFFFF)
     */
    explicit MirlibClient(uint16_t deviceAddress = 0xFFFF);

    /**
     * @brief Деструктор
     */
    virtual ~MirlibClient() = default;

    /**
     * @brief Отправить команду целевому устройству
     * @param command Команда для отправки
     * @param targetAddress Адрес целевого устройства
     * @param responseData Буфер для данных ответа (опционально)
     * @param responseSize Размер буфера ответа
     * @return true если команда отправлена и ответ получен успешно
     */
    bool sendCommand(BaseCommand *command, uint16_t targetAddress,
                     uint8_t *responseData = nullptr, size_t responseSize = 0);

    /**
     * @brief Автоопределение поколения устройства с помощью команды GetInfo
     * @param targetAddress Адрес целевого устройства
     * @return true если определение прошло успешно
     */
    bool autoDetectGeneration(uint16_t targetAddress);

    /**
     * @brief Ping устройства
     * @param targetAddress Адрес целевого устройства
     * @param firmwareVersion Полученная версия прошивки (выход)
     * @param deviceAddress Полученный адрес устройства (выход)
     * @return true если ping успешен
     */
    bool ping(uint16_t targetAddress, uint16_t *firmwareVersion = nullptr, uint16_t *deviceAddress = nullptr);

    /**
     * @brief Получить информацию об устройстве
     * @param targetAddress Адрес целевого устройства
     * @param response Структура для ответа
     * @return true если команда выполнена успешно
     */
    bool getInfo(uint16_t targetAddress, GetInfoResponseBase *response = nullptr);

    /**
     * @brief Прочитать дату и время устройства
     * @param targetAddress Адрес целевого устройства
     * @param response Структура для ответа
     * @return true если команда выполнена успешно
     */
    bool readDateTime(uint16_t targetAddress, ReadDateTimeResponse *response = nullptr);

    /**
     * @brief Прочитать статус счетчика
     * @param targetAddress Адрес целевого устройства
     * @param energyType Тип энергии (для новых поколений)
     * @param oldResponse Ответ старого поколения (выход)
     * @param newResponse Ответ нового поколения (выход)
     * @return true если команда выполнена успешно
     */
    bool readStatus(uint16_t targetAddress, EnergyType energyType = ACTIVE_FORWARD,
                    ReadStatusResponseOld *oldResponse = nullptr, 
                    ReadStatusResponseNew *newResponse = nullptr);

    /**
     * @brief Прочитать мгновенные значения (для переходного/нового поколения)
     * @param targetAddress Адрес целевого устройства
     * @param group Группа параметров
     * @param transResponse Ответ переходного поколения (выход)
     * @param newResponse Ответ нового поколения (выход)
     * @return true если команда выполнена успешно
     */
    bool readInstantValue(uint16_t targetAddress, ParameterGroup group = GROUP_BASIC,
                          ReadInstantValueResponseTransition *transResponse = nullptr,
                          ReadInstantValueResponseNewBasic *newResponse = nullptr);

    /**
     * @brief Установить поколение для команд (если известно заранее)
     * @param generation Поколение устройства
     */
    void setDeviceGeneration(Generation generation) { m_generation = generation; }

private:
    /**
     * @brief Определить поколение для команды на основе известного поколения или автоопределения
     * @param boardId Board ID (если известен)
     * @param role Role (если известен)
     * @return Информация о поколении
     */
    GenerationInfo getGenerationInfo(uint8_t boardId = 0, uint8_t role = 0x32);
};

#endif // MIRLIB_CLIENT_H