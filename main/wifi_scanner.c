#include "wifi_scanner.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "wifi_scanner";

// --- El ÁLBUM (Memoria RAM Estática) ---
static wifi_scan_result_t g_scan_album[20];
static int g_networks_found = 0;

void wifi_scanner_init(void) {
    memset(g_scan_album, 0, sizeof(g_scan_album));
    g_networks_found = 0;
    ESP_LOGI(TAG, "WiFi scanner inicializado (Memoria limpia)");
}

/**
 * ESTA FUNCIÓN ES EL "FOTÓGRAFO"
 * Retorna: >0 (redes encontradas), 0 (no hay redes), <0 (Error de hardware)
 */
int wifi_scanner_execute_actual_scan(void) {
    wifi_ap_record_t ap_info[20];
    uint16_t ap_count = 0;
    uint16_t max_number = 20;

    wifi_scan_config_t scan_config = {
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    ESP_LOGI(TAG, "Hardware: Iniciando escaneo real...");
    
    // 1. Intentar el escaneo
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error hardware radio: %s", esp_err_to_name(ret));
        return -1; // <--- CAMBIO CLAVE: Devolvemos -1 para indicar FALLO DE HARDWARE
    }

    // 2. Si llegamos aquí, el hardware respondió. Obtenemos resultados.
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > max_number) ap_count = max_number;

    // Solo pedimos records si ap_count > 0 para evitar errores innecesarios
    if (ap_count > 0) {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

        // --- ACTUALIZAR EL ÁLBUM ---
        g_networks_found = ap_count;
        memset(g_scan_album, 0, sizeof(g_scan_album));

        for (int i = 0; i < ap_count; i++) {
            strncpy(g_scan_album[i].ssid, (char *)ap_info[i].ssid, sizeof(g_scan_album[i].ssid) - 1);
            g_scan_album[i].rssi = ap_info[i].rssi;
            g_scan_album[i].authmode = ap_info[i].authmode;
            g_scan_album[i].channel = ap_info[i].primary;
            g_scan_album[i].hidden = (strlen((char *)ap_info[i].ssid) == 0);
        }
    } else {
        g_networks_found = 0;
        memset(g_scan_album, 0, sizeof(g_scan_album));
    }

    // IMPORTANTE: Limpiar el estado del driver para que la próxima vez esté fresco
    esp_wifi_scan_stop(); 
    
    ESP_LOGI(TAG, "Escaneo finalizado. %d redes guardadas en RAM.", g_networks_found);
    return (int)g_networks_found;
}

/**
 * ESTA FUNCIÓN ES EL "LECTOR DEL ÁLBUM" (La usa el Portal HTTP)
 * No toca el radio. Es 100% segura para llamar mientras hay clientes conectados.
 */
int wifi_scanner_get_results(wifi_scan_result_t *results, int max_results) {
    if (!results || max_results <= 0) return 0;

    // Entregamos lo que ya tenemos en memoria
    int count_to_copy = (g_networks_found < max_results) ? g_networks_found : max_results;
    
    memcpy(results, g_scan_album, sizeof(wifi_scan_result_t) * count_to_copy);
    
    ESP_LOGI(TAG, "Servidor HTTP: Entregando %d redes desde memoria RAM.", count_to_copy);
    return count_to_copy;
}

/**
 * VERIFICACIÓN PASIVA
 */
bool wifi_scanner_is_network_available(const char *target_ssid) {
    if (!target_ssid || strlen(target_ssid) == 0) return false;

    // Buscamos en el álbum, no volvemos a sacar la foto.
    for (int i = 0; i < g_networks_found; i++) {
        if (strcmp(g_scan_album[i].ssid, target_ssid) == 0) {
            return true;
        }
    }
    return false;
}