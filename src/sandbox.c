/**
 * @file sandbox.c
 * @brief Implementación del sistema de sandbox para ejecución controlada
 */

#include "sandbox.h"
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Configuración global del sandbox
static sandbox_config_t g_sandbox_config;
static int g_sandbox_initialized = 0;

/**
 * @brief Crea directorio si no existe
 */
static int ensure_directory_exists(const char* path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0755) == -1)
        {
            perror("mkdir");
            return -1;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Path exists but is not a directory: %s\n", path);
        return -1;
    }

    return 0;
}

/**
 * @brief Detecta las capacidades del sistema para el sandbox
 */
int sandbox_detect_capabilities(void)
{
    sandbox_mode_t mode = SANDBOX_MODE_B; // Por defecto modo fallback

    // Verificar si podemos usar chroot (necesitamos root)
    if (getuid() == 0)
    {
        // Verificar si el directorio de trabajo existe y podemos crear chroot
        if (ensure_directory_exists(SANDBOX_WORK_DIR) == 0)
        {
            mode = SANDBOX_MODE_A;
        }
    }

    printf("Sandbox capabilities detected: %s\n", sandbox_mode_to_string(mode));

    return mode;
}

/**
 * @brief Inicializa el sistema de sandbox
 */
int sandbox_init(sandbox_config_t* config)
{
    if (g_sandbox_initialized)
    {
        return 0; // Ya inicializado
    }

    // Usar configuración proporcionada o crear una por defecto
    if (config)
    {
        g_sandbox_config = *config;
    }
    else
    {
        // Configuración por defecto
        g_sandbox_config.mode = sandbox_detect_capabilities();
        strncpy(g_sandbox_config.plugins_dir, SANDBOX_PLUGINS_DIR, sizeof(g_sandbox_config.plugins_dir) - 1);
        strncpy(g_sandbox_config.work_dir, SANDBOX_WORK_DIR, sizeof(g_sandbox_config.work_dir) - 1);
        strncpy(g_sandbox_config.log_file, SANDBOX_LOG_FILE, sizeof(g_sandbox_config.log_file) - 1);

        // Buscar usuario sin privilegios para modo A
        struct passwd* nobody = getpwnam("nobody");
        if (nobody)
        {
            g_sandbox_config.sandbox_uid = nobody->pw_uid;
            g_sandbox_config.sandbox_gid = nobody->pw_gid;
        }
        else
        {
            g_sandbox_config.sandbox_uid = 65534; // nobody
            g_sandbox_config.sandbox_gid = 65534; // nogroup
        }

        g_sandbox_config.cpu_limit = SANDBOX_CPU_LIMIT_SECONDS;
        g_sandbox_config.file_size_limit = SANDBOX_FILE_SIZE_LIMIT;
        g_sandbox_config.max_fds = SANDBOX_MAX_FDS;
        g_sandbox_config.memory_limit = SANDBOX_MEMORY_LIMIT;
    }

    // Crear directorios necesarios
    if (ensure_directory_exists(g_sandbox_config.plugins_dir) != 0)
    {
        fprintf(stderr, "Failed to create plugins directory\n");
        return -1;
    }

    if (ensure_directory_exists(g_sandbox_config.work_dir) != 0)
    {
        fprintf(stderr, "Failed to create work directory\n");
        return -1;
    }

    if (ensure_directory_exists(SANDBOX_LOGS_DIR) != 0)
    {
        fprintf(stderr, "Failed to create logs directory\n");
        return -1;
    }

    g_sandbox_initialized = 1;

    printf("Sandbox initialized in %s\n", sandbox_mode_to_string(g_sandbox_config.mode));

    return 0;
}

/**
 * @brief Valida que una ruta de plugin sea segura
 */
int sandbox_validate_plugin_path(const char* plugin_path)
{
    if (!plugin_path)
        return -1;

    // Resolver ruta absoluta
    char resolved_path[SANDBOX_MAX_PATH_LENGTH];
    if (!realpath(plugin_path, resolved_path))
    {
        return -1; // Archivo no existe o no se puede resolver
    }

    // Verificar que está dentro del directorio de plugins
    if (strncmp(resolved_path, g_sandbox_config.plugins_dir, strlen(g_sandbox_config.plugins_dir)) != 0)
    {
        fprintf(stderr, "Plugin path outside allowed directory: %s\n", resolved_path);
        return -1;
    }

    // Verificar que es archivo regular
    struct stat st;
    if (stat(resolved_path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Valida que una ruta de entrada sea segura
 */
int sandbox_validate_input_path(const char* input_path)
{
    if (!input_path)
        return -1;

    // Resolver ruta absoluta
    char resolved_path[SANDBOX_MAX_PATH_LENGTH];
    if (!realpath(input_path, resolved_path))
    {
        return -1; // Archivo no existe
    }

    // Verificar que está en área de trabajo del sandbox
    if (strncmp(resolved_path, g_sandbox_config.work_dir, strlen(g_sandbox_config.work_dir)) != 0)
    {
        fprintf(stderr, "Input path outside sandbox work directory: %s\n", resolved_path);
        return -1;
    }

    // Verificar que es archivo regular
    struct stat st;
    if (stat(resolved_path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Configura límites de recursos usando setrlimit
 */
int sandbox_setup_limits(void)
{
    struct rlimit limit;

    // Límite de CPU
    limit.rlim_cur = (rlim_t)g_sandbox_config.cpu_limit;
    limit.rlim_max = (rlim_t)g_sandbox_config.cpu_limit;
    if (setrlimit(RLIMIT_CPU, &limit) != 0)
    {
        perror("setrlimit CPU");
        return -1;
    }

    // Límite de tamaño de archivos
    limit.rlim_cur = (rlim_t)g_sandbox_config.file_size_limit;
    limit.rlim_max = (rlim_t)g_sandbox_config.file_size_limit;
    if (setrlimit(RLIMIT_FSIZE, &limit) != 0)
    {
        perror("setrlimit FSIZE");
        return -1;
    }

    // Límite de file descriptors
    limit.rlim_cur = (rlim_t)g_sandbox_config.max_fds;
    limit.rlim_max = (rlim_t)g_sandbox_config.max_fds;
    if (setrlimit(RLIMIT_NOFILE, &limit) != 0)
    {
        perror("setrlimit NOFILE");
        return -1;
    }

    // Límite de memoria (solo en sistemas que lo soportan)
    limit.rlim_cur = (rlim_t)g_sandbox_config.memory_limit;
    limit.rlim_max = (rlim_t)g_sandbox_config.memory_limit;
    setrlimit(RLIMIT_AS, &limit); // Ignorar errores, no todos los sistemas lo soportan

    return 0;
}

/**
 * @brief Limpia variables de entorno peligrosas
 */
static void clean_environment(void)
{
    // Mantener solo variables esenciales
    const char* safe_vars[] = {"PATH=/usr/bin:/bin", "LANG=C", "LC_ALL=C", NULL};

    // Limpiar todas las variables de entorno
    clearenv();

    // Establecer variables seguras
    for (int i = 0; safe_vars[i] != NULL; i++)
    {
        putenv((char*)safe_vars[i]);
    }
}

/**
 * @brief Configura aislamiento según el modo
 */
int sandbox_setup_isolation(sandbox_mode_t mode)
{
    if (mode == SANDBOX_MODE_A)
    {
        // Modo A: chroot + cambio de usuario
        if (chroot(g_sandbox_config.work_dir) != 0)
        {
            perror("chroot");
            return -1;
        }

        if (chdir("/") != 0)
        {
            perror("chdir");
            return -1;
        }

        // Cambiar grupo primero
        if (setgid(g_sandbox_config.sandbox_gid) != 0)
        {
            perror("setgid");
            return -1;
        }

        // Cambiar usuario
        if (setuid(g_sandbox_config.sandbox_uid) != 0)
        {
            perror("setuid");
            return -1;
        }
    }
    else
    {
        // Modo B: solo cambio de directorio de trabajo
        if (chdir(g_sandbox_config.work_dir) != 0)
        {
            perror("chdir to sandbox work directory");
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Configura el entorno del sandbox
 */
int sandbox_setup_environment(void)
{
    // Limpiar entorno
    clean_environment();

    // Configurar límites de recursos
    if (sandbox_setup_limits() != 0)
    {
        return -1;
    }

    // Cerrar file descriptors no esenciales (mantener stdin, stdout, stderr)
    for (int fd = 3; fd < g_sandbox_config.max_fds; fd++)
    {
        close(fd);
    }

    return 0;
}

/**
 * @brief Wrapper para dlopen con validación de seguridad
 */
void* sandbox_dlopen(const char* filename)
{
    if (!filename)
        return NULL;

    // Validar que el archivo esté en el directorio de plugins
    char full_path[SANDBOX_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", g_sandbox_config.plugins_dir, filename);

    if (sandbox_validate_plugin_path(full_path) != 0)
    {
        return NULL;
    }

    return dlopen(full_path, RTLD_LAZY);
}

/**
 * @brief Wrapper para dlsym
 */
void* sandbox_dlsym(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

/**
 * @brief Wrapper para dlclose
 */
int sandbox_dlclose(void* handle)
{
    return dlclose(handle);
}

/**
 * @brief Registra eventos del sandbox en el log
 */
int sandbox_log_execution(const char* plugin_name, const sandbox_execution_t* execution)
{
    FILE* log_file = fopen(g_sandbox_config.log_file, "a");
    if (!log_file)
    {
        perror("Cannot open sandbox log file");
        return -1;
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] EXEC plugin=%s result=%s exit_code=%d duration=%ld stdout_size=%zu stderr_size=%zu\n",
            timestamp, plugin_name, sandbox_result_to_string(execution->result), execution->exit_code,
            execution->end_time - execution->start_time, execution->stdout_size, execution->stderr_size);

    fclose(log_file);
    return 0;
}

/**
 * @brief Registra errores del sandbox
 */
int sandbox_log_error(const char* plugin_name, sandbox_result_t error, const char* message)
{
    FILE* log_file = fopen(g_sandbox_config.log_file, "a");
    if (!log_file)
    {
        perror("Cannot open sandbox log file");
        return -1;
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] ERROR plugin=%s error=%s message=\"%s\"\n", timestamp,
            plugin_name ? plugin_name : "unknown", sandbox_result_to_string(error), message ? message : "");

    fclose(log_file);
    return 0;
}

/**
 * @brief Ejecuta un plugin en el sandbox
 */
int sandbox_execute_plugin(const char* plugin_name, const char* input_file, sandbox_execution_t* execution)
{
    if (!g_sandbox_initialized || !plugin_name || !execution)
    {
        return -1;
    }

    memset(execution, 0, sizeof(sandbox_execution_t));
    execution->result = SANDBOX_ERROR_SETUP_FAILED;
    execution->start_time = time(NULL);

    // Construir ruta completa del plugin
    char plugin_path[SANDBOX_MAX_PATH_LENGTH];
    snprintf(plugin_path, sizeof(plugin_path), "%s%s", g_sandbox_config.plugins_dir, plugin_name);

    // Validar plugin
    if (sandbox_validate_plugin_path(plugin_path) != 0)
    {
        execution->result = SANDBOX_ERROR_INVALID_PATH;
        sandbox_log_error(plugin_name, execution->result, "Invalid plugin path");
        return -1;
    }

    // Validar archivo de entrada si se proporciona
    if (input_file && sandbox_validate_input_path(input_file) != 0)
    {
        execution->result = SANDBOX_ERROR_INVALID_PATH;
        sandbox_log_error(plugin_name, execution->result, "Invalid input file path");
        return -1;
    }

    // Crear pipes para capturar stdout y stderr
    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
    {
        execution->result = SANDBOX_ERROR_SETUP_FAILED;
        sandbox_log_error(plugin_name, execution->result, "Failed to create pipes");
        return -1;
    }

    // Fork proceso hijo para ejecutar el plugin
    pid_t pid = fork();
    if (pid == -1)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        execution->result = SANDBOX_ERROR_SETUP_FAILED;
        sandbox_log_error(plugin_name, execution->result, "Fork failed");
        return -1;
    }

    if (pid == 0)
    {
        // Proceso hijo - configurar sandbox y ejecutar plugin

        // Redirigir stdout y stderr a pipes
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        // Cerrar extremos no usados de los pipes
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        // Configurar entorno del sandbox
        if (sandbox_setup_environment() != 0)
        {
            fprintf(stderr, "Failed to setup sandbox environment\n");
            exit(1);
        }

        // Configurar aislamiento
        if (sandbox_setup_isolation(g_sandbox_config.mode) != 0)
        {
            fprintf(stderr, "Failed to setup sandbox isolation\n");
            exit(1);
        }

        // Cargar plugin dinámicamente
        void* handle = sandbox_dlopen(plugin_name);
        if (!handle)
        {
            fprintf(stderr, "Failed to load plugin: %s\n", dlerror());
            exit(1);
        }

        // Buscar símbolo analizar_datos usando union para evitar warning de pedantic
        union {
            void* obj;
            analizar_datos_func_t func;
        } cast_helper;

        cast_helper.obj = sandbox_dlsym(handle, "analizar_datos");
        analizar_datos_func_t analizar_datos = cast_helper.func;

        if (!analizar_datos)
        {
            fprintf(stderr, "Symbol 'analizar_datos' not found: %s\n", dlerror());
            sandbox_dlclose(handle);
            exit(1);
        }

        // Ejecutar función del plugin
        int result = analizar_datos(input_file);

        // FORZAR FLUSH de stdout y stderr antes de salir
        fflush(stdout);
        fflush(stderr);

        // Pequeña espera para asegurar que el buffer se vacía
        usleep(10000); // 10ms

        // Limpiar y salir
        sandbox_dlclose(handle);
        exit(result);
    }
    else
    {
        // Proceso padre - leer salida y esperar
        execution->pid = pid;

        // Cerrar extremos de escritura
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Leer stdout y stderr de forma SÍNCRONA
        char buffer[4096];
        ssize_t n;

        execution->stdout_size = 0;
        execution->stderr_size = 0;

        // Leer TODO el stdout disponible
        while ((n = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
        {
            if (execution->stdout_size + (size_t)n < sizeof(execution->stdout_data) - 1)
            {
                memcpy(execution->stdout_data + execution->stdout_size, buffer, (size_t)n);
                execution->stdout_size += (size_t)n;
            }
        }
        execution->stdout_data[execution->stdout_size] = '\0';

        // Leer TODO el stderr disponible
        while ((n = read(stderr_pipe[0], buffer, sizeof(buffer))) > 0)
        {
            if (execution->stderr_size + (size_t)n < sizeof(execution->stderr_data) - 1)
            {
                memcpy(execution->stderr_data + execution->stderr_size, buffer, (size_t)n);
                execution->stderr_size += (size_t)n;
            }
        }
        execution->stderr_data[execution->stderr_size] = '\0';

        // Cerrar pipes
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        // Esperar a que termine el proceso hijo
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            execution->result = SANDBOX_ERROR_EXECUTION_FAILED;
            sandbox_log_error(plugin_name, execution->result, "waitpid failed");
            return -1;
        }

        execution->end_time = time(NULL);

        if (WIFEXITED(status))
        {
            execution->exit_code = WEXITSTATUS(status);
            execution->result = (execution->exit_code == 0) ? SANDBOX_SUCCESS : SANDBOX_ERROR_EXECUTION_FAILED;
        }
        else if (WIFSIGNALED(status))
        {
            execution->exit_code = -WTERMSIG(status);
            execution->result = SANDBOX_ERROR_TIMEOUT;
        }
        else
        {
            execution->result = SANDBOX_ERROR_EXECUTION_FAILED;
        }
    }
    // Registrar ejecución
    sandbox_log_execution(plugin_name, execution);

    return (execution->result == SANDBOX_SUCCESS) ? 0 : -1;
}

/**
 * @brief Lista plugins disponibles
 */
int sandbox_list_plugins(plugin_info_t plugins[], int max_plugins)
{
    // Auto-inicializar si es necesario
    const char* plugins_dir = g_sandbox_initialized ? g_sandbox_config.plugins_dir : SANDBOX_PLUGINS_DIR;

    DIR* dir = opendir(plugins_dir);
    if (!dir)
    {
        perror("Cannot open plugins directory");
        return -1;
    }

    struct dirent* entry;
    int plugin_count = 0;

    while ((entry = readdir(dir)) != NULL && plugin_count < max_plugins)
    {
        if (entry->d_name[0] == '.')
        {
            continue; // Ignorar archivos ocultos
        }

        char full_path[SANDBOX_MAX_PATH_LENGTH];
        int written = snprintf(full_path, sizeof(full_path), "%s%s", plugins_dir, entry->d_name);

        // Verificar truncamiento
        if (written < 0 || (size_t)written >= sizeof(full_path))
        {
            fprintf(stderr, "Warning: path too long for %s, skipping\n", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
        {
            strncpy(plugins[plugin_count].name, entry->d_name, sizeof(plugins[plugin_count].name) - 1);
            strncpy(plugins[plugin_count].path, full_path, sizeof(plugins[plugin_count].path) - 1);

            plugins[plugin_count].last_modified = st.st_mtime;
            plugins[plugin_count].file_size = (size_t)st.st_size;
            plugins[plugin_count].is_valid = 1;
            strncpy(plugins[plugin_count].description, "C++ Plugin", sizeof(plugins[plugin_count].description) - 1);

            plugin_count++;
        }
    }

    closedir(dir);
    return plugin_count;
}

/**
 * @brief Convierte resultado a string
 */
const char* sandbox_result_to_string(sandbox_result_t result)
{
    switch (result)
    {
    case SANDBOX_SUCCESS:
        return "SUCCESS";
    case SANDBOX_ERROR_INVALID_PATH:
        return "INVALID_PATH";
    case SANDBOX_ERROR_PLUGIN_NOT_FOUND:
        return "PLUGIN_NOT_FOUND";
    case SANDBOX_ERROR_LOAD_FAILED:
        return "LOAD_FAILED";
    case SANDBOX_ERROR_SYMBOL_NOT_FOUND:
        return "SYMBOL_NOT_FOUND";
    case SANDBOX_ERROR_EXECUTION_FAILED:
        return "EXECUTION_FAILED";
    case SANDBOX_ERROR_TIMEOUT:
        return "TIMEOUT";
    case SANDBOX_ERROR_RESOURCE_LIMIT:
        return "RESOURCE_LIMIT";
    case SANDBOX_ERROR_PERMISSION_DENIED:
        return "PERMISSION_DENIED";
    case SANDBOX_ERROR_SETUP_FAILED:
        return "SETUP_FAILED";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convierte modo a string
 */
const char* sandbox_mode_to_string(sandbox_mode_t mode)
{
    switch (mode)
    {
    case SANDBOX_MODE_A:
        return "Mode A (Full isolation with chroot)";
    case SANDBOX_MODE_B:
        return "Mode B (Basic isolation)";
    default:
        return "Unknown mode";
    }
}

/**
 * @brief Establece modo del sandbox
 */
int sandbox_set_mode(sandbox_mode_t mode)
{
    if (!g_sandbox_initialized)
        return -1;

    g_sandbox_config.mode = mode;
    return 0;
}

/**
 * @brief Obtiene modo actual
 */
sandbox_mode_t sandbox_get_mode(void)
{
    return g_sandbox_config.mode;
}

/**
 * @brief Muestra configuración del sandbox
 */
void sandbox_dump_config(const sandbox_config_t* config)
{
    if (!config)
        config = &g_sandbox_config;

    printf("\n=== SANDBOX CONFIGURATION ===\n");
    printf("Mode: %s\n", sandbox_mode_to_string(config->mode));
    printf("Plugins directory: %s\n", config->plugins_dir);
    printf("Work directory: %s\n", config->work_dir);
    printf("Log file: %s\n", config->log_file);
    printf("Sandbox UID: %u\n", config->sandbox_uid);
    printf("Sandbox GID: %u\n", config->sandbox_gid);
    printf("CPU limit: %d seconds\n", config->cpu_limit);
    printf("File size limit: %zu bytes\n", config->file_size_limit);
    printf("Max FDs: %d\n", config->max_fds);
    printf("Memory limit: %zu bytes\n", config->memory_limit);
    printf("=============================\n\n");
}

/**
 * @brief Muestra información de ejecución
 */
void sandbox_dump_execution(const sandbox_execution_t* execution)
{
    if (!execution)
        return;

    printf("\n=== EXECUTION RESULT ===\n");
    printf("PID: %d\n", execution->pid);
    printf("Result: %s\n", sandbox_result_to_string(execution->result));
    printf("Exit code: %d\n", execution->exit_code);
    printf("Duration: %ld seconds\n", execution->end_time - execution->start_time);
    printf("Stdout size: %zu bytes\n", execution->stdout_size);
    printf("Stderr size: %zu bytes\n", execution->stderr_size);

    if (execution->stdout_size > 0)
    {
        printf("Stdout:\n%s\n", execution->stdout_data);
    }

    if (execution->stderr_size > 0)
    {
        printf("Stderr:\n%s\n", execution->stderr_data);
    }

    printf("========================\n\n");
}

/**
 * @brief Limpia el sistema de sandbox
 */
void sandbox_cleanup(void)
{
    if (g_sandbox_initialized)
    {
        memset(&g_sandbox_config, 0, sizeof(sandbox_config_t));
        g_sandbox_initialized = 0;
        printf("Sandbox system cleaned up\n");
    }
}
