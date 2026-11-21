#include <Arduino.h>
#include "config/config.h"
#include "console/serial.h"
#include "console/command.h"
#include "drivers/led.h"

void setup()
{
  init_console(CONSOLE_BAUD);

  set_led(100, 100, 0);
  delay(500);
  set_led(0, 100, 100);
  delay(500);
  set_led(30, 30, 0);
  delay(500);
  set_led(0, 0, 0);
}

void loop()
{
  poll_console();
  handle_perform_task();
}