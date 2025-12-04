/**
 * @file shell_tp3_commands.c
 * @brief Implementación de comandos del shell para TP3
 *
 * Incluye comandos para:
 * - Gestión de memoria (memory_manager)
 * - Almacenamiento persistente (storage)
 * - Sandbox de ejecución (sandbox)
 * - Demostraciones integradas
 */

#include "memory_manager.h"
#include "sandbox.h"
#include "shell.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================
// COMANDOS DE GESTIÓN DE MEMORIA
// ============================================================

/**
 * @brief Inicializa el gestor de memoria con estrategia específica
 */
int cmd_mem_init(shell_context_t* ctx, const char* strategy)
{
    (void)ctx;

    alloc_strategy_t strat = ALLOC_FIRST_FIT; // Por defecto

    if (strategy)
    {
        if (strcmp(strategy, "best_fit") == 0)
        {
            strat = ALLOC_BEST_FIT;
        }
        else if (strcmp(strategy, "first_fit") == 0)
        {
            strat = ALLOC_FIRST_FIT;
        }
        else
        {
            printf("Estrategia desconocida: %s\n", strategy);
            printf("Uso: mem_init [first_fit|best_fit]\n");
            return -1;
        }
    }

    if (mem_init(strat) == 0)
    {
        printf("Gestor de memoria inicializado con estrategia: %s\n",
               strat == ALLOC_FIRST_FIT ? "First Fit" : "Best Fit");
        return 0;
    }

    printf("Error al inicializar gestor de memoria\n");
    return -1;
}

/**
 * @brief Asigna un bloque de memoria
 */
int cmd_mem_alloc(shell_context_t* ctx, const char* size_str)
{
    (void)ctx;

    if (!size_str || strlen(size_str) == 0)
    {
        printf("Uso: mem_alloc <size>\n");
        printf("Ejemplo: mem_alloc 256\n");
        return -1;
    }

    size_t size = (size_t)atoi(size_str);

    if (size == 0)
    {
        printf("Error: tamaño inválido\n");
        return -1;
    }

    void* ptr = mem_alloc(size);

    if (ptr)
    {
        printf("Memoria asignada: %zu bytes en dirección %p\n", size, ptr);
        return 0;
    }

    printf("Error: no se pudo asignar memoria\n");
    return -1;
}

/**
 * @brief Libera un bloque de memoria
 */
int cmd_mem_free(shell_context_t* ctx, const char* ptr_str)
{
    (void)ctx;

    if (!ptr_str || strlen(ptr_str) == 0)
    {
        printf("Uso: mem_free <address>\n");
        printf("Ejemplo: mem_free 0x7fff12345678\n");
        return -1;
    }

    void* ptr = NULL;
    if (sscanf(ptr_str, "%p", &ptr) != 1)
    {
        printf("Error: dirección inválida\n");
        return -1;
    }

    if (mem_free(ptr) == 0)
    {
        printf("Memoria liberada: %p\n", ptr);
        return 0;
    }

    printf("Error al liberar memoria\n");
    return -1;
}

/**
 * @brief Muestra el estado completo del heap
 */
int cmd_mem_dump(shell_context_t* ctx)
{
    (void)ctx;
    mem_dump_state();
    return 0;
}

/**
 * @brief Muestra estadísticas del gestor de memoria
 */
int cmd_mem_stats(shell_context_t* ctx)
{
    (void)ctx;

    heap_stats_t stats;

    if (mem_get_stats(&stats) == 0)
    {
        printf("\n=== ESTADÍSTICAS DE MEMORIA ===\n");
        printf("Tamaño total:          %zu bytes\n", stats.total_size);
        printf("Memoria utilizada:     %zu bytes (%.2f%%)\n", stats.used_size,
               (double)stats.used_size / stats.total_size * 100.0);
        printf("Memoria libre:         %zu bytes (%.2f%%)\n", stats.free_size,
               (double)stats.free_size / stats.total_size * 100.0);
        printf("Mayor bloque libre:    %zu bytes\n", stats.largest_free_block);
        printf("Bloques libres:        %zu\n", stats.free_block_count);
        printf("Bloques asignados:     %zu\n", stats.allocated_block_count);
        printf("Fragmentación externa: %.2f%%\n", stats.fragmentation_ratio * 100.0);
        printf("===============================\n\n");
        return 0;
    }

    printf("Error al obtener estadísticas\n");
    return -1;
}

/**
 * @brief Ejecuta tests de memoria
 */
int cmd_mem_test(shell_context_t* ctx, const char* test_type)
{
    (void)ctx;

    if (!test_type)
    {
        printf("Tests disponibles:\n");
        printf("  fragmentation  - Demuestra fragmentación externa\n");
        printf("  coalescing     - Demuestra coalescing de bloques\n");
        printf("  stress         - Test de estrés con múltiples asignaciones\n");
        return 0;
    }

    if (strcmp(test_type, "fragmentation") == 0)
    {
        printf("\n=== TEST DE FRAGMENTACIÓN ===\n");
        printf("Inicializando memoria...\n");
        mem_init(ALLOC_FIRST_FIT);

        printf("Asignando 4 bloques de 1KB...\n");
        void* p1 = mem_alloc(1024);
        void* p2 = mem_alloc(1024);
        void* p3 = mem_alloc(1024);
        void* p4 = mem_alloc(1024);

        printf("Liberando bloques alternados (p2 y p4)...\n");
        mem_free(p2);
        mem_free(p4);

        printf("Intentando asignar bloque de 2KB...\n");
        void* p5 = mem_alloc(2048);

        if (p5)
        {
            printf("✓ Asignación exitosa (no hay fragmentación crítica)\n");
        }
        else
        {
            printf("✗ Falló - fragmentación externa evitó la asignación\n");
        }

        mem_dump_state();

        mem_free(p1);
        mem_free(p3);
        if (p5)
            mem_free(p5);

        return 0;
    }

    if (strcmp(test_type, "coalescing") == 0)
    {
        printf("\n=== TEST DE COALESCING ===\n");
        printf("Inicializando memoria...\n");
        mem_init(ALLOC_FIRST_FIT);

        printf("Asignando 3 bloques contiguos...\n");
        void* p1 = mem_alloc(512);
        void* p2 = mem_alloc(512);
        void* p3 = mem_alloc(512);

        printf("Estado antes de liberar:\n");
        mem_dump_state();

        printf("\nLiberando bloques en orden...\n");
        mem_free(p1);
        mem_free(p2);
        mem_free(p3);

        printf("Estado después de coalescing:\n");
        mem_dump_state();

        return 0;
    }

    if (strcmp(test_type, "stress") == 0)
    {
        printf("\n=== TEST DE ESTRÉS ===\n");
        mem_init(ALLOC_FIRST_FIT);

        void* ptrs[20];
        int count = 0;

        printf("Asignando 20 bloques de tamaños variables...\n");
        for (int i = 0; i < 20; i++)
        {
            size_t size = 128 + (i * 64);
            ptrs[i] = mem_alloc(size);
            if (ptrs[i])
                count++;
        }

        printf("Asignados: %d/20 bloques\n", count);

        printf("Liberando bloques pares...\n");
        for (int i = 0; i < 20; i += 2)
        {
            if (ptrs[i])
                mem_free(ptrs[i]);
        }

        mem_dump_state();

        printf("Liberando bloques impares...\n");
        for (int i = 1; i < 20; i += 2)
        {
            if (ptrs[i])
                mem_free(ptrs[i]);
        }

        printf("Test completado\n");
        return 0;
    }

    printf("Test desconocido: %s\n", test_type);
    return -1;
}

/**
 * @brief Limpia el gestor de memoria
 */
int cmd_mem_cleanup(shell_context_t* ctx)
{
    (void)ctx;
    mem_cleanup();
    printf("Gestor de memoria limpiado\n");
    return 0;
}

// ============================================================
// COMANDOS DE ALMACENAMIENTO PERSISTENTE
// ============================================================

/**
 * @brief Inicializa el sistema de almacenamiento
 */
int cmd_storage_init(shell_context_t* ctx)
{
    (void)ctx;

    if (storage_init() == 0)
    {
        printf("Sistema de almacenamiento inicializado\n");
        return 0;
    }

    printf("Error al inicializar almacenamiento\n");
    return -1;
}

/**
 * @brief Escribe métricas actuales al almacenamiento
 */
int cmd_storage_write(shell_context_t* ctx)
{
    (void)ctx;

    // Recolectar métricas actuales
    system_metrics_t metrics;
    if (collect_metrics(&metrics) != 0)
    {
        printf("Error al recolectar métricas\n");
        return -1;
    }

    // Abrir archivo del día actual
    storage_handle_t handle;
    if (storage_open_daily_file(&handle, time(NULL), 0) != 0)
    {
        printf("Error al abrir archivo de almacenamiento\n");
        return -1;
    }

    // Escribir registro
    if (storage_write_record(&handle, &metrics) != 0)
    {
        printf("Error al escribir registro\n");
        storage_close(&handle);
        return -1;
    }

    printf("Métricas escritas exitosamente\n");
    printf("Timestamp: %ld\n", metrics.timestamp);
    printf("CPU: %.2f%%\n", metrics.cpu.usage_percent);
    printf("Memoria usada: %ld KB\n", metrics.memory.used);

    storage_close(&handle);
    return 0;
}

/**
 * @brief Callback para imprimir registros
 */
static int print_record_callback(const storage_record_t* record, void* user_data)
{
    (void)user_data;

    time_t ts = (time_t)record->timestamp;
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&ts));

    printf("[%s] CPU: %.2f%% | MEM: %lu KB usado | LOAD: %.2f\n", timestamp, record->cpu_usage, record->memory_used,
           record->load_1m);

    return 0; // Continuar
}

/**
 * @brief Lee registros del almacenamiento
 */
int cmd_storage_read(shell_context_t* ctx, const char* date_str)
{
    (void)ctx;

    time_t date = time(NULL); // Por defecto, hoy

    if (date_str && strlen(date_str) > 0)
    {
        // Parsear fecha YYYYMMDD
        struct tm tm_date = {0};
        if (sscanf(date_str, "%4d%2d%2d", &tm_date.tm_year, &tm_date.tm_mon, &tm_date.tm_mday) == 3)
        {
            tm_date.tm_year -= 1900;
            tm_date.tm_mon -= 1;
            date = mktime(&tm_date);
        }
        else
        {
            printf("Formato de fecha inválido. Use: YYYYMMDD\n");
            return -1;
        }
    }

    storage_handle_t handle;
    if (storage_open_daily_file(&handle, date, 1) != 0)
    {
        printf("No hay datos para la fecha especificada\n");
        return -1;
    }

    printf("\n=== REGISTROS DE ALMACENAMIENTO ===\n");
    int count = storage_read_all_records(&handle, print_record_callback, NULL);
    printf("===================================\n");
    printf("Total de registros válidos: %d\n\n", count);

    storage_close(&handle);
    return 0;
}

/**
 * @brief Lista archivos de almacenamiento disponibles
 */
int cmd_storage_list(shell_context_t* ctx)
{
    (void)ctx;

    char files[32][STORAGE_MAX_PATH_LENGTH];
    int count = storage_list_files(files, 32);

    if (count < 0)
    {
        printf("Error al listar archivos\n");
        return -1;
    }

    printf("\n=== ARCHIVOS DE ALMACENAMIENTO ===\n");
    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n", i + 1, files[i]);
    }
    printf("==================================\n");
    printf("Total: %d archivo(s)\n\n", count);

    return 0;
}

/**
 * @brief Muestra estadísticas del almacenamiento
 */
int cmd_storage_stats(shell_context_t* ctx)
{
    (void)ctx;

    storage_stats_t stats;
    if (storage_get_stats(&stats) != 0)
    {
        printf("Error al obtener estadísticas\n");
        return -1;
    }

    printf("\n=== ESTADÍSTICAS DE ALMACENAMIENTO ===\n");
    printf("Archivos totales:      %u\n", stats.total_files);
    printf("Registros totales:     %u\n", stats.total_records);
    printf("Registros válidos:     %u\n", stats.valid_records);
    printf("Registros corruptos:   %u\n", stats.corrupt_records);
    printf("Tamaño total:          %lu bytes (%.2f MB)\n", stats.total_size, stats.total_size / 1024.0 / 1024.0);

    if (stats.oldest_record > 0)
    {
        char oldest[64], newest[64];
        strftime(oldest, sizeof(oldest), "%Y-%m-%d %H:%M:%S", localtime(&stats.oldest_record));
        strftime(newest, sizeof(newest), "%Y-%m-%d %H:%M:%S", localtime(&stats.newest_record));
        printf("Registro más antiguo:  %s\n", oldest);
        printf("Registro más reciente: %s\n", newest);
    }
    printf("======================================\n\n");

    return 0;
}

/**
 * @brief Valida un archivo de almacenamiento
 */
int cmd_storage_validate(shell_context_t* ctx, const char* filename)
{
    (void)ctx;

    if (!filename || strlen(filename) == 0)
    {
        printf("Uso: storage_validate <filename>\n");
        return -1;
    }

    printf("Validación de archivos no implementada completamente\n");
    printf("Use storage_read para verificar registros\n");
    return 0;
}

// ============================================================
// COMANDOS DE SANDBOX
// ============================================================

/**
 * @brief Inicializa el sandbox
 */
int cmd_sandbox_init(shell_context_t* ctx, const char* mode)
{
    (void)ctx;

    sandbox_config_t config = {0};

    if (mode && strcmp(mode, "mode_a") == 0)
    {
        config.mode = SANDBOX_MODE_A;
    }
    else if (mode && strcmp(mode, "mode_b") == 0)
    {
        config.mode = SANDBOX_MODE_B;
    }
    else
    {
        config.mode = sandbox_detect_capabilities();
    }

    if (sandbox_init(&config) == 0)
    {
        printf("Sandbox inicializado en %s\n", sandbox_mode_to_string(config.mode));
        return 0;
    }

    printf("Error al inicializar sandbox\n");
    return -1;
}

/**
 * @brief Lista plugins disponibles
 */
int cmd_sandbox_list(shell_context_t* ctx)
{
    (void)ctx;

    plugin_info_t plugins[SANDBOX_MAX_PLUGINS];
    int count = sandbox_list_plugins(plugins, SANDBOX_MAX_PLUGINS);

    if (count < 0)
    {
        printf("Error al listar plugins\n");
        return -1;
    }

    printf("\n=== PLUGINS DISPONIBLES ===\n");
    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n", i + 1, plugins[i].name);
        printf("   Ruta: %s\n", plugins[i].path);
        printf("   Tamaño: %zu bytes\n", plugins[i].file_size);
        printf("   Válido: %s\n", plugins[i].is_valid ? "Sí" : "No");
        printf("\n");
    }
    printf("===========================\n");
    printf("Total: %d plugin(s)\n\n", count);

    return 0;
}

/**
 * @brief Ejecuta un plugin en el sandbox
 */
int cmd_sandbox_run(shell_context_t* ctx, const char* plugin_name, const char* input_file)
{
    (void)ctx;

    if (!plugin_name)
    {
        printf("Uso: sandbox_run <plugin_name> <input_file>\n");
        printf("Ejemplo: sandbox_run libanalizador.so /var/monitoreo/sandbox/datos.txt\n");
        return -1;
    }

    sandbox_execution_t execution;

    printf("Ejecutando plugin: %s\n", plugin_name);
    if (input_file)
    {
        printf("Archivo de entrada: %s\n", input_file);
    }

    int result = sandbox_execute_plugin(plugin_name, input_file, &execution);

    printf("\n=== RESULTADO DE EJECUCIÓN ===\n");
    printf("Estado: %s\n", sandbox_result_to_string(execution.result));
    printf("Código de salida: %d\n", execution.exit_code);
    printf("Duración: %ld segundos\n", execution.end_time - execution.start_time);

    if (execution.stdout_size > 0)
    {
        printf("\n--- STDOUT ---\n");
        printf("%s\n", execution.stdout_data);
    }

    if (execution.stderr_size > 0)
    {
        printf("\n--- STDERR ---\n");
        printf("%s\n", execution.stderr_data);
    }

    printf("==============================\n\n");

    return result;
}

/**
 * @brief Muestra configuración del sandbox
 */
int cmd_sandbox_config(shell_context_t* ctx)
{
    (void)ctx;
    sandbox_dump_config(NULL);
    return 0;
}

/**
 * @brief Test del sandbox
 */
int cmd_sandbox_test(shell_context_t* ctx)
{
    (void)ctx;

    printf("\n=== TEST DEL SANDBOX ===\n");
    printf("Detectando capacidades del sistema...\n");

    sandbox_mode_t mode = sandbox_detect_capabilities();
    printf("Modo detectado: %s\n", sandbox_mode_to_string(mode));

    printf("\nPara ejecutar un test completo:\n");
    printf("1. Compile el plugin: g++ -std=gnu++20 -fPIC -shared analizador.cpp -o libanalizador.so\n");
    printf("2. Coloque el plugin en: /var/monitoreo/plugins/\n");
    printf("3. Cree un archivo de datos de prueba en: /var/monitoreo/sandbox/test_data.txt\n");
    printf("4. Ejecute: sandbox_run libanalizador.so /var/monitoreo/sandbox/test_data.txt\n");

    return 0;
}

// ============================================================
// COMANDO DE DEMOSTRACIÓN INTEGRADA
// ============================================================

/**
 * @brief Ejecuta demostraciones integradas del sistema
 */
int cmd_demo(shell_context_t* ctx, const char* demo_type)
{
    if (!demo_type)
    {
        printf("\nDemostraciones disponibles:\n");
        printf("  memory   - Demostración del gestor de memoria\n");
        printf("  storage  - Demostración del almacenamiento persistente\n");
        printf("  sandbox  - Demostración del sandbox\n");
        printf("  full     - Demostración completa integrada\n");
        return 0;
    }

    if (strcmp(demo_type, "memory") == 0)
    {
        printf("\n========================================\n");
        printf("   DEMOSTRACIÓN: GESTOR DE MEMORIA\n");
        printf("========================================\n\n");

        cmd_mem_init(ctx, "first_fit");
        cmd_mem_test(ctx, "fragmentation");
        cmd_mem_stats(ctx);

        return 0;
    }

    if (strcmp(demo_type, "storage") == 0)
    {
        printf("\n========================================\n");
        printf("   DEMOSTRACIÓN: ALMACENAMIENTO\n");
        printf("========================================\n\n");

        cmd_storage_init(ctx);
        cmd_storage_write(ctx);
        cmd_storage_list(ctx);
        cmd_storage_stats(ctx);

        return 0;
    }

    if (strcmp(demo_type, "sandbox") == 0)
    {
        printf("\n========================================\n");
        printf("   DEMOSTRACIÓN: SANDBOX\n");
        printf("========================================\n\n");

        cmd_sandbox_init(ctx, NULL);
        cmd_sandbox_list(ctx);
        cmd_sandbox_config(ctx);

        return 0;
    }

    if (strcmp(demo_type, "full") == 0)
    {
        printf("\n========================================\n");
        printf("   DEMOSTRACIÓN COMPLETA INTEGRADA\n");
        printf("========================================\n\n");

        printf("1. Inicializando subsistemas...\n");
        cmd_mem_init(ctx, "first_fit");
        cmd_storage_init(ctx);
        cmd_sandbox_init(ctx, NULL);

        printf("\n2. Demostrando gestión de memoria...\n");
        cmd_mem_test(ctx, "coalescing");

        printf("\n3. Escribiendo métricas al almacenamiento...\n");
        cmd_storage_write(ctx);
        cmd_storage_stats(ctx);

        printf("\n4. Estado del sandbox...\n");
        cmd_sandbox_config(ctx);

        printf("\n========================================\n");
        printf("   DEMOSTRACIÓN COMPLETADA\n");
        printf("========================================\n\n");

        return 0;
    }

    printf("Demostración desconocida: %s\n", demo_type);
    return -1;
}
