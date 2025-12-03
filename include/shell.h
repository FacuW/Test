#ifndef SHELL_H
#define SHELL_H

#include "monitoring.h"
#include "memory_manager.h"
#include "storage.h"
#include "sandbox.h"
#include <pthread.h>

#define SHELL_PROMPT "monitoring> "
// MAX_COMMAND_LENGTH ya está definido en monitoring.h
#define SHELL_LOG_PATH "/var/log/monitoreo/shell.log"

// Estados del sistema de monitoreo
typedef enum
{
    MONITOR_STOPPED,
    MONITOR_RUNNING
} monitor_state_t;

// Estructura de control del shell
typedef struct
{
    monitor_state_t state;
    pthread_mutex_t state_mutex;
    int pipe_fd[2]; // IPC con proceso de monitoreo
    pthread_t monitor_thread;
} shell_context_t;

// Funciones principales del shell
int shell_init(shell_context_t* ctx);
int shell_run(shell_context_t* ctx);
void shell_cleanup(shell_context_t* ctx);

// Comandos del shell (internos)
int cmd_status(shell_context_t* ctx);
int cmd_start(shell_context_t* ctx);
int cmd_stop(shell_context_t* ctx);
int cmd_psnode(shell_context_t* ctx);
int cmd_exit(shell_context_t* ctx);

// Comandos externos (NUEVO)
int cmd_exec(shell_context_t* ctx, const char* command);
int parse_and_execute_pipeline(shell_context_t* ctx, const char* command_line);
int execute_with_redirection(shell_context_t* ctx, const char* command);

// === NUEVOS COMANDOS TP3 ===

// Comandos de gestión de memoria
int cmd_mem_init(shell_context_t* ctx, const char* strategy);
int cmd_mem_alloc(shell_context_t* ctx, const char* size_str);
int cmd_mem_free(shell_context_t* ctx, const char* ptr_str);
int cmd_mem_dump(shell_context_t* ctx);
int cmd_mem_stats(shell_context_t* ctx);
int cmd_mem_test(shell_context_t* ctx, const char* test_type);
int cmd_mem_cleanup(shell_context_t* ctx);

// Comandos de almacenamiento persistente
int cmd_storage_init(shell_context_t* ctx);
int cmd_storage_write(shell_context_t* ctx);
int cmd_storage_read(shell_context_t* ctx, const char* date_str);
int cmd_storage_list(shell_context_t* ctx);
int cmd_storage_stats(shell_context_t* ctx);
int cmd_storage_validate(shell_context_t* ctx, const char* filename);

// Comandos de sandbox
int cmd_sandbox_init(shell_context_t* ctx, const char* mode);
int cmd_sandbox_list(shell_context_t* ctx);
int cmd_sandbox_run(shell_context_t* ctx, const char* plugin_name, const char* input_file);
int cmd_sandbox_config(shell_context_t* ctx);
int cmd_sandbox_test(shell_context_t* ctx);

// Comando integrado de demostración
int cmd_demo(shell_context_t* ctx, const char* demo_type);

// Logging
int shell_log_command(const char* command);

// Thread de monitoreo
void* monitor_thread_func(void* arg);

#endif