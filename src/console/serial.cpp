#include "console/serial.h"
#include "console/command.h"
#include "config/config.h"

#include <Arduino.h>
#include <cstddef>


namespace
{
    struct ConsolePort
    {
        Stream *stream;
        char rxBuf[CONSOLE_RX_BUF_SIZE];
        size_t idx;
    };

#if ARDUINO_USB_CDC_ON_BOOT
    ConsolePort uartPort = {&Serial0, {0}, 0};
    ConsolePort usbPort = {&Serial, {0}, 0};
#else
    ConsolePort uartPort = {&Serial, {0}, 0};
#if SOC_USB_SERIAL_JTAG_SUPPORTED
    HWCDC usbSerial;
    ConsolePort usbPort = {&usbSerial, {0}, 0};
#endif
#endif

    void poll_console_port(ConsolePort &port)
    {
        while (port.stream->available() > 0)
        {
            char c = static_cast<char>(port.stream->read());
            if (c == '\r' || c == '\n')
            {
                port.rxBuf[port.idx] = '\0';
                if (port.idx > 0)
                {
                    handleCommand(port.rxBuf, *port.stream);
                    port.idx = 0;
                }
            }
            else if (port.idx < sizeof(port.rxBuf) - 1)
            {
                port.rxBuf[port.idx++] = c;
            }
        }
    }
}

void init_console(unsigned long baud)
{
#if ARDUINO_USB_CDC_ON_BOOT
    Serial0.begin(baud);
    Serial.begin(baud);
#else
    Serial.begin(baud);
#if SOC_USB_SERIAL_JTAG_SUPPORTED
    usbSerial.begin(baud);
#endif
#endif
    context.selected_access_points = -1;
}

void poll_console()
{
    poll_console_port(uartPort);
#if ARDUINO_USB_CDC_ON_BOOT || SOC_USB_SERIAL_JTAG_SUPPORTED
    poll_console_port(usbPort);
#endif
}