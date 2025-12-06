#ifndef SANDBOX_H
#define SANDBOX_H

#include <sys/types.h>
#include <time.h>

/**
 * @file sandbox.h
 * @brief Sistema de sandbox para ejecución controlada de plugins C++
 */

// Configuración del sandbox
#define SANDBOX_PLUGINS_DIR "/var/monitoreo/plugins/"
#define SANDBOX_LOGS_DIR "/var/monitoreo/logs/"
#define SANDBOX_WORK_DIR "/var/monitoreo/sandbox/"
#define SANDBOX_LOG_FILE "/var/monitoreo/logs/sandbox.log"

#define SANDBOX_MAX_PATH_LENGTH 512
#define SANDBOX_MAX_OUTPUT_SIZE (16 * 1024) // 16KB máximo de salida
#define SANDBOX_MAX_PLUGINS 32
#define SANDBOX_MAX_FILENAME 256

// Límites de recursos
#define SANDBOX_CPU_LIMIT_SECONDS 30            // 30 segundos máximo de CPU
#define SANDBOX_FILE_SIZE_LIMIT (1024 * 1024)   // 1MB máximo por archivo
#define SANDBOX_MAX_FDS 32                      // 32 file descriptors máximo
#define SANDBOX_MEMORY_LIMIT (64 * 1024 * 1024) // 64MB máximo de memoria

// Códigos de resultado del sandbox
typedef enum
{
    SANDBOX_SUCCESS = 0,
    SANDBOX_ERROR_INVALID_PATH = 1,
    SANDBOX_ERROR_PLUGIN_NOT_FOUND = 2,
    SANDBOX_ERROR_LOAD_FAILED = 3,
    SANDBOX_ERROR_SYMBOL_NOT_FOUND = 4,
    SANDBOX_ERROR_EXECUTION_FAILED = 5,
    SANDBOX_ERROR_TIMEOUT = 6,
    SANDBOX_ERROR_RESOURCE_LIMIT = 7,
    SANDBOX_ERROR_PERMISSION_DENIED = 8,
    SANDBOX_ERROR_SETUP_FAILED = 9
} sandbox_result_t;

// Modos de aislamiento
typedef enum
{
    SANDBOX_MODE_A = 0, // Modo completo con chroot
    SANDBOX_MODE_B = 1  // Modo fallback sin chroot
} sandbox_mode_t;

// Configuración del sandbox
typedef struct
{
    sandbox_mode_t mode;                       // Modo de aislamiento
    char plugins_dir[SANDBOX_MAX_PATH_LENGTH]; // Directorio de plugins
    char work_dir[SANDBOX_MAX_PATH_LENGTH];    // Directorio de trabajo
    char log_file[SANDBOX_MAX_PATH_LENGTH];    // Archivo de log
    uid_t sandbox_uid;                         // UID para el sandbox (modo A)
    gid_t sandbox_gid;                         // GID para el sandbox (modo A)
    int cpu_limit;                             // Límite de CPU en segundos
    size_t file_size_limit;                    // Límite de tamaño de archivos
    int max_fds;                               // Máximo número de FDs
    size_t memory_limit;                       // Límite de memoria
} sandbox_config_t;

// Información de ejecución
typedef struct
{
    pid_t pid;                                 // PID del proceso hijo
    int exit_code;                             // Código de salida
    time_t start_time;                         // Tiempo de inicio
    time_t end_time;                           // Tiempo de finalización
    size_t stdout_size;                        // Tamaño de la salida estándar
    size_t stderr_size;                        // Tamaño de la salida de error
    char stdout_data[SANDBOX_MAX_OUTPUT_SIZE]; // Datos de stdout
    char stderr_data[SANDBOX_MAX_OUTPUT_SIZE]; // Datos de stderr
    sandbox_result_t result;                   // Resultado de la ejecución
} sandbox_execution_t;

// Información de un plugin
typedef struct
{
    char name[SANDBOX_MAX_FILENAME];    // Nombre del plugin
    char path[SANDBOX_MAX_PATH_LENGTH]; // Ruta completa
    char description[256];              // Descripción
    time_t last_modified;               // Última modificación
    size_t file_size;                   // Tamaño del archivo
    int is_valid;                       // Flag de validez
} plugin_info_t;

// Funciones principales del sandbox
int sandbox_init(sandbox_config_t* config);
int sandbox_execute_plugin(const char* plugin_name, const char* input_file, sandbox_execution_t* execution);
void sandbox_cleanup(void);

// Funciones de configuración
int sandbox_set_mode(sandbox_mode_t mode);
sandbox_mode_t sandbox_get_mode(void);
int sandbox_detect_capabilities(void);

// Funciones de validación y seguridad
int sandbox_validate_plugin_path(const char* plugin_path);
int sandbox_validate_input_path(const char* input_path);
int sandbox_setup_environment(void);
int sandbox_setup_limits(void);
int sandbox_setup_isolation(sandbox_mode_t mode);

// Funciones de gestión de plugins
int sandbox_list_plugins(plugin_info_t plugins[], int max_plugins);
int sandbox_verify_plugin(const char* plugin_path);
int sandbox_load_plugin_info(const char* plugin_path, plugin_info_t* info);

// Funciones de logging
int sandbox_log_execution(const char* plugin_name, const sandbox_execution_t* execution);
int sandbox_log_error(const char* plugin_name, sandbox_result_t error, const char* message);

// Funciones utilitarias
const char* sandbox_result_to_string(sandbox_result_t result);
const char* sandbox_mode_to_string(sandbox_mode_t mode);
void sandbox_dump_config(const sandbox_config_t* config);
void sandbox_dump_execution(const sandbox_execution_t* execution);

// Funciones de dlopen wrapper (para testing y abstracción)
void* sandbox_dlopen(const char* filename);
void* sandbox_dlsym(void* handle, const char* symbol);
int sandbox_dlclose(void* handle);

// Función específica para el plugin analizador
typedef int (*analizar_datos_func_t)(const char* archivo_entrada);

#endif // SANDBOX_H
