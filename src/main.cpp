/*  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

// www.makerguides.com/max7219-led-dot-matrix-display-arduino-tutorial/

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>

// Define hardware type, size, and output pins:
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN SS
#define LED_PIN GPIO_NUM_8

static const char *TAG = "main";

TaskHandle_t flashLED_TaskHandler = nullptr;

// Setup for software SPI:
// #define DATA_PIN 2
// #define CLK_PIN 4
// MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN,
// MAX_DEVICES);

// Create a new instance of the MD_Parola class with hardware SPI connection:
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

/*
 * We warn if a secondary serial console is enabled. A secondary serial console is always output-only and
 * hence not very useful for interactive console applications. If you encounter this warning, consider disabling
 * the secondary serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

#ifdef CONFIG_ESP_CONSOLE_USB_CDC
#error This example is incompatible with USB CDC console. Please try "console_usb" example instead.
#endif // CONFIG_ESP_CONSOLE_USB_CDC

#ifdef CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error This example is incompatible with USB serial JTAG console.
#endif // CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG

#define PROMPT_STR CONFIG_IDF_TARGET

/**
 * @brief Flash LED continuously
 */
void flashLED(void *) {
  while (1) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

extern "C" {
void app_main(void);
}

static int do_hello_cmd(int argc, char **argv)
{
    printf("Hello World\n");
    return 0;
}

TEST_CASE("esp console init/deinit test", "[console]")
{
    esp_console_config_t console_config = ESP_CONSOLE_CONFIG_DEFAULT();
    TEST_ESP_OK(esp_console_init(&console_config));
    const esp_console_cmd_t cmd = {
        .command = "hello",
        .help = "Print Hello World",
        .hint = NULL,
        .func = do_hello_cmd,
    };
    TEST_ESP_OK(esp_console_cmd_register(&cmd));
    // re-register the same command, just for test
    TEST_ESP_OK(esp_console_cmd_register(&cmd));
    TEST_ESP_OK(esp_console_deinit());
}

static esp_console_repl_t *s_repl = NULL;

/* handle 'quit' command */
static int do_cmd_quit(int argc, char **argv)
{
    printf("ByeBye\r\n");
    s_repl->del(s_repl);
    return 0;
}

// Enter "quit" to exit REPL environment
TEST_CASE("esp console repl test", "[console][ignore]")
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    TEST_ESP_OK(esp_console_new_repl_uart(&uart_config, &repl_config, &s_repl));

    esp_console_cmd_t cmd = {
        .command = "quit",
        .help = "Quit REPL environment",
        .func = &do_cmd_quit
    };
    TEST_ESP_OK(esp_console_cmd_register(&cmd));

    TEST_ESP_OK(esp_console_start_repl(s_repl));
    vTaskDelay(pdMS_TO_TICKS(2000));
}