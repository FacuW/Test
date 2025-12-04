#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file memory_manager.h
 * @brief Gestor de memoria simulado con algoritmos First Fit y Best Fit
 */

// Configuración del heap simulado
#define HEAP_SIZE (64 * 1024)  // 64KB heap simulado
#define MIN_BLOCK_SIZE 16      // Tamaño mínimo de bloque
#define BLOCK_MAGIC 0xDEADBEEF // Magic number para validación

// Algoritmos de asignación disponibles
typedef enum
{
    ALLOC_FIRST_FIT = 0,
    ALLOC_BEST_FIT = 1
} alloc_strategy_t;

// Estados de bloque
typedef enum
{
    BLOCK_FREE = 0,
    BLOCK_ALLOCATED = 1
} block_state_t;

// Estructura de un bloque en el heap
typedef struct block_header
{
    uint32_t magic;            // Magic number para validación
    size_t size;               // Tamaño del bloque (sin incluir header)
    block_state_t state;       // Estado del bloque
    struct block_header* next; // Siguiente bloque libre (solo para bloques libres)
    struct block_header* prev; // Bloque anterior libre (solo para bloques libres)
    uint32_t checksum;         // Checksum simple para integridad
} block_header_t;

// Estructura de control del heap
typedef struct
{
    void* heap_start;          // Inicio del heap
    void* heap_end;            // Final del heap
    size_t heap_size;          // Tamaño total del heap
    size_t used_memory;        // Memoria utilizada
    size_t free_memory;        // Memoria libre
    block_header_t* free_list; // Lista de bloques libres
    alloc_strategy_t strategy; // Estrategia de asignación actual
    uint32_t alloc_count;      // Contador de asignaciones
    uint32_t free_count;       // Contador de liberaciones
} heap_control_t;

// Estadísticas del heap
typedef struct
{
    size_t total_size;            // Tamaño total
    size_t used_size;             // Memoria utilizada
    size_t free_size;             // Memoria libre
    size_t largest_free_block;    // Mayor bloque libre
    size_t free_block_count;      // Número de bloques libres
    size_t allocated_block_count; // Número de bloques asignados
    double fragmentation_ratio;   // Ratio de fragmentación externa
} heap_stats_t;

// Funciones principales del gestor de memoria
int mem_init(alloc_strategy_t strategy);
void* mem_alloc(size_t size);
int mem_free(void* ptr);
void mem_cleanup(void);

// Funciones de información y debugging
void mem_dump_state(void);
int mem_get_stats(heap_stats_t* stats);
int mem_set_strategy(alloc_strategy_t strategy);
alloc_strategy_t mem_get_strategy(void);

// Funciones de validación
int mem_validate_heap(void);
int mem_validate_pointer(void* ptr);

// Funciones internas (para testing)
block_header_t* mem_find_free_block_first_fit(size_t size);
block_header_t* mem_find_free_block_best_fit(size_t size);
int mem_coalesce_free_blocks(void);
uint32_t mem_calculate_checksum(block_header_t* block);

#endif // MEMORY_MANAGER_H
