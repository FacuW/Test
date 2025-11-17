#include "shell.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t shell_running = 1;

void shell_signal_handler(int sig)
{
    (void)sig;
    printf("\nUse 'exit' para salir del shell\n");
    printf(SHELL_PROMPT);
    fflush(stdout);
}

int shell_init(shell_context_t* ctx)
{
    if (!ctx)
        return -1;

    // Inicializar estado
    ctx->state = MONITOR_STOPPED;

    // Inicializar mutex
    if (pthread_mutex_init(&ctx->state_mutex, NULL) != 0)
    {
        perror("Error inicializando mutex");
        return -1;
    }

    // Crear pipe para IPC
    if (pipe(ctx->pipe_fd) == -1)
    {
        perror("Error creando pipe");
        pthread_mutex_destroy(&ctx->state_mutex);
        return -1;
    }

    // Crear directorio de logs si no existe
    struct stat st = {0};
    if (stat("SHELL_LOG_DIR", &st) == -1)
    {
        mkdir("SHELL_LOG_DIR", DIR_PERMISSIONS);
        if (stat("SHELL_LOG_DIR", &st) == -1)
        {
            fprintf(stderr, "Warning: No se pudo crear /var/log/monitoreo, logging deshabilitado\n");
        }
    }

    // Configurar manejador de señales
    signal(SIGINT, shell_signal_handler);

    printf("Shell de monitoreo inicializado\n");
    shell_log_command("SHELL_INIT");

    return 0;
}

int shell_run(shell_context_t* ctx)
{
    char command[MAX_COMMAND_LENGTH];

    printf("\n=== Shell de Monitoreo ===\n");
    printf("Comandos internos: status, start, stop, psnode, exit\n");
    printf("Comandos externos: ls, cat, grep, ps, etc. (ver whitelist con 'exec')\n");
    printf("Soporte de pipes: ls | grep test\n\n");

    while (shell_running)
    {
        printf(SHELL_PROMPT);
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        // Eliminar newline
        command[strcspn(command, "\n")] = 0;

        // Ignorar líneas vacías
        if (strlen(command) == 0)
        {
            continue;
        }

        // Log del comando
        shell_log_command(command);

        // Ejecutar comando
        if (strcmp(command, "status") == 0)
        {
            cmd_status(ctx);
        }
        else if (strcmp(command, "start") == 0)
        {
            cmd_start(ctx);
        }
        else if (strcmp(command, "stop") == 0)
        {
            cmd_stop(ctx);
        }
        else if (strcmp(command, "psnode") == 0)
        {
            cmd_psnode(ctx);
        }
        else if (strcmp(command, "exit") == 0)
        {
            cmd_exit(ctx);
            break;
        }
        else if (strncmp(command, "exec ", 5) == 0)
        {
            // Comando exec explícito
            cmd_exec(ctx, command + 5);
        }
        else if (strchr(command, '|') != NULL)
        {
            // Detectar pipe y ejecutar pipeline
            parse_and_execute_pipeline(ctx, command);
        }
        else
        {
            // Intentar ejecutar como comando externo
            if (cmd_exec(ctx, command) != 0)
            {
                printf("Comando desconocido: %s\n", command);
                printf("Comandos disponibles: status, start, stop, psnode, exit, exec\n");
            }
        }
    }

    return 0;
}

void shell_cleanup(shell_context_t* ctx)
{
    if (!ctx)
        return;

    // Cerrar pipes
    if (ctx->pipe_fd[0] > 0)
        close(ctx->pipe_fd[0]);
    if (ctx->pipe_fd[1] > 0)
        close(ctx->pipe_fd[1]);

    // CRÍTICO: Asegurar que el mutex está libre antes de destruir
    pthread_mutex_lock(&ctx->state_mutex);
    pthread_mutex_unlock(&ctx->state_mutex);

    // Ahora sí destruir
    pthread_mutex_destroy(&ctx->state_mutex);

    printf("Shell terminado\n");
}

int shell_log_command(const char* command)
{
    FILE* log = fopen(SHELL_LOG_PATH, "a");
    if (!log)
    {
        return -1;
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log, "[%s] %s\n", timestamp, command);
    fclose(log);

    return 0;
}
