/**
 * @file test_metrics.c
 * @brief Tests unitarios para las funciones de métricas
 */

#include "monitoring.h"
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

    return UNITY_END();
}