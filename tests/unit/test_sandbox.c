/**
 * @file test_sandbox.c
 * @brief Tests unitarios para el sistema de sandbox
 */

#include "sandbox.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Directorios de prueba
#define TEST_PLUGINS_DIR "/tmp/test_sandbox_plugins/"
#define TEST_WORK_DIR "/tmp/test_sandbox_work/"
#define TEST_LOG_FILE "/tmp/test_sandbox.log"

void setUp(void)
{
    // Crear directorios de prueba
    mkdir(TEST_PLUGINS_DIR, 0755);
    mkdir(TEST_WORK_DIR, 0755);

    // Limpiar archivos anteriores
    char command[512];
    snprintf(command, sizeof(command), "rm -f %s* %s* %s", TEST_PLUGINS_DIR, TEST_WORK_DIR, TEST_LOG_FILE);
    system(command);
}

void tearDown(void)
{
    // Limpiar directorios de prueba
    char command[512];
    snprintf(command, sizeof(command), "rm -rf %s %s %s", TEST_PLUGINS_DIR, TEST_WORK_DIR, TEST_LOG_FILE);
    system(command);

    sandbox_cleanup();
}

/**
 * @brief Test de detección de capacidades
 */
void test_sandbox_detect_capabilities(void)
{
    sandbox_mode_t mode = sandbox_detect_capabilities();

    // Debe retornar un modo válido
    TEST_ASSERT_TRUE(mode == SANDBOX_MODE_A || mode == SANDBOX_MODE_B);

    // Si no somos root, debe ser Modo B
    if (getuid() != 0)
    {
        TEST_ASSERT_EQUAL_INT(SANDBOX_MODE_B, mode);
    }
}

/**
 * @brief Test de inicialización básica del sandbox
 */
void test_sandbox_init_basic(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B; // Modo seguro para tests
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);
    config.cpu_limit = 10;
    config.file_size_limit = 1024 * 1024;
    config.max_fds = 32;

    int result = sandbox_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de inicialización con NULL (usa defaults)
 */
void test_sandbox_init_defaults(void)
{
    int result = sandbox_init(NULL);

    // Puede fallar si no existen los directorios por defecto
    // pero no debe crashear
    TEST_ASSERT_TRUE(result == 0 || result == -1);
}

/**
 * @brief Test de cambio de modo
 */
void test_sandbox_set_mode(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    TEST_ASSERT_EQUAL_INT(SANDBOX_MODE_B, sandbox_get_mode());

    int result = sandbox_set_mode(SANDBOX_MODE_A);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(SANDBOX_MODE_A, sandbox_get_mode());
}

/**
 * @brief Test de validación de ruta de plugin válida
 */
void test_sandbox_validate_plugin_path_valid(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // Crear un archivo de prueba en el directorio de plugins
    char test_plugin[512];
    snprintf(test_plugin, sizeof(test_plugin), "%stest.so", TEST_PLUGINS_DIR);

    FILE* f = fopen(test_plugin, "w");
    if (f)
    {
        fprintf(f, "test");
        fclose(f);

        int result = sandbox_validate_plugin_path(test_plugin);
        TEST_ASSERT_EQUAL_INT(0, result);

        unlink(test_plugin);
    }
}

/**
 * @brief Test de validación de ruta de plugin inválida (fuera del directorio)
 */
void test_sandbox_validate_plugin_path_invalid(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // Intentar validar un archivo fuera del directorio de plugins
    int result = sandbox_validate_plugin_path("/tmp/malicious.so");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test de validación de ruta de entrada válida
 */
void test_sandbox_validate_input_path_valid(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // Crear un archivo de entrada en el directorio de trabajo
    char test_input[512];
    snprintf(test_input, sizeof(test_input), "%sinput.txt", TEST_WORK_DIR);

    FILE* f = fopen(test_input, "w");
    if (f)
    {
        fprintf(f, "test data");
        fclose(f);

        int result = sandbox_validate_input_path(test_input);
        TEST_ASSERT_EQUAL_INT(0, result);

        unlink(test_input);
    }
}

/**
 * @brief Test de validación de ruta de entrada inválida
 */
void test_sandbox_validate_input_path_invalid(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // Intentar validar un archivo fuera del directorio de trabajo
    int result = sandbox_validate_input_path("/etc/passwd");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test de conversión de resultado a string
 */
void test_sandbox_result_to_string(void)
{
    const char* str;

    str = sandbox_result_to_string(SANDBOX_SUCCESS);
    TEST_ASSERT_EQUAL_STRING("SUCCESS", str);

    str = sandbox_result_to_string(SANDBOX_ERROR_INVALID_PATH);
    TEST_ASSERT_EQUAL_STRING("INVALID_PATH", str);

    str = sandbox_result_to_string(SANDBOX_ERROR_PLUGIN_NOT_FOUND);
    TEST_ASSERT_EQUAL_STRING("PLUGIN_NOT_FOUND", str);

    str = sandbox_result_to_string(SANDBOX_ERROR_TIMEOUT);
    TEST_ASSERT_EQUAL_STRING("TIMEOUT", str);
}

/**
 * @brief Test de conversión de modo a string
 */
void test_sandbox_mode_to_string(void)
{
    const char* str;

    str = sandbox_mode_to_string(SANDBOX_MODE_A);
    TEST_ASSERT_TRUE(strstr(str, "Mode A") != NULL);

    str = sandbox_mode_to_string(SANDBOX_MODE_B);
    TEST_ASSERT_TRUE(strstr(str, "Mode B") != NULL);
}

/**
 * @brief Test de listado de plugins (directorio vacío)
 */
void test_sandbox_list_plugins_empty(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    plugin_info_t plugins[10];
    int count = sandbox_list_plugins(plugins, 10);

    TEST_ASSERT_EQUAL_INT(0, count);
}

/**
 * @brief Test de listado de plugins con archivos
 */
void test_sandbox_list_plugins_with_files(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // Crear archivos de prueba
    char plugin1[512], plugin2[512];
    snprintf(plugin1, sizeof(plugin1), "%splugin1.so", TEST_PLUGINS_DIR);
    snprintf(plugin2, sizeof(plugin2), "%splugin2.so", TEST_PLUGINS_DIR);

    FILE* f1 = fopen(plugin1, "w");
    FILE* f2 = fopen(plugin2, "w");

    if (f1 && f2)
    {
        fprintf(f1, "plugin1 content");
        fprintf(f2, "plugin2 content");
        fclose(f1);
        fclose(f2);

        plugin_info_t plugins[10];
        int count = sandbox_list_plugins(plugins, 10);

        TEST_ASSERT_EQUAL_INT(2, count);
        TEST_ASSERT_TRUE(strlen(plugins[0].name) > 0);
        TEST_ASSERT_TRUE(strlen(plugins[1].name) > 0);

        unlink(plugin1);
        unlink(plugin2);
    }
}

/**
 * @brief Test de logging de ejecución
 */
void test_sandbox_log_execution(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    sandbox_execution_t execution = {0};
    execution.pid = 12345;
    execution.exit_code = 0;
    execution.result = SANDBOX_SUCCESS;
    execution.start_time = time(NULL);
    execution.end_time = execution.start_time + 2;
    execution.stdout_size = 10;
    execution.stderr_size = 0;

    int result = sandbox_log_execution("test_plugin.so", &execution);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verificar que el archivo de log existe
    struct stat st;
    result = stat(TEST_LOG_FILE, &st);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de logging de error
 */
void test_sandbox_log_error(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    int result = sandbox_log_error("bad_plugin.so", SANDBOX_ERROR_PLUGIN_NOT_FOUND, "Plugin file not found");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verificar que el archivo de log existe
    struct stat st;
    result = stat(TEST_LOG_FILE, &st);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de ejecución de plugin inexistente
 */
void test_sandbox_execute_plugin_not_found(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    sandbox_execution_t execution;
    int result = sandbox_execute_plugin("nonexistent.so", NULL, &execution);

    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_INT(SANDBOX_ERROR_INVALID_PATH, execution.result);
}

/**
 * @brief Test de ejecución con NULL pointer
 */
void test_sandbox_execute_plugin_null_params(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    int result = sandbox_execute_plugin(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test de dlopen wrapper con NULL
 */
void test_sandbox_dlopen_null(void)
{
    void* handle = sandbox_dlopen(NULL);
    TEST_ASSERT_NULL(handle);
}

/**
 * @brief Test de dump de configuración
 */
void test_sandbox_dump_config(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    config.cpu_limit = 30;
    config.file_size_limit = 1048576;
    config.max_fds = 32;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // No debe crashear
    sandbox_dump_config(NULL);
    TEST_PASS();
}

/**
 * @brief Test de dump de ejecución
 */
void test_sandbox_dump_execution(void)
{
    sandbox_execution_t execution = {0};
    execution.pid = 12345;
    execution.exit_code = 0;
    execution.result = SANDBOX_SUCCESS;
    execution.start_time = time(NULL);
    execution.end_time = execution.start_time + 5;
    execution.stdout_size = 50;
    execution.stderr_size = 10;
    snprintf(execution.stdout_data, sizeof(execution.stdout_data), "Test output");
    snprintf(execution.stderr_data, sizeof(execution.stderr_data), "Test error");

    // No debe crashear
    sandbox_dump_execution(&execution);
    TEST_PASS();
}

/**
 * @brief Test de cleanup del sandbox
 */
void test_sandbox_cleanup(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    sandbox_init(&config);

    // No debe crashear
    sandbox_cleanup();
    TEST_PASS();
}

/**
 * @brief Test de verificación de setup básico
 */
void test_sandbox_setup_environment(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    config.cpu_limit = 10;
    config.file_size_limit = 1024 * 1024;
    config.max_fds = 32;
    config.memory_limit = 64 * 1024 * 1024;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    int result = sandbox_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);

    // El entorno debe estar configurado correctamente
    TEST_ASSERT_EQUAL_INT(SANDBOX_MODE_B, sandbox_get_mode());
}

/**
 * @brief Test de múltiples inicializaciones
 */
void test_sandbox_multiple_init(void)
{
    sandbox_config_t config = {0};
    config.mode = SANDBOX_MODE_B;
    strncpy(config.plugins_dir, TEST_PLUGINS_DIR, sizeof(config.plugins_dir) - 1);
    strncpy(config.work_dir, TEST_WORK_DIR, sizeof(config.work_dir) - 1);
    strncpy(config.log_file, TEST_LOG_FILE, sizeof(config.log_file) - 1);

    int result1 = sandbox_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result1);

    // Segunda inicialización (puede retornar 0 si detecta que ya está inicializado)
    int result2 = sandbox_init(&config);
    TEST_ASSERT_TRUE(result2 == 0 || result2 == -1);
}

/**
 * @brief Función principal
 */
int main(void)
{
    UNITY_BEGIN();

    // Tests de inicialización
    RUN_TEST(test_sandbox_detect_capabilities);
    RUN_TEST(test_sandbox_init_basic);
    RUN_TEST(test_sandbox_init_defaults);
    RUN_TEST(test_sandbox_multiple_init);

    // Tests de configuración
    RUN_TEST(test_sandbox_set_mode);
    RUN_TEST(test_sandbox_dump_config);

    // Tests de validación de rutas
    RUN_TEST(test_sandbox_validate_plugin_path_valid);
    RUN_TEST(test_sandbox_validate_plugin_path_invalid);
    RUN_TEST(test_sandbox_validate_input_path_valid);
    RUN_TEST(test_sandbox_validate_input_path_invalid);

    // Tests de conversión y utilidades
    RUN_TEST(test_sandbox_result_to_string);
    RUN_TEST(test_sandbox_mode_to_string);

    // Tests de listado de plugins
    RUN_TEST(test_sandbox_list_plugins_empty);
    RUN_TEST(test_sandbox_list_plugins_with_files);

    // Tests de logging
    RUN_TEST(test_sandbox_log_execution);
    RUN_TEST(test_sandbox_log_error);

    // Tests de ejecución (casos de error)
    RUN_TEST(test_sandbox_execute_plugin_not_found);
    RUN_TEST(test_sandbox_execute_plugin_null_params);
    RUN_TEST(test_sandbox_dlopen_null);

    // Tests de dump y cleanup
    RUN_TEST(test_sandbox_dump_execution);
    RUN_TEST(test_sandbox_cleanup);

    // Test de setup
    RUN_TEST(test_sandbox_setup_environment);

    return UNITY_END();
}
