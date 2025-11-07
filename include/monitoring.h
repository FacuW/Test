#ifndef MONITORING_H
#define MONITORING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MONITORING_VERSION "1.0.0"
#define DEFAULT_LOG_DIR "/var/lib/monitoreo/"
#define DEFAULT_INTERVAL 5
#define DEFAULT_PROMETHEUS_PORT 8080

// Buffer sizes
#define MAX_FILENAME_LENGTH 256
#define MAX_PATH_LENGTH 512
#define MAX_LINE_LENGTH 256
#define MAX_HTTP_RESPONSE_SIZE 8192
#define MAX_COMMAND_LENGTH 300

// Paths de archivos del sistema
#define PROC_STAT_PATH "/proc/stat"
#define PROC_LOADAVG_PATH "/proc/loadavg"
#define PROC_MEMINFO_PATH "/proc/meminfo"

// para evitar magic numbers
#define MIN_VALID_PORT 1024
#define MAX_VALID_PORT 65535
#define LISTEN_QUEUE_SIZE 5
#define BYTES_PER_KB 1024
#define CPU_STAT_FIELDS_COUNT 8
#define TAIL_LINES_DEFAULT 5
#define TAIL_LINES_STR "5"
#define DIR_PERMISSIONS 0755
#define THREAD_STARTUP_DELAY_US 100000

// Estructuras para las métricas
typedef struct
{
    double user;
    double system;
    double idle;
    double usage_percent;
} cpu_metrics_t;

typedef struct
{
    long total;
    long free;
    long used;
    long buffers;
    long cached;
} memory_metrics_t;

typedef struct
{
    double load_1m;
    double load_5m;
    double load_15m;
} load_metrics_t;

typedef struct
{
    time_t timestamp;
    cpu_metrics_t cpu;
    memory_metrics_t memory;
    load_metrics_t load;
} system_metrics_t;

// Funciones principales
int init_monitoring_system(void);
int collect_metrics(system_metrics_t* metrics);
int save_metrics_to_json(const system_metrics_t* metrics, const char* filepath);
void cleanup_monitoring_system(void);

// Funciones para testing
int read_cpu_metrics(cpu_metrics_t* cpu);
int read_memory_metrics(memory_metrics_t* memory);
int read_load_metrics(load_metrics_t* load);

// Funciones para Prometheus
int init_prometheus_metrics(void);
int update_prometheus_metrics(const system_metrics_t* metrics);
int start_prometheus_server(int port);
void cleanup_prometheus(void);

// Variable global para testing
extern char* test_log_dir;

#endif
