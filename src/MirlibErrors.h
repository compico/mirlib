//
// Created by morekhov on 01.06.2025.
//

#ifndef MIRLIBERRORS_H
#define MIRLIBERRORS_H

enum ErrorCode {
    ERR_NONE = 0,
    // SPI Connection CC1101 Error
    ERR_SPI_CC1101_CON_ERROR = 1,
    // Команда равна null
    ERR_COMMAND_IS_NULL = 2,
    // Не удалось создать пакет запроса
    ERR_FAIL_CREATE_PACKAGE = 3,
    // Не удалось отправить пакет
    ERR_FAIL_SEND_PACKAGE = 4,
    // Ответ не получен
    ERR_FAIL_RECEIVE_PACKAGE = 5,
    // Код команды ответа не совпадает
    ERR_RESPONSE_COMMANDS_DO_NOT_MATCH = 6,
    // Адрес источника ответа не совпадает
    ERR_RESPONSE_ADDRESS_DO_NOT_MATCH = 7,
    // Адрес назначения ответа не совпадает
    ERR_RESPONSE_TARGET_DO_NOT_MATCH = 8,
    // Полученный пакет не является ответом
    ERR_RESPONSE_IS_NOT_RESPONSE = 9,
    // Не удалось разобрать данные ответа
    ERR_UNABLE_TO_PARSE_RESPONSE_DATA = 10,
    // Полученный пакет не является запросом
    ERR_PACKAGE_IS_NO_REQUEST = 11,
    // Нет обработчика для команды
    ERR_NO_HAVE_HANDLER_FOR_THIS_COMMAND = 12,
    // Обработчик команды не сработал
    ERR_COMMAND_HANDLER_FAILED = 13,
    // Не удалось отправить ответ
    ERR_FAILED_TO_SEND_RESPONSE = 14,
};

#endif //MIRLIBERRORS_H
