/**
 * @file storage.c
 * @brief Implementación del sistema de almacenamiento persistente binario
 */

#include "storage.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Tabla CRC32
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

/**
 * @brief Inicializa la tabla CRC32
 */
static void init_crc32_table(void)
{
    if (crc32_table_initialized)
        return;

    uint32_t polynomial = 0xEDB88320;

    for (int i = 0; i < 256; i++)
    {
        uint32_t crc = (uint32_t)(unsigned int)i;
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ polynomial;
            }
            else
            {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }

    crc32_table_initialized = 1;
}

/**
 * @brief Calcula CRC32 de un bloque de datos
 */
uint32_t storage_calculate_crc32(const void* data, size_t length)
{
    if (!crc32_table_initialized)
    {
        init_crc32_table();
    }

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t i = 0; i < length; i++)
    {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Crea directorio recursivamente
 */
static int create_directory_recursive(const char* path)
{
    char tmp[STORAGE_MAX_PATH_LENGTH];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
            {
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Inicializa el sistema de almacenamiento
 */
int storage_init(void)
{
    // Crear directorio de almacenamiento si no existe
    if (create_directory_recursive(STORAGE_DIR) != 0)
    {
        perror("Error creating storage directory");
        return -1;
    }

    // Inicializar tabla CRC32
    init_crc32_table();

    printf("Storage system initialized\n");
    printf("Storage directory: %s\n", STORAGE_DIR);

    return 0;
}

/**
 * @brief Genera nombre de archivo basado en fecha
 */
void storage_generate_filename(char* buffer, size_t size, time_t date)
{
    struct tm* tm_info = localtime(&date);
    snprintf(buffer, size, "%s%s%04d%02d%02d%s", STORAGE_DIR, STORAGE_FILE_PREFIX, tm_info->tm_year + 1900,
             tm_info->tm_mon + 1, tm_info->tm_mday, STORAGE_FILE_SUFFIX);
}

/**
 * @brief Crea y configura cabecera del archivo
 */
static void init_storage_header(storage_header_t* header)
{
    memset(header, 0, sizeof(storage_header_t));

    header->magic = STORAGE_MAGIC;
    header->version = STORAGE_VERSION;
    header->record_size = STORAGE_RECORD_SIZE;
    header->total_records = 0;
    header->valid_records = 0;
    header->created_time = (uint64_t)time(NULL);
    header->last_write_time = header->created_time;

    // Calcular CRC de la cabecera (excluyendo el campo header_crc)
    header->header_crc = storage_calculate_crc32(header, sizeof(storage_header_t) - sizeof(header->header_crc) -
                                                             sizeof(header->reserved));
}

/**
 * @brief Valida la cabecera del archivo
 */
int storage_validate_header(const storage_header_t* header)
{
    if (!header)
        return -1;

    // Verificar magic number
    if (header->magic != STORAGE_MAGIC)
    {
        fprintf(stderr, "Invalid storage file magic number\n");
        return -1;
    }

    // Verificar versión
    if (header->version != STORAGE_VERSION)
    {
        fprintf(stderr, "Unsupported storage file version: %d\n", header->version);
        return -1;
    }

    // Verificar tamaño de registro
    if (header->record_size != STORAGE_RECORD_SIZE)
    {
        fprintf(stderr, "Invalid record size: %d\n", header->record_size);
        return -1;
    }

    // Verificar CRC de la cabecera
    uint32_t calculated_crc = storage_calculate_crc32(header, sizeof(storage_header_t) - sizeof(header->header_crc) -
                                                                  sizeof(header->reserved));

    if (header->header_crc != calculated_crc)
    {
        fprintf(stderr, "Header CRC mismatch\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Abre o crea archivo diario de almacenamiento
 */
int storage_open_daily_file(storage_handle_t* handle, time_t date, int readonly)
{
    if (!handle)
        return -1;

    memset(handle, 0, sizeof(storage_handle_t));

    // Generar nombre de archivo
    storage_generate_filename(handle->filepath, sizeof(handle->filepath), date);
    handle->file_date = date;
    handle->is_readonly = readonly;

    // Abrir o crear archivo
    int flags = readonly ? O_RDONLY : (O_RDWR | O_CREAT);
    int mode = 0644;

    handle->fd = open(handle->filepath, flags, mode);
    if (handle->fd == -1)
    {
        perror("Error opening storage file");
        return -1;
    }

    // Leer o crear cabecera
    ssize_t header_size = read(handle->fd, &handle->header, sizeof(storage_header_t));

    if (header_size == 0)
    {
        // Archivo nuevo, crear cabecera
        if (readonly)
        {
            fprintf(stderr, "Cannot create header in readonly mode\n");
            close(handle->fd);
            return -1;
        }

        init_storage_header(&handle->header);

        if (write(handle->fd, &handle->header, sizeof(storage_header_t)) != sizeof(storage_header_t))
        {
            perror("Error writing header");
            close(handle->fd);
            return -1;
        }

        printf("Created new storage file: %s\n", handle->filepath);
    }
    else if (header_size == sizeof(storage_header_t))
    {
        // Archivo existente, validar cabecera
        if (storage_validate_header(&handle->header) != 0)
        {
            fprintf(stderr, "Invalid storage file header\n");
            close(handle->fd);
            return -1;
        }

        printf("Opened existing storage file: %s\n", handle->filepath);
        printf("Records: %u total, %u valid\n", handle->header.total_records, handle->header.valid_records);
    }
    else
    {
        fprintf(stderr, "Incomplete header in storage file\n");
        close(handle->fd);
        return -1;
    }

    handle->is_open = 1;
    return 0;
}

/**
 * @brief Convierte métricas del sistema a registro de almacenamiento
 */
int storage_metrics_to_record(const system_metrics_t* metrics, storage_record_t* record)
{
    if (!metrics || !record)
        return -1;

    memset(record, 0, sizeof(storage_record_t));

    record->timestamp = (uint64_t)metrics->timestamp;
    record->record_type = 1; // Tipo 1 = métricas del sistema
    record->data_length = (uint32_t)(sizeof(storage_record_t) - offsetof(storage_record_t, cpu_usage) -
                                     sizeof(record->padding) - sizeof(record->record_crc));

    record->cpu_usage = metrics->cpu.usage_percent;
    record->cpu_user = metrics->cpu.user;
    record->cpu_system = metrics->cpu.system;
    record->memory_total = (uint64_t)metrics->memory.total;
    record->memory_used = (uint64_t)metrics->memory.used;
    record->memory_free = (uint64_t)metrics->memory.free;
    record->load_1m = metrics->load.load_1m;
    record->load_5m = metrics->load.load_5m;
    record->load_15m = metrics->load.load_15m;

    // Calcular CRC del registro (excluyendo el campo record_crc)
    record->record_crc = storage_calculate_crc32(record, sizeof(storage_record_t) - sizeof(record->record_crc));

    return 0;
}

/**
 * @brief Convierte registro de almacenamiento a métricas del sistema
 */
int storage_record_to_metrics(const storage_record_t* record, system_metrics_t* metrics)
{
    if (!record || !metrics)
        return -1;

    memset(metrics, 0, sizeof(system_metrics_t));

    metrics->timestamp = (time_t)record->timestamp;
    metrics->cpu.usage_percent = record->cpu_usage;
    metrics->cpu.user = record->cpu_user;
    metrics->cpu.system = record->cpu_system;
    metrics->memory.total = (long)record->memory_total;
    metrics->memory.used = (long)record->memory_used;
    metrics->memory.free = (long)record->memory_free;
    metrics->load.load_1m = record->load_1m;
    metrics->load.load_5m = record->load_5m;
    metrics->load.load_15m = record->load_15m;

    return 0;
}

/**
 * @brief Valida un registro
 */
int storage_validate_record(const storage_record_t* record)
{
    if (!record)
        return -1;

    // Verificar CRC del registro
    uint32_t calculated_crc = storage_calculate_crc32(record, sizeof(storage_record_t) - sizeof(record->record_crc));

    if (record->record_crc != calculated_crc)
    {
        return -1; // CRC inválido
    }

    // Validaciones básicas de rango
    time_t now = time(NULL);
    if (record->timestamp <= 0 || record->timestamp > (uint64_t)(now + 3600))
    {
        return -1; // Timestamp inválido
    }

    if (record->cpu_usage < 0.0 || record->cpu_usage > 100.0)
    {
        return -1; // CPU usage fuera de rango
    }

    return 0;
}

/**
 * @brief Escribe un registro al archivo
 */
int storage_write_record(storage_handle_t* handle, const system_metrics_t* metrics)
{
    if (!handle || !handle->is_open || handle->is_readonly || !metrics)
    {
        return -1;
    }

    storage_record_t record;
    if (storage_metrics_to_record(metrics, &record) != 0)
    {
        return -1;
    }

    // Posicionarse al final del archivo
    if (lseek(handle->fd, 0, SEEK_END) == -1)
    {
        perror("Error seeking to end of file");
        return -1;
    }

    // Escribir registro
    ssize_t written = write(handle->fd, &record, sizeof(storage_record_t));
    if (written != sizeof(storage_record_t))
    {
        perror("Error writing record");
        return -1;
    }

    // Actualizar cabecera
    handle->header.total_records++;
    handle->header.valid_records++;
    handle->header.last_write_time = (uint64_t)time(NULL);

    // Recalcular CRC de la cabecera
    handle->header.header_crc =
        storage_calculate_crc32(&handle->header, sizeof(storage_header_t) - sizeof(handle->header.header_crc) -
                                                     sizeof(handle->header.reserved));

    // Escribir cabecera actualizada
    if (lseek(handle->fd, 0, SEEK_SET) == -1)
    {
        perror("Error seeking to beginning of file");
        return -1;
    }

    if (write(handle->fd, &handle->header, sizeof(storage_header_t)) != sizeof(storage_header_t))
    {
        perror("Error updating header");
        return -1;
    }

    // Sincronizar con disco
    fsync(handle->fd);

    return 0;
}

/**
 * @brief Lee todos los registros del archivo
 */
int storage_read_all_records(storage_handle_t* handle, int (*callback)(const storage_record_t* record, void* user_data),
                             void* user_data)
{
    if (!handle || !handle->is_open || !callback)
    {
        return -1;
    }

    // Posicionarse después de la cabecera
    if (lseek(handle->fd, sizeof(storage_header_t), SEEK_SET) == -1)
    {
        perror("Error seeking past header");
        return -1;
    }

    storage_record_t record;
    uint32_t records_read = 0;
    uint32_t valid_records = 0;
    uint32_t corrupt_records = 0;

    while (read(handle->fd, &record, sizeof(storage_record_t)) == sizeof(storage_record_t))
    {
        records_read++;

        if (storage_validate_record(&record) == 0)
        {
            valid_records++;

            // Llamar callback con registro válido
            int result = callback(&record, user_data);
            if (result != 0)
            {
                break; // Callback solicita terminar
            }
        }
        else
        {
            corrupt_records++;
            fprintf(stderr, "Corrupt record found at position %u\n", records_read);
        }
    }

    printf("Read %u records (%u valid, %u corrupt)\n", records_read, valid_records, corrupt_records);

    return (int)valid_records;
}

/**
 * @brief Cierra un handle de almacenamiento
 */
int storage_close(storage_handle_t* handle)
{
    if (!handle || !handle->is_open)
    {
        return -1;
    }

    if (close(handle->fd) == -1)
    {
        perror("Error closing storage file");
        return -1;
    }

    handle->is_open = 0;
    handle->fd = -1;

    return 0;
}

/**
 * @brief Lista archivos de almacenamiento disponibles
 */
int storage_list_files(char files[][STORAGE_MAX_PATH_LENGTH], int max_files)
{
    DIR* dir = opendir(STORAGE_DIR);
    if (!dir)
    {
        perror("Error opening storage directory");
        return -1;
    }

    struct dirent* entry;
    int file_count = 0;

    while ((entry = readdir(dir)) != NULL && file_count < max_files)
    {
        if (strstr(entry->d_name, STORAGE_FILE_PREFIX) == entry->d_name &&
            strstr(entry->d_name, STORAGE_FILE_SUFFIX) != NULL)
        {

            snprintf(files[file_count], STORAGE_MAX_PATH_LENGTH, "%s%s", STORAGE_DIR, entry->d_name);
            file_count++;
        }
    }

    closedir(dir);
    return file_count;
}

/**
 * @brief Obtiene estadísticas del sistema de almacenamiento
 */
int storage_get_stats(storage_stats_t* stats)
{
    if (!stats)
        return -1;

    memset(stats, 0, sizeof(storage_stats_t));

    char files[32][STORAGE_MAX_PATH_LENGTH];
    int file_count = storage_list_files(files, 32);

    if (file_count < 0)
    {
        return -1;
    }

    stats->total_files = (uint32_t)file_count;
    stats->oldest_record = time(NULL);
    stats->newest_record = 0;

    for (int i = 0; i < file_count; i++)
    {
        storage_handle_t handle;
        if (storage_open_daily_file(&handle, time(NULL), 1) == 0)
        {
            // Usar la ruta del archivo encontrado
            strncpy(handle.filepath, files[i], sizeof(handle.filepath) - 1);
            close(handle.fd);

            handle.fd = open(files[i], O_RDONLY);
            if (handle.fd != -1)
            {
                if (read(handle.fd, &handle.header, sizeof(storage_header_t)) == sizeof(storage_header_t))
                {
                    if (storage_validate_header(&handle.header) == 0)
                    {
                        stats->total_records += handle.header.total_records;
                        stats->valid_records += handle.header.valid_records;
                        stats->corrupt_records += (handle.header.total_records - handle.header.valid_records);

                        struct stat file_stat;
                        if (fstat(handle.fd, &file_stat) == 0)
                        {
                            stats->total_size += (uint64_t)file_stat.st_size;
                        }

                        if ((time_t)handle.header.created_time < stats->oldest_record)
                        {
                            stats->oldest_record = (time_t)handle.header.created_time;
                        }

                        if ((time_t)handle.header.last_write_time > stats->newest_record)
                        {
                            stats->newest_record = (time_t)handle.header.last_write_time;
                        }
                    }
                }
                close(handle.fd);
            }
        }
    }

    return 0;
}

/**
 * @brief Muestra información de la cabecera
 */
void storage_dump_header(const storage_header_t* header)
{
    if (!header)
        return;

    printf("\n=== STORAGE HEADER DUMP ===\n");
    printf("Magic: 0x%08X\n", header->magic);
    printf("Version: %u\n", header->version);
    printf("Record size: %u bytes\n", header->record_size);
    printf("Total records: %u\n", header->total_records);
    printf("Valid records: %u\n", header->valid_records);

    // Evitar warning de alignment usando variable temporal
    time_t created = (time_t)header->created_time;
    time_t last_write = (time_t)header->last_write_time;
    printf("Created: %s", ctime(&created));
    printf("Last write: %s", ctime(&last_write));

    printf("Header CRC: 0x%08X\n", header->header_crc);
    printf("==========================\n\n");
}

/**
 * @brief Muestra información de un registro
 */
void storage_dump_record(const storage_record_t* record)
{
    if (!record)
        return;

    printf("Record: timestamp=%ld, cpu=%.2f%%, mem_used=%lu, load_1m=%.2f\n", (long)record->timestamp,
           record->cpu_usage, record->memory_used, record->load_1m);
}

/**
 * @brief Limpia el sistema de almacenamiento
 */
void storage_cleanup(void)
{
    // No hay recursos globales que limpiar por ahora
    printf("Storage system cleaned up\n");
}
