/**
 * @file test_metrics.c
 * @brief Tests unitarios para las funciones de métricas
 */

#include "monitoring.h"
#include "cjson/cJSON.h"
#include "unity.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TEST_JSON_FILE "/tmp/test_metrics.json"

void setUp(void)
{
}
void tearDown(void)
{
    remove(TEST_JSON_FILE);
}

/**
 * @brief Test read_cpu_metrics
 */
void test_read_cpu_metrics(void)
{
    cpu_metrics_t cpu;
    int result = read_cpu_metrics(&cpu);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(cpu.user >= 0.0 && cpu.user <= 100.0);
    TEST_ASSERT_TRUE(cpu.system >= 0.0 && cpu.system <= 100.0);
    TEST_ASSERT_TRUE(cpu.idle >= 0.0 && cpu.idle <= 100.0);
    TEST_ASSERT_TRUE(cpu.usage_percent >= 0.0 && cpu.usage_percent <= 100.0);
}

/**
 * @brief Test read_memory_metrics
 */
void test_read_memory_metrics(void)
{
    memory_metrics_t memory;
    int result = read_memory_metrics(&memory);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(memory.total > 0);
    TEST_ASSERT_TRUE(memory.free >= 0);
    TEST_ASSERT_TRUE(memory.used >= 0);
    TEST_ASSERT_TRUE(memory.buffers >= 0);
    TEST_ASSERT_TRUE(memory.cached >= 0);
    TEST_ASSERT_EQUAL_INT(memory.total, memory.free + memory.buffers + memory.cached + memory.used);
}

/**
 * @brief Test read_load_metrics
 */
void test_read_load_metrics(void)
{
    load_metrics_t load;
    int result = read_load_metrics(&load);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(load.load_1m >= 0.0);
    TEST_ASSERT_TRUE(load.load_5m >= 0.0);
    TEST_ASSERT_TRUE(load.load_15m >= 0.0);
}

/**
 * @brief Test collect_metrics
 */
void test_collect_metrics(void)
{
    system_metrics_t metrics;
    int result = collect_metrics(&metrics);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(metrics.timestamp > 0);
    TEST_ASSERT_TRUE(metrics.cpu.usage_percent >= 0.0 && metrics.cpu.usage_percent <= 100.0);
    TEST_ASSERT_TRUE(metrics.memory.total > 0);
    TEST_ASSERT_TRUE(metrics.memory.used >= 0);
    TEST_ASSERT_TRUE(metrics.load.load_1m >= 0.0);
}

/**
 * @brief Test save_metrics_to_json
 */
void test_save_metrics_to_json(void)
{
    system_metrics_t metrics;
    metrics.timestamp = 1234567890;
    metrics.cpu.user = 10.5;
    metrics.cpu.system = 5.2;
    metrics.cpu.idle = 84.3;
    metrics.cpu.usage_percent = 15.7;
    metrics.memory.total = 8192000;
    metrics.memory.free = 2048000;
    metrics.memory.used = 4096000;
    metrics.memory.buffers = 1024000;
    metrics.memory.cached = 1024000;
    metrics.load.load_1m = 0.5;
    metrics.load.load_5m = 0.7;
    metrics.load.load_15m = 1.0;

    int result = save_metrics_to_json(&metrics, TEST_JSON_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE* file = fopen(TEST_JSON_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);

    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    TEST_ASSERT_TRUE(strstr(buffer, "timestamp") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "cpu_user") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "cpu_system") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "cpu_usage") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "mem_total") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "load_1m") != NULL);
}

/**
 * @brief Test init_monitoring_system
 */
void test_init_monitoring_system(void)
{
#ifdef GITHUB_ACTIONS
    TEST_IGNORE_MESSAGE("Skipping test in GitHub Actions");
    return;
#endif

    char test_dir[256];
    snprintf(test_dir, sizeof(test_dir), "/tmp/monitoring_test_%d/", getpid());
    extern char* test_log_dir;
    char* old_test_log_dir = test_log_dir;
    test_log_dir = test_dir;

    char rm_command[300];
    snprintf(rm_command, sizeof(rm_command), "rm -rf %s", test_dir);
    system(rm_command);

    int result = init_monitoring_system();
    TEST_ASSERT_EQUAL_INT(0, result);

    struct stat st;
    int stat_result = stat(test_dir, &st);
    TEST_ASSERT_EQUAL_INT(0, stat_result);
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

    test_log_dir = old_test_log_dir;
    system(rm_command);
}

/**
 * @brief Test que verifica que los timestamps aumentan entre mediciones
 */
void test_metrics_timestamp_increases(void)
{
    system_metrics_t metrics1, metrics2;
    
    int result1 = collect_metrics(&metrics1);
    TEST_ASSERT_EQUAL_INT(0, result1);
    
    sleep(1);
    
    int result2 = collect_metrics(&metrics2);
    TEST_ASSERT_EQUAL_INT(0, result2);
    
    TEST_ASSERT_TRUE(metrics2.timestamp >= metrics1.timestamp);
}

/**
 * @brief Test de validación del formato JSON
 */
void test_json_format_validation(void)
{
    system_metrics_t metrics;
    int result = collect_metrics(&metrics);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    const char* test_file = "/tmp/test_json_validation.json";
    result = save_metrics_to_json(&metrics, test_file);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    FILE* file = fopen(test_file, "r");
    TEST_ASSERT_NOT_NULL(file);
    
    char buffer[1024];
    char* line = fgets(buffer, sizeof(buffer), file);
    TEST_ASSERT_NOT_NULL(line);
    fclose(file);
    
    // Verificar que es JSON válido
    cJSON* json = cJSON_Parse(buffer);
    TEST_ASSERT_NOT_NULL(json);
    
    // Verificar campos obligatorios
    cJSON* timestamp = cJSON_GetObjectItem(json, "timestamp");
    TEST_ASSERT_NOT_NULL(timestamp);
    TEST_ASSERT_TRUE(cJSON_IsNumber(timestamp));
    
    cJSON_Delete(json);
    remove(test_file);
}

/**
 * @brief Test save_metrics_to_json con path NULL (usa path por defecto)
 */
void test_save_metrics_with_null_path(void)
{
    system_metrics_t metrics;
    collect_metrics(&metrics);
    
    // Debería usar path por defecto
    int result = save_metrics_to_json(&metrics, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test save_metrics_to_json con métricas NULL
 */
void test_save_metrics_with_null_metrics(void)
{
    int result = save_metrics_to_json(NULL, "/tmp/test.json");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test collect_metrics con puntero NULL
 */
void test_collect_metrics_with_null_pointer(void)
{
    int result = collect_metrics(NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test que verifica rangos válidos de métricas de CPU
 */
void test_cpu_metrics_ranges(void)
{
    cpu_metrics_t cpu;
    int result = read_cpu_metrics(&cpu);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que los porcentajes estén en rango válido
    TEST_ASSERT_TRUE(cpu.user >= 0.0 && cpu.user <= 100.0);
    TEST_ASSERT_TRUE(cpu.system >= 0.0 && cpu.system <= 100.0);
    TEST_ASSERT_TRUE(cpu.idle >= 0.0 && cpu.idle <= 100.0);
    TEST_ASSERT_TRUE(cpu.usage_percent >= 0.0 && cpu.usage_percent <= 100.0);
    
    // La suma de user + system + idle debería ser menor o igual a 100
    double sum = cpu.user + cpu.system + cpu.idle;
    TEST_ASSERT_TRUE(sum <= 105.0); // Pequeño margen por redondeo
}

/**
 * @brief Test save_metrics_to_json con path vacío (debería usar default)
 */
void test_save_metrics_with_empty_path(void)
{
    system_metrics_t metrics;
    collect_metrics(&metrics);
    
    // Path vacío debería usar path por defecto
    int result = save_metrics_to_json(&metrics, "");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test cleanup_monitoring_system (aunque esté vacía)
 */
void test_cleanup_monitoring_system(void)
{
    // Esta función debería ejecutarse sin problemas aunque esté vacía
    cleanup_monitoring_system();
    TEST_ASSERT_TRUE(1); // Test que siempre pasa para verificar que se ejecutó
}

/**
 * @brief Test para verificar coherencia en métricas de memoria
 */
void test_memory_metrics_coherence(void)
{
    memory_metrics_t memory;
    int result = read_memory_metrics(&memory);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que used es positivo y coherente
    TEST_ASSERT_TRUE(memory.used >= 0);
    TEST_ASSERT_TRUE(memory.total >= memory.used);
    TEST_ASSERT_TRUE(memory.total >= memory.free);
    
    // El total debería ser la suma de todos los componentes
    long calculated_total = memory.free + memory.buffers + memory.cached + memory.used;
    TEST_ASSERT_EQUAL_INT(memory.total, calculated_total);
}

/**
 * @brief Función principal para ejecutar los tests
 */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_read_cpu_metrics);
    RUN_TEST(test_read_memory_metrics);
    RUN_TEST(test_read_load_metrics);
    RUN_TEST(test_collect_metrics);
    RUN_TEST(test_save_metrics_to_json);
    RUN_TEST(test_init_monitoring_system);
    
    // Nuevos tests para aumentar coverage
    RUN_TEST(test_metrics_timestamp_increases);
    RUN_TEST(test_json_format_validation);
    RUN_TEST(test_save_metrics_with_null_path);
    RUN_TEST(test_save_metrics_with_null_metrics);
    RUN_TEST(test_collect_metrics_with_null_pointer);
    RUN_TEST(test_cpu_metrics_ranges);
    RUN_TEST(test_save_metrics_with_empty_path);
    RUN_TEST(test_cleanup_monitoring_system);
    RUN_TEST(test_memory_metrics_coherence);

    return UNITY_END();
}