/**
 * @file prometheus_metrics.c
 * @brief Implementación básica de métricas para Prometheus
 */

#include "monitoring.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <prom.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

// Variables globales para métricas Prometheus
prom_gauge_t* prometheus_cpu_usage_percent = NULL;
prom_gauge_t* prometheus_memory_total_bytes = NULL;
prom_gauge_t* prometheus_memory_free_bytes = NULL;
prom_gauge_t* prometheus_memory_used_bytes = NULL;
prom_gauge_t* prometheus_load_average_1m = NULL;
prom_gauge_t* prometheus_load_average_5m = NULL;
prom_gauge_t* prometheus_load_average_15m = NULL;
prom_counter_t* prometheus_metrics_collected_total = NULL;

// Variables para el servidor HTTP
static pthread_t prometheus_thread;
static int prometheus_server_running = 0;
static int server_socket = -1;

/**
 * @brief Servidor HTTP básico para métricas de Prometheus
 */
static void* prometheus_server_thread_func(void* arg)
{
    int port = *(int*)arg;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Crear socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creando socket");
        return NULL;
    }

    // Configurar reutilización de dirección
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Error en setsockopt");
        close(server_socket);
        return NULL;
    }

    // Configurar dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error en bind");
        close(server_socket);
        return NULL;
    }

    // Listen
    if (listen(server_socket, 5) < 0)
    {
        perror("Error en listen");
        close(server_socket);
        return NULL;
    }

    printf("🚀 Servidor de métricas Prometheus iniciado en puerto %d\n", port);
    printf("📊 Métricas disponibles en: http://localhost:%d/metrics\n", port);

    while (prometheus_server_running)
    {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (prometheus_server_running)
            {
                perror("Error en accept");
            }
            break;
        }

        // Generar métricas en formato Prometheus
        const char* metrics_output = prom_collector_registry_bridge(PROM_COLLECTOR_REGISTRY_DEFAULT);
        const char* default_message = "# No metrics available\n";
        int should_free = 1;

        if (!metrics_output)
        {
            metrics_output = default_message;
            should_free = 0;
        }

        // Crear respuesta HTTP
        char response[8192];
        int response_len = snprintf(response, sizeof(response),
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/plain; charset=utf-8\r\n"
                                    "Content-Length: %ld\r\n"
                                    "Connection: close\r\n"
                                    "\r\n"
                                    "%s",
                                    strlen(metrics_output), metrics_output);

        // Enviar respuesta
        send(client_socket, response, (size_t)response_len, 0);
        close(client_socket);

        if (should_free && metrics_output)
        {
            free((void*)metrics_output);
        }
    }

    close(server_socket);
    return NULL;
}

/**
 * @brief Inicializa las métricas de Prometheus
 */
int init_prometheus_metrics(void)
{
    // Inicializar el registro de Prometheus
    if (prom_collector_registry_default_init() != 0)
    {
        fprintf(stderr, "Error al inicializar registro de Prometheus\n");
        return -1;
    }

    // Crear métricas gauge para CPU
    prometheus_cpu_usage_percent = prom_gauge_new("monitoring_cpu_usage_percent", "CPU usage percentage", 0, NULL);

    // Crear métricas gauge para memoria
    prometheus_memory_total_bytes = prom_gauge_new("monitoring_memory_total_bytes", "Total memory in bytes", 0, NULL);

    prometheus_memory_free_bytes = prom_gauge_new("monitoring_memory_free_bytes", "Free memory in bytes", 0, NULL);

    prometheus_memory_used_bytes = prom_gauge_new("monitoring_memory_used_bytes", "Used memory in bytes", 0, NULL);

    // Crear métricas gauge para load average
    prometheus_load_average_1m = prom_gauge_new("monitoring_load_average_1m", "Load average 1 minute", 0, NULL);

    prometheus_load_average_5m = prom_gauge_new("monitoring_load_average_5m", "Load average 5 minutes", 0, NULL);

    prometheus_load_average_15m = prom_gauge_new("monitoring_load_average_15m", "Load average 15 minutes", 0, NULL);

    // Crear contador para métricas recolectadas
    prometheus_metrics_collected_total =
        prom_counter_new("monitoring_metrics_collected_total", "Total number of metrics collected", 0, NULL);

    // Verificar que todas las métricas se crearon correctamente
    if (!prometheus_cpu_usage_percent || !prometheus_memory_total_bytes || !prometheus_memory_free_bytes ||
        !prometheus_memory_used_bytes || !prometheus_load_average_1m || !prometheus_load_average_5m ||
        !prometheus_load_average_15m || !prometheus_metrics_collected_total)
    {
        fprintf(stderr, "Error al crear métricas de Prometheus\n");
        return -1;
    }

    // Registrar métricas en el registro por defecto
    prom_collector_registry_must_register_metric(prometheus_cpu_usage_percent);
    prom_collector_registry_must_register_metric(prometheus_memory_total_bytes);
    prom_collector_registry_must_register_metric(prometheus_memory_free_bytes);
    prom_collector_registry_must_register_metric(prometheus_memory_used_bytes);
    prom_collector_registry_must_register_metric(prometheus_load_average_1m);
    prom_collector_registry_must_register_metric(prometheus_load_average_5m);
    prom_collector_registry_must_register_metric(prometheus_load_average_15m);
    prom_collector_registry_must_register_metric(prometheus_metrics_collected_total);

    printf("✅ Métricas de Prometheus inicializadas correctamente\n");
    return 0;
}

/**
 * @brief Actualiza las métricas de Prometheus con los valores actuales
 */
int update_prometheus_metrics(const system_metrics_t* metrics)
{
    if (!metrics)
    {
        return -1;
    }

    // Incrementar contador de métricas recolectadas
    prom_counter_inc(prometheus_metrics_collected_total, NULL);

    // Actualizar métricas de CPU
    prom_gauge_set(prometheus_cpu_usage_percent, metrics->cpu.usage_percent, NULL);

    // Actualizar métricas de memoria (convertir de KB a bytes)
    prom_gauge_set(prometheus_memory_total_bytes, (double)metrics->memory.total * 1024.0, NULL);
    prom_gauge_set(prometheus_memory_free_bytes, (double)metrics->memory.free * 1024.0, NULL);
    prom_gauge_set(prometheus_memory_used_bytes, (double)metrics->memory.used * 1024.0, NULL);

    // Actualizar métricas de load average
    prom_gauge_set(prometheus_load_average_1m, metrics->load.load_1m, NULL);
    prom_gauge_set(prometheus_load_average_5m, metrics->load.load_5m, NULL);
    prom_gauge_set(prometheus_load_average_15m, metrics->load.load_15m, NULL);

    return 0;
}

/**
 * @brief Inicia el servidor HTTP de Prometheus
 */
int start_prometheus_server(int port)
{
    if (prometheus_server_running)
    {
        printf("⚠️  Servidor de Prometheus ya está ejecutándose\n");
        return 0;
    }

    static int server_port;
    server_port = port;

    prometheus_server_running = 1;

    if (pthread_create(&prometheus_thread, NULL, prometheus_server_thread_func, &server_port) != 0)
    {
        perror("Error al crear hilo del servidor Prometheus");
        prometheus_server_running = 0;
        return -1;
    }

    return 0;
}

/**
 * @brief Limpia recursos de Prometheus
 */
void cleanup_prometheus(void)
{
    if (prometheus_server_running)
    {
        prometheus_server_running = 0;

        // Cerrar socket para que accept() falle y termine el hilo
        if (server_socket >= 0)
        {
            close(server_socket);
        }

        // Esperar a que termine el hilo
        pthread_join(prometheus_thread, NULL);
    }

    printf("🧹 Limpieza de recursos de Prometheus completada\n");
}