#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

int cmd_status(shell_context_t* ctx) {
    (void)ctx;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Error en fork");
        return -1;
    }
    
    if (pid == 0) {
        // Proceso hijo: leer último archivo de métricas
        DIR* dir = opendir(DEFAULT_LOG_DIR);
        if (!dir) {
            perror("Error abriendo directorio de métricas");
            exit(1);
        }
        
        struct dirent* entry;
        char latest_file[MAX_FILENAME_LENGTH] = {0};
        
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, "metrics-") != NULL) {
                if (strcmp(entry->d_name, latest_file) > 0) {
                    strncpy(latest_file, entry->d_name, sizeof(latest_file) - 1);
                }
            }
        }
        closedir(dir);
        
        if (strlen(latest_file) == 0) {
            printf("No hay métricas disponibles\n");
            exit(0);
        }
        
        char filepath[MAX_PATH_LENGTH];
        snprintf(filepath, sizeof(filepath), "%s%s", DEFAULT_LOG_DIR, latest_file);
        
        // Usar tail para mostrar últimas líneas
        execlp("tail", "tail", "-n", "5", filepath, (char*)NULL);
        perror("Error en exec");
        exit(1);
    }
    
    // Proceso padre: esperar al hijo
    int status;
    waitpid(pid, &status, 0);
    
    return 0;
}

int cmd_start(shell_context_t* ctx) {
    pthread_mutex_lock(&ctx->state_mutex);
    
    if (ctx->state == MONITOR_RUNNING) {
        printf("El monitoreo ya está activo\n");
        pthread_mutex_unlock(&ctx->state_mutex);
        return 0;
    }
    
    // Crear thread de monitoreo
    if (pthread_create(&ctx->monitor_thread, NULL, monitor_thread_func, ctx) != 0) {
        perror("Error creando thread de monitoreo");
        pthread_mutex_unlock(&ctx->state_mutex);
        return -1;
    }
    
    ctx->state = MONITOR_RUNNING;
    printf("Monitoreo iniciado\n");
    
    pthread_mutex_unlock(&ctx->state_mutex);
    return 0;
}

int cmd_stop(shell_context_t* ctx) {
    pthread_mutex_lock(&ctx->state_mutex);
    
    if (ctx->state == MONITOR_STOPPED) {
        printf("El monitoreo ya está detenido\n");
        pthread_mutex_unlock(&ctx->state_mutex);
        return 0;
    }
    
    ctx->state = MONITOR_STOPPED;
    pthread_mutex_unlock(&ctx->state_mutex);
    
    // Enviar señal de stop por pipe
    char msg = 'S';
    write(ctx->pipe_fd[1], &msg, 1);
    
    // Esperar a que termine el thread
    pthread_join(ctx->monitor_thread, NULL);
    
    printf("Monitoreo detenido\n");
    
    return 0;
}

int cmd_psnode(shell_context_t* ctx) {
    (void)ctx;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Error en fork");
        return -1;
    }
    
    if (pid == 0) {
        // Proceso hijo: ejecutar ps filtrado
        printf("Procesos relacionados con el nodo:\n");
        execlp("sh", "sh", "-c", "ps aux | grep -E 'monitoreo|monitoring' | grep -v grep", (char*)NULL);
        perror("Error en exec");
        exit(1);
    }
    
    // Proceso padre: esperar al hijo
    int status;
    waitpid(pid, &status, 0);
    
    return 0;
}

int cmd_exit(shell_context_t* ctx) {
    (void)ctx;
    printf("Saliendo del shell...\n");
    return 0;
}