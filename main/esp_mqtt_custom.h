#ifndef __ESP_MQTT_CUSTOM_H__
#define __ESP_MQTT_CUSTOM_H__

void mqtt_custom_init(void);
void mqtt_send_dev_info(char *dev_name, int confidence);
void mqtt_send_door_status(int status);

#endif // __ESP_MQTT_CUSTOM_H__