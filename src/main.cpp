/*  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

// www.makerguides.com/max7219-led-dot-matrix-display-arduino-tutorial/

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_console.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define hardware type, size, and output pins:
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN SS
#define LED_PIN GPIO_NUM_8

static const char *TAG = "main";
#define PROMPT_STR CONFIG_IDF_TARGET

TaskHandle_t flashLED_TaskHandler = nullptr;

#ifndef CONFIG_ESP_CONSOLE_UART_DEFAULT
#error "CONFIG_ESP_CONSOLE_UART_DEFAULT not defined in sdkconfig"
#endif

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

// Setup for software SPI:
// #define DATA_PIN 2
// #define CLK_PIN 4
// MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Create a new instance of the MD_Parola class with hardware SPI connection:
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

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

extern "C"
{
    void app_main(void);
}

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    delay(3000);
    ESP_LOGI(TAG, "Running");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is a %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "\n");

    ESP_LOGI(TAG, "silicon revision %d", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    printf("MOSI is %d\n", MOSI);
    printf("MISO is %d\n", MISO);
    printf("SS is %d\n", SS);
    printf("SCK is %d\n", SCK);

    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Initialize the object:
    myDisplay.begin();
    // Set the intensity (brightness) of the display (0-15):
    myDisplay.setIntensity(0);
    // Clear the display:
    myDisplay.displayClear();
    myDisplay.displayText("Dada is The G.O.A.T", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

    if (xTaskCreate(flashLED, "flashLED", 1024 * 2, NULL, 10, &flashLED_TaskHandler) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
    }


    while (1) {
        if (myDisplay.displayAnimate()) {
            myDisplay.displayReset();
        }

        esp_console_repl_t *repl = NULL;
        esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
        /* Prompt to be printed before each line.
        * This can be customized, made dynamic, etc.
        */
        repl_config.prompt = PROMPT_STR ">";
        repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

        initialize_nvs();

        // while (Serial.available() > 0) {
        //     serial_input = Serial.readStringUntil('\n');
        //     myDisplay.displayClear();
        //     myDisplay.displayText(serial_input.c_str(), PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        // }

        ESP_ERROR_CHECK(esp_console_start_repl(repl));

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
