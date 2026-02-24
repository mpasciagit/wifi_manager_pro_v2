#include "storage_nvs.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include <string.h>

static const char *TAG = "storage_nvs";

static const char *NVS_NAMESPACE = "wifi_storage";
static const char *KEY_SSID = "ssid";
static const char *KEY_PASS = "password";

/* Initialize NVS */
void storage_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrupted or version mismatch, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } 
    else {
        ESP_ERROR_CHECK(err);
    }

    ESP_LOGI(TAG, "NVS initialized successfully");
}

/* Save WiFi credentials */
bool storage_save_wifi_credentials(const char *ssid, const char *password)
{
    if (!ssid || !password) {
        ESP_LOGE(TAG, "SSID or password is NULL");
        return false;
    }

    size_t ssid_len = strlen(ssid);
    size_t pass_len = strlen(password);

    if (ssid_len == 0 || ssid_len >= WIFI_SSID_MAX_LEN) {
        ESP_LOGE(TAG, "Invalid SSID length");
        return false;
    }

    if (pass_len >= WIFI_PASS_MAX_LEN) {
        ESP_LOGE(TAG, "Password too long");
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS (%s)", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(handle, KEY_SSID, ssid);
    if (err != ESP_OK) goto fail;

    err = nvs_set_str(handle, KEY_PASS, password);
    if (err != ESP_OK) goto fail;

    err = nvs_commit(handle);
    if (err != ESP_OK) goto fail;

    nvs_close(handle);

    ESP_LOGI(TAG, "WiFi credentials saved successfully");
    return true;

fail:
    ESP_LOGE(TAG, "Failed to save WiFi credentials (%s)", esp_err_to_name(err));
    nvs_close(handle);
    return false;
}

/* Load WiFi credentials */
bool storage_load_wifi_credentials(char *ssid_out, char *password_out)
{
    if (!ssid_out || !password_out) {
        ESP_LOGE(TAG, "Output buffers are NULL");
        return false;
    }

    memset(ssid_out, 0, WIFI_SSID_MAX_LEN);
    memset(password_out, 0, WIFI_PASS_MAX_LEN);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "NVS namespace not found (first boot)");
        return false;
    }
    else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS (%s)", esp_err_to_name(err));
        return false;
    }

    size_t ssid_len = WIFI_SSID_MAX_LEN;
    size_t pass_len = WIFI_PASS_MAX_LEN;

    err = nvs_get_str(handle, KEY_SSID, ssid_out, &ssid_len);
    if (err != ESP_OK || ssid_len == 0) goto fail;

    err = nvs_get_str(handle, KEY_PASS, password_out, &pass_len);
    if (err != ESP_OK) goto fail;

    nvs_close(handle);

    ESP_LOGI(TAG, "WiFi credentials loaded from NVS");
    return true;

fail:
    ESP_LOGW(TAG, "No valid WiFi credentials found in NVS");
    nvs_close(handle);
    return false;
}

/* Clear WiFi credentials */
bool storage_clear_wifi_credentials(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS (%s)", esp_err_to_name(err));
        return false;
    }

    nvs_erase_key(handle, KEY_SSID);
    nvs_erase_key(handle, KEY_PASS);

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit erase (%s)", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "WiFi credentials cleared");
    return true;
}

bool storage_wifi_credentials_exist(void) {
    // Esta es la lógica mínima para que el Linker la encuentre
    char ssid[32] = {0};
    char pass[64] = {0};
    
    // Si logramos cargar algo, es que existen
    return storage_load_wifi_credentials(ssid, pass);
}