#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <time.h>
#include "monitoring.h"

/**
 * @file storage.h
 * @brief Sistema de almacenamiento persistente binario con integridad
 */

// Configuración del almacenamiento
#define STORAGE_DIR "/var/monitoreo/data/"
#define STORAGE_FILE_PREFIX "data-"
#define STORAGE_FILE_SUFFIX ".bin"
#define STORAGE_MAX_PATH_LENGTH 512

// Magic numbers y versiones
#define STORAGE_MAGIC 0x4D4F4E49  // "MONI" en hex
#define STORAGE_VERSION 1
#define STORAGE_RECORD_SIZE 128   // Tamaño fijo de cada registro

// Estructura de la cabecera del archivo
typedef struct __attribute__((packed)) {
    uint32_t magic;           // Magic number para validación
    uint16_t version;         // Versión del formato
    uint16_t record_size;     // Tamaño de cada registro
    uint32_t total_records;   // Número total de registros
    uint32_t valid_records;   // Número de registros válidos
    uint64_t created_time;    // Timestamp de creación
    uint64_t last_write_time; // Timestamp de última escritura
    uint32_t header_crc;      // CRC de la cabecera (excluyendo este campo)
    uint8_t reserved[16];     // Espacio reservado para futuras extensiones
} storage_header_t;

// Estructura de un registro de datos
typedef struct __attribute__((packed)) {
    uint64_t timestamp;       // Timestamp del registro
    uint32_t record_type;     // Tipo de registro (para extensibilidad)
    uint32_t data_length;     // Longitud de los datos válidos
    
    // Datos del sistema de monitoreo
    double cpu_usage;         // Uso de CPU
    double cpu_user;          // CPU tiempo usuario
    double cpu_system;        // CPU tiempo sistema
    uint64_t memory_total;    // Memoria total
    uint64_t memory_used;     // Memoria usada
    uint64_t memory_free;     // Memoria libre
    double load_1m;          // Load average 1 minuto
    double load_5m;          // Load average 5 minutos
    double load_15m;         // Load average 15 minutos
    
    uint8_t padding[32];      // Padding para llegar a tamaño fijo
    uint32_t record_crc;      // CRC del registro (excluyendo este campo)
} storage_record_t;

// Handle del archivo de almacenamiento
typedef struct {
    int fd;                   // File descriptor
    char filepath[STORAGE_MAX_PATH_LENGTH]; // Ruta completa del archivo
    storage_header_t header;  // Cabecera cacheada
    time_t file_date;         // Fecha del archivo (para rotación diaria)
    int is_open;              // Flag de estado
    int is_readonly;          // Flag de solo lectura
} storage_handle_t;

// Estadísticas del almacenamiento
typedef struct {
    uint32_t total_files;     // Total de archivos de datos
    uint32_t total_records;   // Total de registros
    uint32_t valid_records;   // Registros válidos
    uint32_t corrupt_records; // Registros corruptos
    uint64_t total_size;      // Tamaño total en bytes
    time_t oldest_record;     // Timestamp del registro más antiguo
    time_t newest_record;     // Timestamp del registro más nuevo
} storage_stats_t;

// Funciones principales del sistema de almacenamiento
int storage_init(void);
int storage_open_daily_file(storage_handle_t* handle, time_t date, int readonly);
int storage_write_record(storage_handle_t* handle, const system_metrics_t* metrics);
int storage_read_all_records(storage_handle_t* handle, 
                           int (*callback)(const storage_record_t* record, void* user_data),
                           void* user_data);
int storage_close(storage_handle_t* handle);
void storage_cleanup(void);

// Funciones de validación e integridad
int storage_validate_header(const storage_header_t* header);
int storage_validate_record(const storage_record_t* record);
uint32_t storage_calculate_crc32(const void* data, size_t length);
int storage_repair_file(const char* filepath);

// Funciones de estadísticas e información
int storage_get_stats(storage_stats_t* stats);
int storage_list_files(char files[][STORAGE_MAX_PATH_LENGTH], int max_files);
void storage_dump_header(const storage_header_t* header);
void storage_dump_record(const storage_record_t* record);

// Funciones utilitarias
void storage_generate_filename(char* buffer, size_t size, time_t date);
int storage_metrics_to_record(const system_metrics_t* metrics, storage_record_t* record);
int storage_record_to_metrics(const storage_record_t* record, system_metrics_t* metrics);

#endif // STORAGE_H