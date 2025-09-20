/**
 * @file test_main.c
 * @brief Tests unitarios básicos
 */

#include "cjson/cJSON.h"
#include "monitoring.h"
#include "unity.h"

void setUp(void) { }
void tearDown(void) { }

/**
 * @brief Test básico para verificar que cJSON funciona correctamente
 */
void test_cjson_basic_functionality(void)
{
    cJSON* json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);

    cJSON* test_string = cJSON_CreateString("test");
    cJSON_AddItemToObject(json, "test_key", test_string);

    cJSON* retrieved = cJSON_GetObjectItem(json, "test_key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("test", cJSON_GetStringValue(retrieved));

    cJSON_Delete(json);
}

/**
 * @brief Test para verificar las constantes del proyecto
 */
void test_monitoring_constants(void)
{
    TEST_ASSERT_EQUAL_STRING("1.0.0", MONITORING_VERSION);
    TEST_ASSERT_EQUAL_INT(5, DEFAULT_INTERVAL);
}

/**
 * @brief Función principal para ejecutar los tests
 */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cjson_basic_functionality);
    RUN_TEST(test_monitoring_constants);

    return UNITY_END();
}
