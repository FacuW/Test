#ifndef SHELL_H
#define SHELL_H

#include "monitoring.h"
#include <pthread.h>

#define SHELL_PROMPT "monitoring> "
// ← ELIMINADO: #define MAX_COMMAND_LENGTH 256 (ya está en monitoring.h)
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

// Comandos del shell
int cmd_status(shell_context_t* ctx);
int cmd_start(shell_context_t* ctx);
int cmd_stop(shell_context_t* ctx);
int cmd_psnode(shell_context_t* ctx);
int cmd_exit(shell_context_t* ctx);

// Logging
int shell_log_command(const char* command);

// Thread de monitoreo
void* monitor_thread_func(void* arg);

#endif
