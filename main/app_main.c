#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_hid_host_main.h"
#include "esp_mqtt_custom.h"
#include "tp_read_custom.h"
#include "ble_tracker.h"
#include "esp_log.h"
#include "door_sensor_driver.h"

QueueHandle_t ble_device_queue;
ble_scan_result_t ble_tracker_data;

void confidence_task(void *param)
{

    while (1)
    {
        if (xQueueReceive(ble_device_queue, &ble_tracker_data, portMAX_DELAY))
        {

            if (ble_tracker_data.scan_state == 1)
            {
                ble_tracker_data.scan_state = 0;
                // ESP_LOGI("CONFIDENCE", "Scan Done");
                for (int i = 0; i < DEVICE_NUMBER; i++)
                {
                    ble_device_t *device = &ble_tracker_data.devices[i];
                    if (device->rssi != 0)
                    {
                        // ESP_LOGI("CONFIDENCE", "Device %s RSSI: %d, Confidence: %d", device->name, device->rssi, device->confidence);
                        if (device->confidence < 100)
                        {
                            device->confidence += 100 / NUMBER_OF_ARRIVALS;
                            if (device->confidence >= 100)
                            {
                                device->confidence = 100;
                                // ESP_LOGI("CONFIDENCE", "Device %s arrived", device->name);
                                mqtt_send_dev_info(device->name, device->confidence);
                            }
                        }
                        device->rssi = 0;
                    }
                    else
                    {
                        if (device->confidence > 0)
                        {
                            // ESP_LOGI("CONFIDENCE", "Device %s RSSI: %d, Confidence: %d", device->name, device->rssi, device->confidence);
                            device->confidence -= 100 / NUMBER_OF_LEAVE;
                            if (device->confidence <= 0)
                            {
                                device->confidence = 0;
                                // ESP_LOGI("CONFIDENCE", "Device %s leave", device->name);
                                mqtt_send_dev_info(device->name, device->confidence);
                            }
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < DEVICE_NUMBER; i++)
                {
                    ble_device_t *device = &ble_tracker_data.devices[i];
                    if (device->rssi != 0)
                    {
                        ESP_LOGI("CONFIDENCE", "Device %s RSSI: %d", device->name, device->rssi);
                    }
                }
            }
        }
    }
}

void app_main(void)
{
    for (int i = 0; i < 6; i++)
    {
        ble_tracker_data.devices[0].mac[i] = 0;
    }

    ble_tracker_data.devices[0].mac[0] = 0xc7;
    ble_tracker_data.devices[0].mac[1] = 0x6a;
    ble_tracker_data.devices[0].mac[2] = 0xcd;
    ble_tracker_data.devices[0].mac[3] = 0x04;
    ble_tracker_data.devices[0].mac[4] = 0x1a;
    ble_tracker_data.devices[0].mac[5] = 0x80;

    ble_device_queue = xQueueCreate(10, sizeof(ble_scan_result_t));

    door_sensor_init();

    mqtt_custom_init();

    // esp_ble_scan_init();
    // xTaskCreate(&confidence_task, "confidence_task", 6 * 1024, NULL, 2, NULL);
    // tp_read_start();
}