#include "system_state.h"
#include "wifi_manager.h"
#include "wifi_provisioning.h"
#include "wifi_scanner.h" 
#include "storage_nvs.h"
#include "led_status.h"
#include "dns_server.h"
#include "http_server.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "system_state";
static system_state_t current_state = SYSTEM_STATE_BOOT;
static bool force_provisioning = false;
static bool es_nueva_config = false; // Solo true si viene del portal
static bool modo_espera_scan = false;
static int contador_espera = 0;

#define STA_CONNECT_TIMEOUT_MS 15000
#define RETRY_DELAY_MS         5000 
#define MAX_STA_RETRIES        3

// Bit de conexión (definido aquí para consistencia)
#define WIFI_CONNECTED_BIT BIT0 

void system_state_set(system_state_t state) {
    current_state = state;
    ESP_LOGI(TAG, "Cambiando estado del sistema -> %d", state);
}

system_state_t system_state_get(void) {
    return current_state;
}

void system_state_init(void) {
    ESP_LOGI(TAG, "Inicializando gestor de estados...");
    system_state_set(SYSTEM_STATE_BOOT);
    xTaskCreate(system_state_task, "system_state_task", 4096, NULL, 5, NULL);
}

void system_state_task(void *pvParameters) {
    TickType_t connect_start_time = 0;
    int retry_count = 0;
    char ssid[32], pass[64]; // Buffers siempre listos arriba

    // 1. Configuración del GPIO 0 (Vigilante)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    while (1) {
        // --- VIGILANTE DE BOTÓN (En cada iteración) ---
        if (gpio_get_level(GPIO_NUM_0) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce
            if (gpio_get_level(GPIO_NUM_0) == 0) {
                ESP_LOGW(TAG, "¡Botón detectado! Forzando Modo Configuración.");
                force_provisioning = true;
                system_state_set(SYSTEM_STATE_SCANNING); // Saltamos a Scanning para decidir
            }
        }

        system_state_t state = system_state_get();
        switch (state) {
            case SYSTEM_STATE_BOOT:
                led_status_set(LED_STATUS_BOOTING);
                esp_err_t ret = esp_wifi_start();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Fallo crítico al iniciar radio");
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                system_state_set(SYSTEM_STATE_SCANNING);
                break;

            case SYSTEM_STATE_SCANNING:
                // --- PASO 1: EL VIGILANTE DEL TIEMPO ---
                if (modo_espera_scan) {
                    // AGREGAMOS ESTO: Si presionan el botón, abortamos la espera para ir a escanear YA
                    if (force_provisioning) {
                        ESP_LOGI(TAG, "Botón detectado. Abortando espera para escanear...");
                        modo_espera_scan = false;
                        contador_espera = 0;
                        // No hacemos break, dejamos que caiga directamente al Paso 2 (Escaneo)
                    }
                    else if (contador_espera < 30) {
                        contador_espera++;
                        // Log cada 5s para no inundar la consola
                        if (contador_espera % 5 == 0) ESP_LOGI(TAG, "Esperando... (%d/30s)", contador_espera);
            
                        vTaskDelay(pdMS_TO_TICKS(500));
                        system_state_set(SYSTEM_STATE_SCANNING); // "Auto-salto" para re-entrar
                        break; 
                    } else {
                    // Se acabó el tiempo de espera
                    modo_espera_scan = false;
                    contador_espera = 0;
                    // No hacemos break, dejamos que fluya al escaneo abajo
                    }
                }
                
                // --- PASO 2: EL ESCANEO REAL ---
                ESP_LOGI(TAG, "Estado: SCANNING (Armando álbum)");
                int redes_encontradas = wifi_scanner_execute_actual_scan();

                if (redes_encontradas <= 0) {
                    ESP_LOGW(TAG, "No hay redes. Iniciando cuenta de 30s...");
                    modo_espera_scan = true;
                    contador_espera = 0;
                    system_state_set(SYSTEM_STATE_SCANNING);
                }
                else {
                    // --- PASO 3 NODO DE DECISIÓN - BOTÓN
                    if (force_provisioning) {
                        ESP_LOGW(TAG, "Banderín activo. Saltando a Provisión.");
                        force_provisioning = false; // Bajamos banderín
                        system_state_set(SYSTEM_STATE_PROVISIONING);
                    }
                    else if (storage_load_wifi_credentials(ssid, pass)) {
                        // REVISIÓN DEL ÁLBUM: ¿La red guardada está presente?
                        if (wifi_scanner_is_network_available(ssid)) {
                            ESP_LOGI(TAG, "Red '%s' hallada en el lugar. Conectando...", ssid);
                            wifi_manager_set_credentials(ssid, pass);
                            wifi_manager_reconnect(); 
                            system_state_set(SYSTEM_STATE_TRY_STA);
                            connect_start_time = xTaskGetTickCount();
                        } else {
                            ESP_LOGW(TAG, "Red '%s' no detectada en el escaneo actual.", ssid);
                            system_state_set(SYSTEM_STATE_PROVISIONING);
                        }
                    } else {
                        ESP_LOGW(TAG, "NVS vacío. Yendo a Provisión.");
                        system_state_set(SYSTEM_STATE_PROVISIONING);
                    }
                }
                break;

            case SYSTEM_STATE_PROVISIONING:
                led_status_set(LED_STATUS_PROVISIONING);
                if (!wifi_provisioning_is_active()) {
                    wifi_provisioning_start(); 
                    dns_server_start();
                    http_server_start();
                }
                
                if (wifi_provisioning_has_new_credentials()) {
                    wifi_credentials_t creds;
                    wifi_provisioning_get_credentials(&creds);
                    wifi_manager_set_credentials(creds.ssid, creds.password);
                    es_nueva_config = true;

                    http_server_stop();
                    dns_server_stop();
                    wifi_provisioning_stop();
                    
                    wifi_manager_reconnect();
                    retry_count = 0; 
                    connect_start_time = 0; 
                    wifi_manager_reset_last_disconnect_reason();
                    system_state_set(SYSTEM_STATE_TRY_STA);
                }
                break;

            case SYSTEM_STATE_TRY_STA:
                led_status_set(LED_STATUS_WIFI_CONNECTING);
                EventGroupHandle_t ev_grp = wifi_manager_get_event_group();

                if (ev_grp != NULL) {
                    EventBits_t bits = xEventGroupWaitBits(ev_grp, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(100));

                    if (bits & WIFI_CONNECTED_BIT) { // 1. ¿Estamos conectados?
    
                        // 2. Solo si es una configuración nueva, guardamos en la Flash
                        if (es_nueva_config) {
                            wifi_manager_get_credentials(ssid, pass); 
                            storage_save_wifi_credentials(ssid, pass); 
                            es_nueva_config = false; // Cerramos el seguro
                            ESP_LOGI(TAG, "Nuevas credenciales guardadas en NVS.");
                        } else {
                            ESP_LOGI(TAG, "Conexión exitosa con datos conocidos (No se escribe Flash).");
                        }

                        // 3. Siempre pasamos al estado conectado, sea nueva o vieja la red
                        system_state_set(SYSTEM_STATE_CONNECTED);
                        retry_count = 0;
                        connect_start_time = 0;
                        } else {
                        uint8_t reason = wifi_manager_get_last_disconnect_reason();

                        if (reason == WIFI_REASON_AUTH_FAIL || reason == 15) { 
                            ESP_LOGE(TAG, "Fallo de credenciales (Razón: %d). Regresando a Provisión.", reason);
                            system_state_set(SYSTEM_STATE_PROVISIONING);
                        } else {
                            if (connect_start_time == 0) connect_start_time = xTaskGetTickCount();
                            
                            if ((xTaskGetTickCount() - connect_start_time) > pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS)) {
                                ESP_LOGW(TAG, "Timeout alcanzado");
                                connect_start_time = 0;
                                system_state_set(SYSTEM_STATE_DISCONNECTED);
                            }
                        }
                    }
                }
                break;

            case SYSTEM_STATE_CONNECTED:
                led_status_set(LED_STATUS_WIFI_CONNECTED);
                if (!wifi_manager_is_connected()) {
                    ESP_LOGW(TAG, "Conexión perdida.");
                    system_state_set(SYSTEM_STATE_DISCONNECTED);
                }
                break;

            case SYSTEM_STATE_DISCONNECTED:
                if (retry_count >= MAX_STA_RETRIES) {
                    ESP_LOGW(TAG, "Reintentos agotados. Re-evaluando con escaneo...");
                    retry_count = 0; 
                    system_state_set(SYSTEM_STATE_SCANNING); 
                } else {
                    retry_count++;
                    ESP_LOGI(TAG, "Reintento %d de %d...", retry_count, MAX_STA_RETRIES);
                    vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
                    wifi_manager_reconnect();
                    system_state_set(SYSTEM_STATE_TRY_STA);
                }
                break;

            case SYSTEM_STATE_ERROR:
                led_status_set(LED_STATUS_ERROR);
                vTaskDelay(pdMS_TO_TICKS(5000));
                system_state_set(SYSTEM_STATE_BOOT);
                break;

            default:
                system_state_set(SYSTEM_STATE_ERROR);
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}