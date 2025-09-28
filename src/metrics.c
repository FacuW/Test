/**
 * @file metrics.c
 * @brief Implementación de funciones para recolectar métricas
 */

#include "cjson/cJSON.h"
#include "monitoring.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Variable global para testing (se puede cambiar el directorio de logs)
char* test_log_dir = NULL;

/**
 * @brief Crea directorios recursivamente
 */
static int create_directory_recursive(const char* path)
{
    char tmp[MAX_PATH_LENGTH];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;

    return 0;
}

/**
 * @brief Lee métricas de CPU desde /proc/stat
 */
int read_cpu_metrics(cpu_metrics_t* cpu)
{
    FILE* file = fopen(PROC_STAT_PATH, "r");
    if (!file)
    {
        perror("Error al abrir /proc/stat");
        return -1;
    }

    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;

    int result = fscanf(file, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq,
                        &softirq, &steal);
    fclose(file);

    if (result != 8)
    {
        fprintf(stderr, "Error al leer datos de CPU\n");
        return -1;
    }

    unsigned long total_time = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long active_time = user + nice + system + irq + softirq + steal;

    cpu->user = (double)user / (double)total_time * 100.0;
    cpu->system = (double)system / (double)total_time * 100.0;
    cpu->idle = (double)idle / (double)total_time * 100.0;
    cpu->usage_percent = (double)active_time / (double)total_time * 100.0;

    return 0;
}

/**
 * @brief Lee métricas de memoria desde /proc/meminfo
 */
int read_memory_metrics(memory_metrics_t* memory)
{
    FILE* file = fopen(PROC_MEMINFO_PATH, "r");
    if (!file)
    {
        perror("Error al abrir /proc/meminfo");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "MemTotal:", 9) == 0)
            sscanf(line, "MemTotal: %ld", &mem_total);
        else if (strncmp(line, "MemFree:", 8) == 0)
            sscanf(line, "MemFree: %ld", &mem_free);
        else if (strncmp(line, "Buffers:", 8) == 0)
            sscanf(line, "Buffers: %ld", &buffers);
        else if (strncmp(line, "Cached:", 7) == 0 && strncmp(line, "SwapCached:", 11) != 0)
            sscanf(line, "Cached: %ld", &cached);
    }
    fclose(file);

    if (mem_total == 0)
    {
        fprintf(stderr, "Error al leer datos de memoria\n");
        return -1;
    }

    memory->total = mem_total;
    memory->free = mem_free;
    memory->buffers = buffers;
    memory->cached = cached;
    memory->used = mem_total - mem_free - buffers - cached;

    return 0;
}

/**
 * @brief Lee métricas de carga desde /proc/loadavg
 */
int read_load_metrics(load_metrics_t* load)
{
    FILE* file = fopen(PROC_LOADAVG_PATH, "r");
    if (!file)
    {
        perror("Error al abrir /proc/loadavg");
        return -1;
    }

    int result = fscanf(file, "%lf %lf %lf", &load->load_1m, &load->load_5m, &load->load_15m);
    fclose(file);

    if (result != 3)
    {
        fprintf(stderr, "Error al leer datos de carga\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Inicializa el sistema de monitoreo
 */
int init_monitoring_system(void)
{
    const char* log_dir = test_log_dir ? test_log_dir : DEFAULT_LOG_DIR;

    struct stat st = {0};
    if (stat(log_dir, &st) == -1)
    {
        if (create_directory_recursive(log_dir) != 0)
        {
            perror("Error al crear directorio de logs");
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Recolecta todas las métricas
 */
int collect_metrics(system_metrics_t* metrics)
{
    if (!metrics)
        return -1;

    metrics->timestamp = time(NULL);

    if (read_cpu_metrics(&metrics->cpu) != 0)
        return -1;
    if (read_memory_metrics(&metrics->memory) != 0)
        return -1;
    if (read_load_metrics(&metrics->load) != 0)
        return -1;

    return 0;
}

/**
 * @brief Guarda métricas en formato NDJSON
 */
int save_metrics_to_json(const system_metrics_t* metrics, const char* filepath)
{
    if (!metrics)
        return -1;

    // Usar el filepath proporcionado o generar uno por defecto
    char filename[MAX_FILENAME_LENGTH];
    if (filepath && strlen(filepath) > 0)
    {
        snprintf(filename, sizeof(filename), "%s", filepath);
    }
    else
    {
        // Generar nombre de archivo metrics-YYYYMMDD.log con la fecha actual
        time_t t = time(NULL);
        struct tm tm_info;
        localtime_r(&t, &tm_info);

        const char* log_dir = test_log_dir ? test_log_dir : DEFAULT_LOG_DIR;
        snprintf(filename, sizeof(filename), "%smetrics-%04d%02d%02d.log", log_dir, tm_info.tm_year + 1900,
                 tm_info.tm_mon + 1, tm_info.tm_mday);
    }

    cJSON* json = cJSON_CreateObject();
    if (!json)
        return -1;

    cJSON_AddNumberToObject(json, "timestamp", (double)metrics->timestamp);
    cJSON_AddNumberToObject(json, "cpu_user", metrics->cpu.user);
    cJSON_AddNumberToObject(json, "cpu_system", metrics->cpu.system);
    cJSON_AddNumberToObject(json, "cpu_usage", metrics->cpu.usage_percent);
    cJSON_AddNumberToObject(json, "mem_total", (double)metrics->memory.total);
    cJSON_AddNumberToObject(json, "mem_free", (double)metrics->memory.free);
    cJSON_AddNumberToObject(json, "mem_used", (double)metrics->memory.used);
    cJSON_AddNumberToObject(json, "load_1m", metrics->load.load_1m);
    cJSON_AddNumberToObject(json, "load_5m", metrics->load.load_5m);
    cJSON_AddNumberToObject(json, "load_15m", metrics->load.load_15m);

    char* json_string = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!json_string)
        return -1;

    FILE* file = fopen(filename, "a");
    if (!file)
    {
        free(json_string);
        return -1;
    }

    fprintf(file, "%s\n", json_string);
    fclose(file);
    free(json_string);

    return 0;
}

void cleanup_monitoring_system(void)
{
    // No resources to cleanup currently
    // This function may be used in the future if it's necessary
}
