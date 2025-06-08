#include "MirlibBase.h"

#include "MirlibDebug.h"

MirlibBase::MirlibBase(uint16_t deviceAddress)
    : m_deviceAddress(deviceAddress)
      , m_password(0)
      , m_status(0)
      , m_timeout(5000)
      , m_generation(UNKNOWN)
      , m_gdo0Pin(2) // По умолчанию пин 2 для
      , m_lastError(ERR_NONE)
{
}

bool MirlibBase::begin(int csPin, int gdo0Pin, int gdo2Pin) {
    // Сохраняем пин GDO0 для использования в функциях
    if (gdo0Pin >= 0) {
        m_gdo0Pin = gdo0Pin;
    }

    ELECHOUSE_cc1101.setGDO0(m_gdo0Pin);

    // Проверка подключения CC1101
    if (!ELECHOUSE_cc1101.getCC1101()) {
        setError(ERR_SPI_CC1101_CON_ERROR);
        return false;
    }

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("SPI Connection CC1101 OK");
    #endif

    // Инициализация CC1101 с оригинальными настройками
    initializeCC1101();

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("CC1101 инициализирован с оригинальными настройками");
    #endif

    return true;
}

bool MirlibBase::initializeCC1101() {
    // Оригинальные настройки rfSettings из старого проекта
    const byte rfSettings[] = {
        0x0D, // IOCFG2              GDO2 Output Pin Configuration
        0x2E, // IOCFG1              GDO1 Output Pin Configuration
        0x06, // IOCFG0              GDO0 Output Pin Configuration
        0x4F, // FIFOTHR             RX FIFO and TX FIFO Thresholds
        0xD3, // SYNC1               Sync Word, High Byte
        0x91, // SYNC0               Sync Word, Low Byte
        0x3C, // PKTLEN              Packet Length
        0x00, // PKTCTRL1            Packet Automation Control
        0x41, // PKTCTRL0            Packet Automation Control
        0x00, // ADDR                Device Address
        0x16, // CHANNR              Channel Number
        0x0F, // FSCTRL1             Frequency Synthesizer Control
        0x00, // FSCTRL0             Frequency Synthesizer Control
        0x10, // FREQ2               Frequency Control Word, High Byte
        0x8B, // FREQ1               Frequency Control Word, Middle Byte
        0x54, // FREQ0               Frequency Control Word, Low Byte
        0xD9, // MDMCFG4             Modem Configuration
        0x83, // MDMCFG3             Modem Configuration
        0x13, // MDMCFG2             Modem Configuration
        0xD2, // MDMCFG1             Modem Configuration
        0xAA, // MDMCFG0             Modem Configuration
        0x31, // DEVIATN             Modem Deviation Setting
        0x07, // MCSM2               Main Radio Control State Machine Configuration
        0x0C, // MCSM1               Main Radio Control State Machine Configuration
        0x08, // MCSM0               Main Radio Control State Machine Configuration
        0x16, // FOCCFG              Frequency Offset Compensation Configuration
        0x6C, // BSCFG               Bit Synchronization Configuration
        0x03, // AGCCTRL2            AGC Control
        0x40, // AGCCTRL1            AGC Control
        0x91, // AGCCTRL0            AGC Control
        0x87, // WOREVT1             High Byte Event0 Timeout
        0x6B, // WOREVT0             Low Byte Event0 Timeout
        0xF8, // WORCTRL             Wake On Radio Control
        0x56, // FREND1              Front End RX Configuration
        0x10, // FREND0              Front End TX Configuration
        0xE9, // FSCAL3              Frequency Synthesizer Calibration
        0x2A, // FSCAL2              Frequency Synthesizer Calibration
        0x00, // FSCAL1              Frequency Synthesizer Calibration
        0x1F, // FSCAL0              Frequency Synthesizer Calibration
        0x41, // RCCTRL1             RC Oscillator Configuration
        0x00, // RCCTRL0             RC Oscillator Configuration
        0x59, // FSTEST              Frequency Synthesizer Calibration Control
        0x59, // PTEST               Production Test
        0x3F, // AGCTEST             AGC Test
        0x81, // TEST2               Various Test Settings
        0x35, // TEST1               Various Test Settings
        0x09, // TEST0               Various Test Settings
    };

    // Сброс CC1101
    ELECHOUSE_cc1101.SpiStrobe(0x30); // SRES - Reset chip
    delay(1);

    // Запись настроек в регистры CC1101
    ELECHOUSE_cc1101.SpiWriteBurstReg(0x00, const_cast<byte *>(rfSettings), 0x2F);

    // Калибровка частотного синтезатора
    ELECHOUSE_cc1101.SpiStrobe(0x33); // SCAL - Calibrate frequency synthesizer and turn it off
    // Очистка FIFO буферов
    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer

    // Переход в режим приема
    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("CC1101 настроен с оригинальными параметрами");
        MIRLIB_DEBUG_PRINT("Режим: RX");
    #endif

    return true;
}

bool MirlibBase::sendPacketOriginalStyle(const PacketData &packet) {
    if (!packet.isValid()) {
        #ifdef MIRLIB_DEBUG
            MIRLIB_DEBUG_PRINT("Невалидный пакет для отправки");
        #endif
        return false;
    }


    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Калибровка частотного синтезатора");
    #endif

    // Калибровка частотного синтезатора
    ELECHOUSE_cc1101.SpiStrobe(0x33); // SCAL - Calibrate frequency synthesizer and turn it off
    delay(1);


    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Очистка TX FIFO и выход из RX/TX режима");
    #endif

    // Очистка TX FIFO и выход из RX/TX режима
    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Установка мощности передачи 10dB");
    #endif

    // Установка мощности передачи 10dB
    ELECHOUSE_cc1101.SpiWriteReg(0x3E, 0xC4); // PATABLE - выставляем мощность 10dB


    #ifdef MIRLIB_DEBUG
        debugPrintPacket(packet, "Отправка запроса");
        char msg[50];
        snprintf(msg, sizeof(msg), "Размер пакета: %d байт", packet.rawSize);
        MIRLIB_DEBUG_PRINT(msg);
    #endif

    // Отправка пакета
    ELECHOUSE_cc1101.SendData(const_cast<uint8_t *>(packet.rawPacket), packet.rawSize);

    // Очистка RX FIFO и переход в режим приема
    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

    return true;
}

bool MirlibBase::receivePacketOriginalStyle(PacketData &packet, uint32_t timeout) {
    packet.clear();

    uint32_t startTime = millis();
    timeout = (timeout == 0) ? m_timeout : timeout;

    #ifdef MIRLIB_DEBUG
        char msg[100];
        snprintf(msg, sizeof(msg), "Ожидание пакета (таймаут: %lu мс)", timeout);
        MIRLIB_DEBUG_PRINT(msg);
    #endif

    while (millis() - startTime < timeout) {
        if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
            uint8_t buffer[ProtocolConstants::MAX_PACKET_SIZE];

            const int len = ELECHOUSE_cc1101.ReceiveData(buffer);

            if (len > 0 && static_cast<size_t>(len) <= ProtocolConstants::MAX_PACKET_SIZE) {
                #ifdef MIRLIB_DEBUG
                    char msg[50];
                    snprintf(msg, sizeof(msg), "Получен пакет, размер: %d байт", len);
                    MIRLIB_DEBUG_PRINT(msg);
                    ProtocolUtils::printHex(buffer, len, "Сырые данные");
                #endif

                // Разбор пакета
                if (ProtocolUtils::unpackPacket(buffer, len, packet)) {
                    #ifdef MIRLIB_DEBUG
                        MIRLIB_DEBUG_PRINT("Пакет успешно разобран");
                    #endif

                    // Очистка RX FIFO и перезапуск приема
                    ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer
                    ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
                    ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
                    ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX

                    return true;
                }
                #ifdef MIRLIB_DEBUG
                    MIRLIB_DEBUG_PRINT("Ошибка разбора пакета");
                #endif
            } else {
                #ifdef MIRLIB_DEBUG
                    char msg[50];
                    snprintf(msg, sizeof(msg), "Неверный размер пакета: %d", len);
                    MIRLIB_DEBUG_PRINT(msg);
                #endif
            }

            // Очистка RX FIFO и перезапуск приема при ошибке
            ELECHOUSE_cc1101.SpiStrobe(0x36); // SIDLE - Exit RX / TX, turn off frequency synthesizer
            ELECHOUSE_cc1101.SpiStrobe(0x3A); // SFRX - Flush the RX FIFO buffer
            ELECHOUSE_cc1101.SpiStrobe(0x3B); // SFTX - Flush the TX FIFO buffer
            ELECHOUSE_cc1101.SpiStrobe(0x34); // SRX - Enable RX
        }

        delay(1); // Небольшая задержка для стабильности
    }

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Таймаут приема пакета");
    #endif

    return false;
}

void MirlibBase::setError(const ErrorCode code) {
    m_lastError = code;
    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT_ERROR(code);
    #endif
}

void MirlibBase::debugPrint(const char *message) {
    #ifdef MIRLIB_DEBUG
        Serial.print("[Mirlib] ");
        Serial.println(message);
    #endif
}

void MirlibBase::debugPrintPacket(const PacketData &packet, const char *title) {
    ProtocolUtils::printPacketHex(packet, title);
}

void MirlibBase::printCC1101Status() {
    #ifndef MIRLIB_DEBUG
        return;
    #endif

    MIRLIB_DEBUG_PRINT("=== Статус CC1101 ===");

    // Чтение регистра статуса
    uint8_t status = ELECHOUSE_cc1101.SpiReadStatus(0xF5); // MARCSTATE
    char msg[50];
    snprintf(msg, sizeof(msg), "MARCSTATE: 0x%02X", status);
    debugPrint(msg);

    // Проверка FIFO
    uint8_t rxBytes = ELECHOUSE_cc1101.SpiReadStatus(0xFB); // RXBYTES
    uint8_t txBytes = ELECHOUSE_cc1101.SpiReadStatus(0xFA); // TXBYTES
    snprintf(msg, sizeof(msg), "RX FIFO: %d байт, TX FIFO: %d байт", rxBytes & 0x7F, txBytes & 0x7F);
    debugPrint(msg);

    debugPrint("==================");
}

void MirlibBase::resetCC1101() {
    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Выполняется сброс CC1101...");
    #endif

    initializeCC1101();

    #ifdef MIRLIB_DEBUG
        MIRLIB_DEBUG_PRINT("Сброс CC1101 завершен");
        printCC1101Status();
    #endif
}
