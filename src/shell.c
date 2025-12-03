#include "shell.h"
#include <ctype.h>
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
    if (stat("/var/log/monitoreo", &st) == -1)
    {
        mkdir("/var/log/monitoreo", DIR_PERMISSIONS);
        if (stat("/var/log/monitoreo", &st) == -1)
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

// Función auxiliar para extraer el primer token (comando)
static void get_first_token(const char* str, char* token, size_t max_len)
{
    size_t i = 0;
    while (i < max_len - 1 && str[i] != '\0' && !isspace((unsigned char)str[i]))
    {
        token[i] = str[i];
        i++;
    }
    token[i] = '\0';
}

int shell_run(shell_context_t* ctx)
{
    char command[MAX_COMMAND_LENGTH];

    printf("\n=== Shell de Monitoreo ===\n");
    printf("Comandos internos: status, start, stop, psnode, exit\n");
    printf("Comandos externos: ls, cat, grep, ps, etc.\n");
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

        // Ejecutar comandos INTERNOS
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
        // Ejecutar comandos EXTERNOS
        else if (strchr(command, '|') != NULL)
        {
            // Detectar pipe y ejecutar pipeline
            parse_and_execute_pipeline(ctx, command);
        }
        else if (strchr(command, '>') != NULL)
        {
            // Detectar redirección y ejecutar
            execute_with_redirection(ctx, command);
        }
        else
        {
            // Intentar ejecutar como comando externo
            // Extraer primer token para mejor manejo de errores
            char first_token[64];
            get_first_token(command, first_token, sizeof(first_token));

            int result = cmd_exec(ctx, command);

            // Solo mostrar error si el comando realmente falló
            if (result != 0)
            {
                printf("Comando desconocido o no permitido: %s\n", first_token);
                printf("Comandos internos: status, start, stop, psnode, exit\n");
                printf("Use 'exec' para ver comandos externos permitidos\n");
            }
        }
    }

    return 0;
}

void shell_cleanup(shell_context_t* ctx)
{
    if (!ctx)
        return;

    // IMPORTANTE: Si hay un thread activo, detenerlo primero
    pthread_mutex_lock(&ctx->state_mutex);
    if (ctx->state == MONITOR_RUNNING)
    {
        ctx->state = MONITOR_STOPPED;
        pthread_mutex_unlock(&ctx->state_mutex);

        // Enviar señal de stop
        char msg = 'S';
        write(ctx->pipe_fd[1], &msg, 1);

        // Esperar a que termine
        pthread_join(ctx->monitor_thread, NULL);
    }
    else
    {
        pthread_mutex_unlock(&ctx->state_mutex);
    }

    // Cerrar pipes
    if (ctx->pipe_fd[0] > 0)
        close(ctx->pipe_fd[0]);
    if (ctx->pipe_fd[1] > 0)
        close(ctx->pipe_fd[1]);

    // Destruir mutex
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
