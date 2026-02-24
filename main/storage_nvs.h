#ifndef STORAGE_NVS_H
#define STORAGE_NVS_H

#include <stdbool.h>

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

/* Initialize NVS storage */
void storage_nvs_init(void);

/* Save Wi-Fi credentials */
bool storage_save_wifi_credentials(const char *ssid, const char *password);

/* Load Wi-Fi credentials */
bool storage_load_wifi_credentials(char *ssid_out, char *password_out);

/* Clear stored Wi-Fi credentials */
bool storage_clear_wifi_credentials(void);

/* Check if credentials exist and are valid */
bool storage_wifi_credentials_exist(void);

#endif // STORAGE_NVS_H
