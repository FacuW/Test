/**
 * @file main.c
 * @brief Programa principal del sistema de monitoreo
 */

#include "cjson/cJSON.h"
#include "monitoring.h"
#include "shell.h"  // ← AGREGADO
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Flag para controlar la ejecución del programa
volatile sig_atomic_t running = 1;

/**
 * @brief Manejador de señales para terminar el programa de forma controlada
 */
void signal_handler(int sig)
{
    (void)sig; // Evitar warning de variable no usada
    running = 0;
}

/**
 * @brief Genera el nombre de archivo para las métricas
 */
void generate_filename(char* buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    snprintf(buffer, size, "%smetrics-%04d%02d%02d.log", DEFAULT_LOG_DIR, tm_info->tm_year + 1900, tm_info->tm_mon + 1,
             tm_info->tm_mday);
}

/**
 * @brief
 */
int main(int argc, char* argv[])
{
    int interval = DEFAULT_INTERVAL;
    int prometheus_port = DEFAULT_PROMETHEUS_PORT;
    int use_shell = 1;  // ← AGREGADO: por defecto usa shell

    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc)
        {
            interval = atoi(argv[i + 1]);
            if (interval < 1)
                interval = DEFAULT_INTERVAL;
            i++;
        }
        else if (strcmp(argv[i], "--prometheus-port") == 0 && i + 1 < argc)
        {
            prometheus_port = atoi(argv[i + 1]);
            if (prometheus_port < 1024 || prometheus_port > 65535)
                prometheus_port = DEFAULT_PROMETHEUS_PORT;
            i++;
        }
        // ← AGREGADO: opción para deshabilitar shell
        else if (strcmp(argv[i], "--no-shell") == 0)
        {
            use_shell = 0;
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            printf("Uso: %s [opciones]\n", argv[0]);
            printf("Opciones:\n");
            printf("  --interval SEGUNDOS       Intervalo de recolección de métricas (por defecto: %d)\n",
                   DEFAULT_INTERVAL);
            printf("  --prometheus-port PUERTO  Puerto para servidor Prometheus (por defecto: %d)\n",
                   DEFAULT_PROMETHEUS_PORT);
            printf("  --no-shell                Ejecutar sin shell interactivo\n");  // ← AGREGADO
            printf("  --help                     Mostrar esta ayuda\n");
            return EXIT_SUCCESS;
        }
    }

    printf("=== Sistema de Monitoreo Básico ===\n");
    printf("Versión %s\n\n", MONITORING_VERSION);

    // config manejador de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Inicializar el sistema
    if (init_monitoring_system() != 0)
    {
        fprintf(stderr, "Error al inicializar el sistema de monitoreo\n");
        return EXIT_FAILURE;
    }

    // Inicializar métricas de Prometheus
    if (init_prometheus_metrics() != 0)
    {
        fprintf(stderr, "Error al inicializar métricas de Prometheus\n");
        cleanup_monitoring_system();
        return EXIT_FAILURE;
    }

    // Iniciar servidor HTTP de Prometheus
    if (start_prometheus_server(prometheus_port) != 0)
    {
        fprintf(stderr, "Error al iniciar servidor de Prometheus\n");
        cleanup_prometheus();
        cleanup_monitoring_system();
        return EXIT_FAILURE;
    }

    printf("Sistema de monitoreo iniciado.\n");
    printf("Intervalo de recolección: %d segundos\n", interval);
    printf("Directorio de logs: %s\n", DEFAULT_LOG_DIR);
    printf("Puerto Prometheus: %d\n", prometheus_port);

    // ← BIFURCACIÓN: usar shell o modo automático
    if (use_shell)
    {
        // ===== CÓDIGO NUEVO: MODO SHELL =====
        printf("Modo: Shell interactivo\n");
        printf("Prometheus activo en puerto %d\n\n", prometheus_port);
        
        shell_context_t shell_ctx;
        if (shell_init(&shell_ctx) != 0)
        {
            fprintf(stderr, "Error al inicializar el shell\n");
            cleanup_prometheus();
            cleanup_monitoring_system();
            return EXIT_FAILURE;
        }
        
        // Ejecutar shell (bloqueante)
        shell_run(&shell_ctx);
        
        // Limpieza del shell
        shell_cleanup(&shell_ctx);
    }
    else
    {
        // ===== CÓDIGO ORIGINAL: MODO AUTOMÁTICO =====
        printf("Modo: Automático (sin shell)\n");
        printf("Presione Ctrl+C para detener\n\n");

        system_metrics_t metrics;
        char filename[MAX_FILENAME_LENGTH];

        while (running)
        {
            // Generar nombre de archivo para las métricas
            generate_filename(filename, sizeof(filename));

            // Recolectar métricas
            if (collect_metrics(&metrics) == 0)
            {
                // Guardar métricas en formato JSON (NDJSON)
                if (save_metrics_to_json(&metrics, filename) == 0)
                {
                    printf("Métricas recolectadas y guardadas en %s\n", filename);
                }
                else
                {
                    fprintf(stderr, "Error al guardar métricas\n");
                }

                // Actualizar métricas de Prometheus
                if (update_prometheus_metrics(&metrics) == 0)
                {
                    printf("Métricas de Prometheus actualizadas\n");
                }
                else
                {
                    fprintf(stderr, "Error al actualizar métricas de Prometheus\n");
                }
            }
            else
            {
                fprintf(stderr, "Error al recolectar métricas\n");
            }

            // Espera hasta el próximo intervalo
            sleep((unsigned int)interval);
        }
    }

    // Limpia recursos
    cleanup_prometheus();
    cleanup_monitoring_system();

    printf("\n Sistema de monitoreo detenido\n");

    return EXIT_SUCCESS;
}