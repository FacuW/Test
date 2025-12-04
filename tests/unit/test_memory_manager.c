/**
 * @file test_memory_manager.c
 * @brief Tests unitarios para el gestor de memoria
 */

#include "memory_manager.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void setUp(void)
{
    // Limpiar antes de cada test
    mem_cleanup();
}

void tearDown(void)
{
    // Limpiar después de cada test
    mem_cleanup();
}

/**
 * @brief Test de inicialización básica
 */
void test_mem_init_first_fit(void)
{
    int result = mem_init(ALLOC_FIRST_FIT);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(ALLOC_FIRST_FIT, mem_get_strategy());
}

/**
 * @brief Test de inicialización con Best Fit
 */
void test_mem_init_best_fit(void)
{
    int result = mem_init(ALLOC_BEST_FIT);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(ALLOC_BEST_FIT, mem_get_strategy());
}

/**
 * @brief Test de asignación simple
 */
void test_mem_alloc_simple(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* ptr = mem_alloc(100);
    TEST_ASSERT_NOT_NULL(ptr);

    mem_free(ptr);
}

/**
 * @brief Test de asignación de tamaño cero
 */
void test_mem_alloc_zero_size(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* ptr = mem_alloc(0);
    TEST_ASSERT_NULL(ptr);
}

/**
 * @brief Test de múltiples asignaciones
 */
void test_mem_alloc_multiple(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* p1 = mem_alloc(256);
    void* p2 = mem_alloc(512);
    void* p3 = mem_alloc(1024);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    // Los punteros deben ser diferentes
    TEST_ASSERT_NOT_EQUAL(p1, p2);
    TEST_ASSERT_NOT_EQUAL(p2, p3);
    TEST_ASSERT_NOT_EQUAL(p1, p3);

    mem_free(p1);
    mem_free(p2);
    mem_free(p3);
}

/**
 * @brief Test de liberación simple
 */
void test_mem_free_simple(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* ptr = mem_alloc(100);
    TEST_ASSERT_NOT_NULL(ptr);

    int result = mem_free(ptr);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de liberación de puntero NULL
 */
void test_mem_free_null(void)
{
    mem_init(ALLOC_FIRST_FIT);

    int result = mem_free(NULL);
    TEST_ASSERT_EQUAL_INT(0, result); // free(NULL) es válido
}

/**
 * @brief Test de coalescing de bloques adyacentes
 */
void test_mem_coalescing(void)
{
    mem_init(ALLOC_FIRST_FIT);

    // Asignar 3 bloques contiguos
    void* p1 = mem_alloc(512);
    void* p2 = mem_alloc(512);
    void* p3 = mem_alloc(512);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    // Liberar todos
    mem_free(p1);
    mem_free(p2);
    mem_free(p3);

    // Hacer coalescing explícito
    int coalesced = mem_coalesce_free_blocks();
    TEST_ASSERT_TRUE(coalesced >= 0);

    // Verificar que ahora podemos asignar un bloque grande
    void* p_large = mem_alloc(1536);
    TEST_ASSERT_NOT_NULL(p_large);

    mem_free(p_large);
}

/**
 * @brief Test de estadísticas básicas
 */
void test_mem_get_stats(void)
{
    mem_init(ALLOC_FIRST_FIT);

    heap_stats_t stats;
    int result = mem_get_stats(&stats);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE, stats.total_size);
    TEST_ASSERT_TRUE(stats.free_size > 0);
}

/**
 * @brief Test de estadísticas con asignaciones
 */
void test_mem_stats_with_allocations(void)
{
    mem_init(ALLOC_FIRST_FIT);

    heap_stats_t stats_before;
    mem_get_stats(&stats_before);

    void* ptr = mem_alloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);

    heap_stats_t stats_after;
    mem_get_stats(&stats_after);

    // La memoria usada debe haber aumentado
    TEST_ASSERT_TRUE(stats_after.used_size > stats_before.used_size);
    TEST_ASSERT_TRUE(stats_after.free_size < stats_before.free_size);

    mem_free(ptr);
}

/**
 * @brief Test de fragmentación externa
 */
void test_mem_fragmentation(void)
{
    mem_init(ALLOC_FIRST_FIT);

    // Asignar 4 bloques
    void* p1 = mem_alloc(1024);
    void* p2 = mem_alloc(1024);
    void* p3 = mem_alloc(1024);
    void* p4 = mem_alloc(1024);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);
    TEST_ASSERT_NOT_NULL(p4);

    // Liberar bloques alternados (crea fragmentación)
    mem_free(p2);
    mem_free(p4);

    heap_stats_t stats;
    mem_get_stats(&stats);

    // Debe haber al menos 2 bloques libres
    TEST_ASSERT_TRUE(stats.free_block_count >= 2);

    mem_free(p1);
    mem_free(p3);
}

/**
 * @brief Test de asignación mayor al heap disponible
 */
void test_mem_alloc_too_large(void)
{
    mem_init(ALLOC_FIRST_FIT);

    // Intentar asignar más que el heap completo
    void* ptr = mem_alloc(HEAP_SIZE + 1024);
    TEST_ASSERT_NULL(ptr);
}

/**
 * @brief Test de cambio de estrategia
 */
void test_mem_set_strategy(void)
{
    mem_init(ALLOC_FIRST_FIT);
    TEST_ASSERT_EQUAL_INT(ALLOC_FIRST_FIT, mem_get_strategy());

    int result = mem_set_strategy(ALLOC_BEST_FIT);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(ALLOC_BEST_FIT, mem_get_strategy());
}

/**
 * @brief Test de validación del heap
 */
void test_mem_validate_heap(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* p1 = mem_alloc(256);
    void* p2 = mem_alloc(512);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);

    int result = mem_validate_heap();
    TEST_ASSERT_EQUAL_INT(0, result);

    mem_free(p1);
    mem_free(p2);

    result = mem_validate_heap();
    TEST_ASSERT_EQUAL_INT(0, result);
}

/**
 * @brief Test de validación de puntero
 */
void test_mem_validate_pointer(void)
{
    mem_init(ALLOC_FIRST_FIT);

    void* valid_ptr = mem_alloc(100);
    TEST_ASSERT_NOT_NULL(valid_ptr);

    int result = mem_validate_pointer(valid_ptr);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Puntero NULL es válido
    result = mem_validate_pointer(NULL);
    TEST_ASSERT_EQUAL_INT(0, result);

    mem_free(valid_ptr);
}

/**
 * @brief Test de First Fit vs Best Fit
 */
void test_mem_first_fit_vs_best_fit(void)
{
    // Test First Fit
    mem_init(ALLOC_FIRST_FIT);

    void* p1 = mem_alloc(512);
    void* p2 = mem_alloc(1024);
    void* p3 = mem_alloc(512);

    mem_free(p2); // Libera el bloque del medio (1024 bytes)

    // First Fit debe usar este bloque
    void* p4 = mem_alloc(256);
    TEST_ASSERT_NOT_NULL(p4);

    mem_free(p1);
    mem_free(p3);
    mem_free(p4);
    mem_cleanup();

    // Test Best Fit
    mem_init(ALLOC_BEST_FIT);

    p1 = mem_alloc(512);
    p2 = mem_alloc(1024);
    p3 = mem_alloc(512);

    mem_free(p2); // Libera el bloque del medio (1024 bytes)

    // Best Fit debe buscar el mejor ajuste
    p4 = mem_alloc(256);
    TEST_ASSERT_NOT_NULL(p4);

    mem_free(p1);
    mem_free(p3);
    mem_free(p4);
}

/**
 * @brief Función principal
 */
int main(void)
{
    UNITY_BEGIN();

    // Tests básicos
    RUN_TEST(test_mem_init_first_fit);
    RUN_TEST(test_mem_init_best_fit);
    RUN_TEST(test_mem_alloc_simple);
    RUN_TEST(test_mem_alloc_zero_size);
    RUN_TEST(test_mem_alloc_multiple);

    // Tests de liberación
    RUN_TEST(test_mem_free_simple);
    RUN_TEST(test_mem_free_null);

    // Tests de coalescing
    RUN_TEST(test_mem_coalescing);

    // Tests de estadísticas
    RUN_TEST(test_mem_get_stats);
    RUN_TEST(test_mem_stats_with_allocations);

    // Tests de fragmentación
    RUN_TEST(test_mem_fragmentation);
    RUN_TEST(test_mem_alloc_too_large);

    // Tests de estrategias
    RUN_TEST(test_mem_set_strategy);
    RUN_TEST(test_mem_first_fit_vs_best_fit);

    // Tests de validación
    RUN_TEST(test_mem_validate_heap);
    RUN_TEST(test_mem_validate_pointer);

    return UNITY_END();
}
