#ifndef __DOOR_SENSOR_H__
#define __DOOR_SENSOR_H__

void led_on(void);
void led_off(void);
void led_blink(uint8_t num, uint32_t delay_ms);
void door_sensor_init();

#endif