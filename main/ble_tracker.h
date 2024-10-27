#ifndef __BLE_TRACKER_H__
#define __BLE_TRACKER_H__

#include <inttypes.h>

#define DEVICE_NUMBER         5
#define NUMBER_OF_ARRIVALS    1
#define NUMBER_OF_LEAVE       4

typedef struct {
    char name[32];
    uint8_t mac[6];
    int confidence;
    int rssi;
} ble_device_t;

typedef struct {
    ble_device_t devices[DEVICE_NUMBER];
    int scan_state;
} ble_scan_result_t;

extern ble_scan_result_t ble_tracker_data;

#endif // __BLE_TRACKER_H__