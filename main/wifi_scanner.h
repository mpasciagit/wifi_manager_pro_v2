#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Estructura para almacenar los resultados del escaneo de redes.
 */
typedef struct {
    char ssid[33];         /**< Nombre de la red (SSID) */
    int8_t rssi;           /**< Fuerza de la señal en dBm */
    uint8_t authmode;      /**< Modo de cifrado */
    uint8_t channel;       /**< Canal de radio */
    bool hidden;           /**< Si la red es oculta */
} wifi_scan_result_t;

void wifi_scanner_init(void);

/**
 * @brief NUEVO: Realiza el escaneo físico y guarda los resultados en RAM.
 * Se debe llamar en el Estado 2 (SCANNING).
 * @return Cantidad de redes encontradas y guardadas.
 */
int wifi_scanner_execute_actual_scan(void); // <--- AGREGADO

/**
 * @brief Obtiene los resultados DESDE LA RAM (sin tocar el hardware).
 * Es la que usa el servidor HTTP para no interrumpir el Portal.
 */
int wifi_scanner_get_results(wifi_scan_result_t *results, int max_results);

/**
 * @brief Verifica si un SSID específico está presente en el "Álbum" de RAM.
 * @param target_ssid Nombre de la red a buscar.
 * @return true si la red está en la última captura, false en caso contrario.
 */
bool wifi_scanner_is_network_available(const char *target_ssid);

#endif // WIFI_SCANNER_H