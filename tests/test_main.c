#include "unity.h"

// Funciones que Unity necesita aunque no hagas nada
void setUp(void) {}
void tearDown(void) {}

// Test de ejemplo
void test_example(void) {
    TEST_ASSERT_EQUAL_INT(2, 1+1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_example);
    return UNITY_END();
}
