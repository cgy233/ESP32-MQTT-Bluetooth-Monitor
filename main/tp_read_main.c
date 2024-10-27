/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "esp_log.h"
#include "tp_read_custom.h"
#include "esp_mqtt_custom.h"

#define OPEN_THRESHOLD 800  // 开门阈值
#define CLOSE_THRESHOLD 100 // 关门阈值
#define FILTER_WINDOW 5    // 移动平均窗口大小

#define TOUCH_PAD_NO_CHANGE (-1)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)
/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
int touch_filter_values[FILTER_WINDOW] = {0}; // 用于存储最近的过滤值
int index = 0;                                // 用于移动平均窗口的索引
int sum_filter_value = 0;                     // 过滤值的和
int average_filter_value = 0;                 // 平均值

typedef enum
{
    DOOR_CLOSED, // 关门状态
    DOOR_OPEN,   // 开门状态
    DOOR_EVENT   // 开关门事件
} door_state_t;

door_state_t door_state = DOOR_CLOSED; // 初始状态为关门
bool open_event_detected = false;      // 用于一次性开门事件的标志

void update_filter_value(int new_value)
{
    // 更新移动平均值
    sum_filter_value -= touch_filter_values[index];          // 减去旧的值
    touch_filter_values[index] = new_value;                  // 更新新的值
    sum_filter_value += new_value;                           // 加上新的值
    index = (index + 1) % FILTER_WINDOW;                     // 更新索引
    average_filter_value = sum_filter_value / FILTER_WINDOW; // 计算平均值
}

void check_door_event()
{
    // 根据移动平均值判断事件
    switch (door_state)
    {
    case DOOR_CLOSED:
        if (average_filter_value > OPEN_THRESHOLD)
        {
            printf("开门事件检测到!\n");
            door_state = DOOR_OPEN;
            open_event_detected = true; // 标记一次性开门事件
            mqtt_send_door_status(door_state);
        }
        break;
    case DOOR_OPEN:
        if (average_filter_value < CLOSE_THRESHOLD)
        {
            printf("关门事件检测到!\n");
            door_state = DOOR_CLOSED;
            mqtt_send_door_status(door_state);
        }
        break;
    default:
        break;
    }

    // 如果一次开门事件发生了，从开到关的过程也算作一次完整开门
    if (open_event_detected && door_state == DOOR_CLOSED)
    {
        printf("一次完整的开门事件检测到!\n");
        open_event_detected = false; // 重置标志
        mqtt_send_door_status(DOOR_EVENT);
    }
}

void tp_read_task()
{
    int touch_value;
    int touch_filter_value;

    while (1)
    {
        touch_pad_read_raw_data(3, &touch_value);        // 读取原始值
        touch_pad_read_filtered(3, &touch_filter_value); // 读取过滤后的值
        update_filter_value(touch_filter_value);         // 更新移动平均值

        // printf("T%d: [origin: %d, filter: %d, average: %d]\r\n",
        //        3, touch_value, touch_filter_value, average_filter_value);

        check_door_event();                   // 检查开门/关门事件
        vTaskDelay(200 / portTICK_PERIOD_MS); // 延时200毫秒
    }
}

static void tp_example_touch_pad_init(void)
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}

void tp_read_start(void)
{
    // Initialize touch pad peripheral.
    // The default fsm mode is software trigger mode.
    ESP_ERROR_CHECK(touch_pad_init());
    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    // The larger the range, the larger the pulse count value.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    tp_example_touch_pad_init();
#if TOUCH_FILTER_MODE_EN
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
#endif
    // Start task to read values sensed by pads
    xTaskCreate(&tp_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
}
