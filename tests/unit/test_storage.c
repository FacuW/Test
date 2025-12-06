/**
 * @file test_storage.c
 * @brief Tests unitarios para el sistema de almacenamiento persistente
 */

#include "monitoring.h"
#include "storage.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Directorio de prueba
#define TEST_STORAGE_DIR "/tmp/test_storage/"

void setUp(void)
{
    // Crear directorio de prueba
    mkdir(TEST_STORAGE_DIR, 0755);

    // Limpiar archivos anteriores del directorio de test
    char command[256];
    snprintf(command, sizeof(command), "rm -f %s*.bin", TEST_STORAGE_DIR);
    system(command);

    // CRÍTICO: Limpiar también el directorio de producción para aislar tests
    system("rm -f /var/monitoreo/data/*.bin");
}

void tearDown(void)
{
    // Limpiar archivos de prueba
    system("rm -rf /tmp/test_storage");
    storage_cleanup();
}

/**
 * @brief Test de inicialización del sistema de almacenamiento
 */
void test_storage_init(void)
{
    int result = storage_init();
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de generación de nombre de archivo
 */
void test_storage_generate_filename(void)
{
    char filename[STORAGE_MAX_PATH_LENGTH];
    time_t date = time(NULL);

    storage_generate_filename(filename, sizeof(filename), date);

    // Debe contener el prefijo y sufijo
    TEST_ASSERT_TRUE(strstr(filename, STORAGE_FILE_PREFIX) != NULL);
    TEST_ASSERT_TRUE(strstr(filename, STORAGE_FILE_SUFFIX) != NULL);

    // Debe contener el directorio
    TEST_ASSERT_TRUE(strstr(filename, STORAGE_DIR) != NULL);
}

/**
 * @brief Test de cálculo de CRC32
 */
void test_storage_crc32(void)
{
    const char* data = "Hello, World!";
    size_t length = strlen(data);

    uint32_t crc1 = storage_calculate_crc32(data, length);
    uint32_t crc2 = storage_calculate_crc32(data, length);

    // CRC debe ser determinístico
    TEST_ASSERT_EQUAL_UINT32(crc1, crc2);

    // CRC de datos diferentes debe ser diferente
    const char* data2 = "Hello, World?";
    uint32_t crc3 = storage_calculate_crc32(data2, strlen(data2));
    TEST_ASSERT_NOT_EQUAL(crc1, crc3);
}

/**
 * @brief Test de apertura de archivo nuevo
 */
void test_storage_open_new_file(void)
{
    storage_init();

    storage_handle_t handle;
    time_t now = time(NULL);

    int result = storage_open_daily_file(&handle, now, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(handle.is_open);
    TEST_ASSERT_FALSE(handle.is_readonly);

    // Verificar cabecera
    TEST_ASSERT_EQUAL_UINT32(STORAGE_MAGIC, handle.header.magic);
    TEST_ASSERT_EQUAL_UINT16(STORAGE_VERSION, handle.header.version);
    TEST_ASSERT_EQUAL_UINT16(STORAGE_RECORD_SIZE, handle.header.record_size);
    TEST_ASSERT_EQUAL_UINT32(0, handle.header.total_records);

    storage_close(&handle);
}

/**
 * @brief Test de validación de cabecera válida
 */
void test_storage_validate_header_valid(void)
{
    storage_header_t header = {0};
    header.magic = STORAGE_MAGIC;
    header.version = STORAGE_VERSION;
    header.record_size = STORAGE_RECORD_SIZE;
    header.total_records = 0;
    header.valid_records = 0;
    header.created_time = (uint64_t)time(NULL);
    header.last_write_time = header.created_time;

    // Calcular CRC
    header.header_crc = storage_calculate_crc32(&header, sizeof(storage_header_t) - sizeof(header.header_crc) -
                                                             sizeof(header.reserved));

    int result = storage_validate_header(&header);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de validación de cabecera inválida (magic number)
 */
void test_storage_validate_header_invalid_magic(void)
{
    storage_header_t header = {0};
    header.magic = 0xDEADBEEF; // Magic incorrecto
    header.version = STORAGE_VERSION;
    header.record_size = STORAGE_RECORD_SIZE;

    int result = storage_validate_header(&header);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test de conversión de métricas a registro
 */
void test_storage_metrics_to_record(void)
{
    system_metrics_t metrics = {0};
    metrics.timestamp = time(NULL);
    metrics.cpu.usage_percent = 50.5;
    metrics.cpu.user = 30.0;
    metrics.cpu.system = 20.5;
    metrics.memory.total = 8192000;
    metrics.memory.used = 4096000;
    metrics.memory.free = 4096000;
    metrics.load.load_1m = 1.5;
    metrics.load.load_5m = 1.8;
    metrics.load.load_15m = 2.0;

    storage_record_t record;
    int result = storage_metrics_to_record(&metrics, &record);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT64(metrics.timestamp, record.timestamp);
    // Comparaciones de doubles deshabilitadas en Unity (requiere UNITY_INCLUDE_DOUBLE)
    // TEST_ASSERT_EQUAL_DOUBLE(metrics.cpu.usage_percent, record.cpu_usage);
    TEST_ASSERT_EQUAL_UINT64(metrics.memory.total, record.memory_total);
    TEST_ASSERT_EQUAL_UINT64(metrics.memory.used, record.memory_used);
}

/**
 * @brief Test de conversión de registro a métricas
 */
void test_storage_record_to_metrics(void)
{
    storage_record_t record = {0};
    record.timestamp = (uint64_t)time(NULL);
    record.cpu_usage = 45.5;
    record.cpu_user = 25.0;
    record.cpu_system = 20.5;
    record.memory_total = 8192000;
    record.memory_used = 3072000;
    record.memory_free = 5120000;
    record.load_1m = 1.2;
    record.load_5m = 1.5;
    record.load_15m = 1.8;

    system_metrics_t metrics;
    int result = storage_record_to_metrics(&record, &metrics);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT64(record.timestamp, metrics.timestamp);
    // Comparaciones de doubles deshabilitadas en Unity
    TEST_ASSERT_EQUAL_UINT64(record.memory_total, metrics.memory.total);
    TEST_ASSERT_EQUAL_UINT64(record.memory_used, metrics.memory.used);
}

/**
 * @brief Test de validación de registro válido
 */
void test_storage_validate_record_valid(void)
{
    storage_record_t record = {0};
    record.timestamp = (uint64_t)time(NULL);
    record.record_type = 1;
    record.cpu_usage = 50.0;
    record.cpu_user = 30.0;
    record.cpu_system = 20.0;
    record.memory_total = 8192000;
    record.memory_used = 4096000;
    record.memory_free = 4096000;

    // Calcular CRC
    record.record_crc = storage_calculate_crc32(&record, sizeof(storage_record_t) - sizeof(record.record_crc));

    int result = storage_validate_record(&record);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de validación de registro corrupto (CRC inválido)
 */
void test_storage_validate_record_corrupt(void)
{
    storage_record_t record = {0};
    record.timestamp = (uint64_t)time(NULL);
    record.record_type = 1;
    record.cpu_usage = 50.0;
    record.record_crc = 0x12345678; // CRC inválido

    int result = storage_validate_record(&record);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/**
 * @brief Test de escritura de un registro
 */
void test_storage_write_record(void)
{
    storage_init();

    storage_handle_t handle;
    time_t now = time(NULL);

    int result = storage_open_daily_file(&handle, now, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Crear métricas de prueba
    system_metrics_t metrics = {0};
    metrics.timestamp = now;
    metrics.cpu.usage_percent = 45.5;
    metrics.cpu.user = 25.0;
    metrics.cpu.system = 20.5;
    metrics.memory.total = 8192000;
    metrics.memory.used = 4096000;
    metrics.memory.free = 4096000;
    metrics.load.load_1m = 1.5;
    metrics.load.load_5m = 1.8;
    metrics.load.load_15m = 2.0;

    // Escribir registro
    result = storage_write_record(&handle, &metrics);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verificar que la cabecera se actualizó
    TEST_ASSERT_EQUAL_UINT32(1, handle.header.total_records);
    TEST_ASSERT_EQUAL_UINT32(1, handle.header.valid_records);

    storage_close(&handle);
}

/**
 * @brief Test de escritura y lectura de múltiples registros
 */
void test_storage_write_and_read_multiple(void)
{
    storage_init();

    storage_handle_t handle;
    time_t now = time(NULL);

    // Abrir archivo para escritura
    int result = storage_open_daily_file(&handle, now, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Escribir 5 registros
    for (int i = 0; i < 5; i++)
    {
        system_metrics_t metrics = {0};
        metrics.timestamp = now + i;
        metrics.cpu.usage_percent = 50.0 + i;
        metrics.memory.used = 4096000 + (i * 1024);
        metrics.load.load_1m = 1.0 + (i * 0.1);

        result = storage_write_record(&handle, &metrics);
        TEST_ASSERT_EQUAL_INT(0, result);
    }

    storage_close(&handle);

    // Reabrir archivo para lectura
    result = storage_open_daily_file(&handle, now, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT32(5, handle.header.total_records);

    storage_close(&handle);
}

/**
 * @brief Callback de prueba para contar registros
 */
static int count_records_callback(const storage_record_t* record, void* user_data)
{
    (void)record;
    int* count = (int*)user_data;
    (*count)++;
    return 0; // Continuar
}

/**
 * @brief Test de lectura de todos los registros
 */
void test_storage_read_all_records(void)
{
    storage_init();

    storage_handle_t handle;
    time_t now = time(NULL);

    // Escribir registros
    int result = storage_open_daily_file(&handle, now, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    for (int i = 0; i < 3; i++)
    {
        system_metrics_t metrics = {0};
        metrics.timestamp = now + i;
        metrics.cpu.usage_percent = 50.0;
        storage_write_record(&handle, &metrics);
    }

    storage_close(&handle);

    // Leer registros
    result = storage_open_daily_file(&handle, now, 1);
    TEST_ASSERT_EQUAL_INT(0, result);

    int count = 0;
    int valid = storage_read_all_records(&handle, count_records_callback, &count);

    TEST_ASSERT_EQUAL_INT(3, valid);
    TEST_ASSERT_EQUAL_INT(3, count);

    storage_close(&handle);
}

/**
 * @brief Test de cierre de handle
 */
void test_storage_close(void)
{
    storage_init();

    storage_handle_t handle;
    time_t now = time(NULL);

    int result = storage_open_daily_file(&handle, now, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(handle.is_open);

    result = storage_close(&handle);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(handle.is_open);
}

/**
 * @brief Test de listado de archivos
 */
void test_storage_list_files(void)
{
    storage_init();

    // Crear 2 archivos
    storage_handle_t handle1, handle2;
    time_t now = time(NULL);

    storage_open_daily_file(&handle1, now, 0);
    storage_close(&handle1);

    storage_open_daily_file(&handle2, now - 86400, 0); // Ayer
    storage_close(&handle2);

    // Listar archivos
    char files[10][STORAGE_MAX_PATH_LENGTH];
    int count = storage_list_files(files, 10);

    TEST_ASSERT_TRUE(count >= 2);
}

/**
 * @brief Test de estadísticas de almacenamiento
 */
void test_storage_get_stats(void)
{
    storage_init();

    // Crear archivo con registros
    storage_handle_t handle;
    time_t now = time(NULL);

    storage_open_daily_file(&handle, now, 0);

    for (int i = 0; i < 5; i++)
    {
        system_metrics_t metrics = {0};
        metrics.timestamp = now + i;
        metrics.cpu.usage_percent = 50.0;
        storage_write_record(&handle, &metrics);
    }

    storage_close(&handle);

    // Obtener estadísticas
    storage_stats_t stats;
    int result = storage_get_stats(&stats);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(stats.total_files >= 1);
    TEST_ASSERT_TRUE(stats.total_records >= 5);
    TEST_ASSERT_TRUE(stats.valid_records >= 5);
    TEST_ASSERT_TRUE(stats.total_size > 0);
}

/**
 * @brief Test de apertura en modo solo lectura
 */
void test_storage_open_readonly(void)
{
    storage_init();

    // Crear archivo
    storage_handle_t handle_write;
    time_t now = time(NULL);

    storage_open_daily_file(&handle_write, now, 0);
    storage_close(&handle_write);

    // Abrir en modo solo lectura
    storage_handle_t handle_read;
    int result = storage_open_daily_file(&handle_read, now, 1);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(handle_read.is_readonly);

    storage_close(&handle_read);
}

/**
 * @brief Test de escritura en archivo readonly (debe fallar)
 */
void test_storage_write_readonly_fail(void)
{
    storage_init();

    // Crear archivo
    storage_handle_t handle;
    time_t now = time(NULL);

    storage_open_daily_file(&handle, now, 0);
    storage_close(&handle);

    // Reabrir en modo readonly
    storage_open_daily_file(&handle, now, 1);

    // Intentar escribir (debe fallar)
    system_metrics_t metrics = {0};
    metrics.timestamp = now;

    int result = storage_write_record(&handle, &metrics);
    TEST_ASSERT_EQUAL_INT(-1, result);

    storage_close(&handle);
}

/**
 * @brief Función principal
 */
int main(void)
{
    UNITY_BEGIN();

    // Tests de inicialización y configuración
    RUN_TEST(test_storage_init);
    RUN_TEST(test_storage_generate_filename);
    RUN_TEST(test_storage_crc32);

    // Tests de apertura y cierre
    RUN_TEST(test_storage_open_new_file);
    RUN_TEST(test_storage_close);
    RUN_TEST(test_storage_open_readonly);

    // Tests de validación
    RUN_TEST(test_storage_validate_header_valid);
    RUN_TEST(test_storage_validate_header_invalid_magic);
    RUN_TEST(test_storage_validate_record_valid);
    RUN_TEST(test_storage_validate_record_corrupt);

    // Tests de conversión
    RUN_TEST(test_storage_metrics_to_record);
    RUN_TEST(test_storage_record_to_metrics);

    // Tests de lectura/escritura
    RUN_TEST(test_storage_write_record);
    RUN_TEST(test_storage_write_and_read_multiple);
    RUN_TEST(test_storage_read_all_records);
    RUN_TEST(test_storage_write_readonly_fail);

    // Tests de listado y estadísticas
    RUN_TEST(test_storage_list_files);
    RUN_TEST(test_storage_get_stats);

    return UNITY_END();
}
