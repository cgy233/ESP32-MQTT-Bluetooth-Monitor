#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "radar_sensor_driver.h"
#include "esp_mqtt_custom.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output (ESP32C2/ESP32H2 uses GPIO8 as the second output pin)
 * GPIO19: output (ESP32C2/ESP32H2 uses GPIO9 as the second output pin)
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Note. These are the default GPIO pins to be used in the example. You can
 * change IO pins in menuconfig.
 *
 * Test:
 * Connect GPIO18(8) with GPIO4
 * Connect GPIO19(9) with GPIO5
 * Generate pulses on GPIO18(8)/19(9), that triggers interrupt on GPIO4/5
 *
 */

/*
 * Let's say, LED_GPIO_PIN_NUM=18, GPIO_OUTPUT_IO_1=19
 * In binary representation,
 * 1ULL<<LED_GPIO_PIN_NUM is equal to 0000000000000000000001000000000000000000 and
 * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
 * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
 * */
#define GPIO_INPUT_IO_0     GPIO_NUM_15
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0))
/*
 * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
 * In binary representation,
 * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
 * 1ULL<<GPIO_INPUT_IO_1 is equal to 0000000000000000000000000000000000100000
 * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
 * */
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void radar_sensor_monitor_task(void* arg)
{
    static uint8_t level = 0xFF;
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            uint8_t temp_level = gpio_get_level(io_num);

            if (level == temp_level)
            {
                continue;
            }

            mqtt_send_radar_status(!temp_level);

            level = temp_level;

#if 0
            if (level)
            {
                printf("Radar sensor detected people in\n");
            }
            else
            {
                printf("Radar sensor detected people out\n");
            }
#endif
        }
    }
}

void radar_sensor_init(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(radar_sensor_monitor_task, "radar_sensor_monitor_task", 4096, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


#if 0
    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
#endif

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
}
