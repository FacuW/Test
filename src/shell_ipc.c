#include "shell.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

void* monitor_thread_func(void* arg) {
    shell_context_t* ctx = (shell_context_t*)arg;
    system_metrics_t metrics;
    char filename[MAX_FILENAME_LENGTH];
    
    while (1) {
        pthread_mutex_lock(&ctx->state_mutex);
        monitor_state_t state = ctx->state;
        pthread_mutex_unlock(&ctx->state_mutex);
        
        if (state == MONITOR_STOPPED) {
            break;
        }
        
        // Recolectar métricas
        if (collect_metrics(&metrics) == 0) {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            
            snprintf(filename, sizeof(filename), "%smetrics-%04d%02d%02d.log",
                    DEFAULT_LOG_DIR,
                    tm_info->tm_year + 1900,
                    tm_info->tm_mon + 1,
                    tm_info->tm_mday);
            
            save_metrics_to_json(&metrics, filename);
        }
        
        // Usar select para timeout interruptible
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(ctx->pipe_fd[0], &readfds);
        
        tv.tv_sec = DEFAULT_INTERVAL;
        tv.tv_usec = 0;
        
        int ret = select(ctx->pipe_fd[0] + 1, &readfds, NULL, NULL, &tv);
        
        if (ret > 0) {
            // Hay datos en el pipe (señal de stop)
            char msg;
            read(ctx->pipe_fd[0], &msg, 1);
            break;
        }
    }
    
    return NULL;
}