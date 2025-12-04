/**
 * @file memory_manager.c
 * @brief Implementación del gestor de memoria simulado
 */

#include "memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// Heap global simulado
static uint8_t g_heap_buffer[HEAP_SIZE];
static heap_control_t g_heap_control;
static int g_heap_initialized = 0;

/**
 * @brief Calcula un checksum simple para un bloque
 */
uint32_t mem_calculate_checksum(block_header_t* block)
{
    if (!block) return 0;
    
    uint32_t checksum = block->magic;
    checksum ^= (uint32_t)block->size;
    checksum ^= (uint32_t)block->state;
    checksum ^= (uint32_t)(uintptr_t)block->next;
    checksum ^= (uint32_t)(uintptr_t)block->prev;
    
    return checksum;
}

/**
 * @brief Inicializa el heap simulado
 */
int mem_init(alloc_strategy_t strategy)
{
    if (g_heap_initialized) {
        return 0; // Ya inicializado
    }
    
    // Limpiar el buffer del heap
    memset(g_heap_buffer, 0, HEAP_SIZE);
    
    // Configurar control del heap
    g_heap_control.heap_start = g_heap_buffer;
    g_heap_control.heap_end = g_heap_buffer + HEAP_SIZE;
    g_heap_control.heap_size = HEAP_SIZE;
    g_heap_control.strategy = strategy;
    g_heap_control.alloc_count = 0;
    g_heap_control.free_count = 0;
    
    // Crear el primer bloque libre que ocupa todo el heap
    block_header_t* initial_block = (block_header_t*)g_heap_buffer;
    initial_block->magic = BLOCK_MAGIC;
    initial_block->size = HEAP_SIZE - sizeof(block_header_t);
    initial_block->state = BLOCK_FREE;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    initial_block->checksum = mem_calculate_checksum(initial_block);
    
    g_heap_control.free_list = initial_block;
    g_heap_control.used_memory = 0;
    g_heap_control.free_memory = initial_block->size;
    
    g_heap_initialized = 1;
    
    printf("Memory manager initialized with %s strategy\n",
           strategy == ALLOC_FIRST_FIT ? "First Fit" : "Best Fit");
    printf("Heap size: %zu bytes\n", (size_t)HEAP_SIZE);
    
    return 0;
}

/**
 * @brief Encuentra un bloque libre usando First Fit
 */
block_header_t* mem_find_free_block_first_fit(size_t size)
{
    block_header_t* current = g_heap_control.free_list;
    
    while (current != NULL) {
        if (current->state == BLOCK_FREE && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL; // No se encontró bloque adecuado
}

/**
 * @brief Encuentra un bloque libre usando Best Fit
 */
block_header_t* mem_find_free_block_best_fit(size_t size)
{
    block_header_t* current = g_heap_control.free_list;
    block_header_t* best_block = NULL;
    size_t best_size = SIZE_MAX;
    
    while (current != NULL) {
        if (current->state == BLOCK_FREE && 
            current->size >= size && 
            current->size < best_size) {
            best_block = current;
            best_size = current->size;
        }
        current = current->next;
    }
    
    return best_block;
}

/**
 * @brief Divide un bloque en dos si es lo suficientemente grande
 */
static void split_block(block_header_t* block, size_t size)
{
    if (block->size < size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
        return; // No vale la pena dividir
    }
    
    // Crear nuevo bloque libre con el espacio restante
    block_header_t* new_block = (block_header_t*)((uint8_t*)block + sizeof(block_header_t) + size);
    new_block->magic = BLOCK_MAGIC;
    new_block->size = block->size - size - sizeof(block_header_t);
    new_block->state = BLOCK_FREE;
    new_block->next = block->next;
    new_block->prev = block;
    new_block->checksum = mem_calculate_checksum(new_block);
    
    // Actualizar bloque original
    block->size = size;
    block->next = new_block;
    block->checksum = mem_calculate_checksum(block);
    
    // Actualizar enlaces de la lista libre
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
}

/**
 * @brief Remueve un bloque de la lista de bloques libres
 */
static void remove_from_free_list(block_header_t* block)
{
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        // Es el primer bloque de la lista libre
        g_heap_control.free_list = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    block->next = NULL;
    block->prev = NULL;
}

/**
 * @brief Agrega un bloque a la lista de bloques libres
 */
static void add_to_free_list(block_header_t* block)
{
    block->next = g_heap_control.free_list;
    block->prev = NULL;
    
    if (g_heap_control.free_list) {
        g_heap_control.free_list->prev = block;
    }
    
    g_heap_control.free_list = block;
}

/**
 * @brief Asigna un bloque de memoria
 */
void* mem_alloc(size_t size)
{
    if (!g_heap_initialized) {
        fprintf(stderr, "Memory manager not initialized\n");
        return NULL;
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // Alinear tamaño a múltiplo de 8 bytes
    size = (size + 7U) & ~((size_t)7U);
    
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }
    
    block_header_t* block = NULL;
    
    // Buscar bloque según la estrategia
    if (g_heap_control.strategy == ALLOC_FIRST_FIT) {
        block = mem_find_free_block_first_fit(size);
    } else {
        block = mem_find_free_block_best_fit(size);
    }
    
    if (!block) {
        fprintf(stderr, "Memory allocation failed: no suitable block found for size %zu\n", size);
        return NULL;
    }
    
    // Dividir bloque si es necesario
    split_block(block, size);
    
    // Marcar como asignado
    block->state = BLOCK_ALLOCATED;
    
    // Remover de la lista libre
    remove_from_free_list(block);
    
    // Actualizar estadísticas
    g_heap_control.used_memory += block->size;
    g_heap_control.free_memory -= block->size;
    g_heap_control.alloc_count++;
    
    // Actualizar checksum
    block->checksum = mem_calculate_checksum(block);
    
    // Retornar puntero al área de datos (después del header)
    return (uint8_t*)block + sizeof(block_header_t);
}

/**
 * @brief Obtiene el header de un bloque a partir del puntero de datos
 */
static block_header_t* get_block_header(void* ptr)
{
    if (!ptr) return NULL;
    
    block_header_t* block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    
    // Validar que está dentro del heap
    if ((uint8_t*)block < g_heap_buffer || 
        (uint8_t*)block >= g_heap_buffer + HEAP_SIZE) {
        return NULL;
    }
    
    // Validar magic number
    if (block->magic != BLOCK_MAGIC) {
        return NULL;
    }
    
    return block;
}

/**
 * @brief Encuentra bloques adyacentes para coalescing
 */
static block_header_t* find_next_block(block_header_t* block)
{
    uint8_t* next_addr = (uint8_t*)block + sizeof(block_header_t) + block->size;
    
    if (next_addr >= g_heap_buffer + HEAP_SIZE) {
        return NULL; // Fuera del heap
    }
    
    block_header_t* next_block = (block_header_t*)next_addr;
    
    if (next_block->magic != BLOCK_MAGIC) {
        return NULL; // Bloque inválido
    }
    
    return next_block;
}

/**
 * @brief Realiza coalescing de bloques libres adyacentes
 */
int mem_coalesce_free_blocks(void)
{
    if (!g_heap_initialized) {
        return -1;
    }
    
    int coalesced_count = 0;
    block_header_t* current = g_heap_control.free_list;
    
    while (current != NULL) {
        if (current->state != BLOCK_FREE) {
            current = current->next;
            continue;
        }
        
        block_header_t* next = find_next_block(current);
        
        if (next && next->state == BLOCK_FREE) {
            // Coalescer bloques
            current->size += sizeof(block_header_t) + next->size;
            
            // Remover el bloque siguiente de la lista libre
            remove_from_free_list(next);
            
            // Actualizar checksum
            current->checksum = mem_calculate_checksum(current);
            
            coalesced_count++;
            
            // No avanzar current para permitir múltiples coalescing
            continue;
        }
        
        current = current->next;
    }
    
    return coalesced_count;
}

/**
 * @brief Libera un bloque de memoria
 */
int mem_free(void* ptr)
{
    if (!g_heap_initialized) {
        fprintf(stderr, "Memory manager not initialized\n");
        return -1;
    }
    
    if (!ptr) {
        return 0; // free(NULL) es válido
    }
    
    block_header_t* block = get_block_header(ptr);
    
    if (!block) {
        fprintf(stderr, "Invalid pointer in mem_free\n");
        return -1;
    }
    
    if (block->state != BLOCK_ALLOCATED) {
        fprintf(stderr, "Double free detected\n");
        return -1;
    }
    
    // Marcar como libre
    block->state = BLOCK_FREE;
    
    // Agregar a la lista libre
    add_to_free_list(block);
    
    // Actualizar estadísticas
    g_heap_control.used_memory -= block->size;
    g_heap_control.free_memory += block->size;
    g_heap_control.free_count++;
    
    // Actualizar checksum
    block->checksum = mem_calculate_checksum(block);
    
    // Intentar coalescing
    mem_coalesce_free_blocks();
    
    return 0;
}

/**
 * @brief Valida la integridad del heap
 */
int mem_validate_heap(void)
{
    if (!g_heap_initialized) {
        return -1;
    }
    
    size_t calculated_free = 0;
    size_t calculated_used = 0;
    int free_blocks = 0;
    int allocated_blocks = 0;
    
    // Recorrer toda la memoria del heap
    uint8_t* current_addr = g_heap_buffer;
    
    while (current_addr < g_heap_buffer + HEAP_SIZE) {
        block_header_t* block = (block_header_t*)current_addr;
        
        // Validar magic number
        if (block->magic != BLOCK_MAGIC) {
            fprintf(stderr, "Invalid magic number at %p\n", (void*)block);
            return -1;
        }
        
        // Validar checksum
        uint32_t expected_checksum = mem_calculate_checksum(block);
        if (block->checksum != expected_checksum) {
            fprintf(stderr, "Checksum mismatch at %p\n", (void*)block);
            return -1;
        }
        
        // Validar tamaño
        if (block->size == 0 || 
            current_addr + sizeof(block_header_t) + block->size > g_heap_buffer + HEAP_SIZE) {
            fprintf(stderr, "Invalid block size at %p\n", (void*)block);
            return -1;
        }
        
        // Actualizar estadísticas
        if (block->state == BLOCK_FREE) {
            calculated_free += block->size;
            free_blocks++;
        } else if (block->state == BLOCK_ALLOCATED) {
            calculated_used += block->size;
            allocated_blocks++;
        } else {
            fprintf(stderr, "Invalid block state at %p\n", (void*)block);
            return -1;
        }
        
        current_addr += sizeof(block_header_t) + block->size;
    }
    
    // Verificar que las estadísticas coincidan
    if (calculated_free != g_heap_control.free_memory ||
        calculated_used != g_heap_control.used_memory) {
        fprintf(stderr, "Statistics mismatch: calculated free=%zu used=%zu, stored free=%zu used=%zu\n",
                calculated_free, calculated_used, 
                g_heap_control.free_memory, g_heap_control.used_memory);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Valida un puntero específico
 */
int mem_validate_pointer(void* ptr)
{
    if (!ptr) return 0; // NULL es válido
    
    block_header_t* block = get_block_header(ptr);
    return block ? 0 : -1;
}

/**
 * @brief Obtiene estadísticas del heap
 */
int mem_get_stats(heap_stats_t* stats)
{
    if (!g_heap_initialized || !stats) {
        return -1;
    }
    
    memset(stats, 0, sizeof(heap_stats_t));
    
    stats->total_size = g_heap_control.heap_size;
    stats->used_size = g_heap_control.used_memory;
    stats->free_size = g_heap_control.free_memory;
    
    // Calcular estadísticas adicionales
    size_t largest_free = 0;
    size_t free_count = 0;
    size_t allocated_count = 0;
    
    block_header_t* current = g_heap_control.free_list;
    while (current) {
        if (current->state == BLOCK_FREE) {
            free_count++;
            if (current->size > largest_free) {
                largest_free = current->size;
            }
        }
        current = current->next;
    }
    
    // Contar bloques asignados recorriendo todo el heap
    uint8_t* addr = g_heap_buffer;
    while (addr < g_heap_buffer + HEAP_SIZE) {
        block_header_t* block = (block_header_t*)addr;
        if (block->magic == BLOCK_MAGIC && block->state == BLOCK_ALLOCATED) {
            allocated_count++;
        }
        addr += sizeof(block_header_t) + block->size;
    }
    
    stats->largest_free_block = largest_free;
    stats->free_block_count = free_count;
    stats->allocated_block_count = allocated_count;
    
    // Calcular fragmentación externa
    if (stats->free_size > 0) {
        stats->fragmentation_ratio = 1.0 - ((double)largest_free / (double)stats->free_size);
    } else {
        stats->fragmentation_ratio = 0.0;
    }
    
    return 0;
}

/**
 * @brief Muestra el estado completo del heap
 */
void mem_dump_state(void)
{
    if (!g_heap_initialized) {
        printf("Memory manager not initialized\n");
        return;
    }
    
    printf("\n=== MEMORY HEAP STATE DUMP ===\n");
    printf("Strategy: %s\n", 
           g_heap_control.strategy == ALLOC_FIRST_FIT ? "First Fit" : "Best Fit");
    printf("Total heap size: %zu bytes\n", g_heap_control.heap_size);
    printf("Used memory: %zu bytes\n", g_heap_control.used_memory);
    printf("Free memory: %zu bytes\n", g_heap_control.free_memory);
    printf("Allocations: %u\n", g_heap_control.alloc_count);
    printf("Deallocations: %u\n", g_heap_control.free_count);
    
    heap_stats_t stats;
    if (mem_get_stats(&stats) == 0) {
        printf("Largest free block: %zu bytes\n", stats.largest_free_block);
        printf("Free blocks: %zu\n", stats.free_block_count);
        printf("Allocated blocks: %zu\n", stats.allocated_block_count);
        printf("External fragmentation: %.2f%%\n", stats.fragmentation_ratio * 100.0);
    }
    
    printf("\n=== HEAP LAYOUT ===\n");
    
    uint8_t* addr = g_heap_buffer;
    int block_num = 0;
    
    while (addr < g_heap_buffer + HEAP_SIZE) {
        block_header_t* block = (block_header_t*)addr;
        
        if (block->magic != BLOCK_MAGIC) {
            printf("ERROR: Invalid block at offset %td\n", (ptrdiff_t)(addr - g_heap_buffer));
            break;
        }
        
        size_t offset = (size_t)(addr - g_heap_buffer);
        printf("Block %d: offset=%zu, size=%zu, state=%s\n",
               block_num++, offset, block->size,
               block->state == BLOCK_FREE ? "FREE" : "ALLOCATED");
        
        addr += sizeof(block_header_t) + block->size;
    }
    
    printf("\n=== FREE LIST ===\n");
    block_header_t* free_block = g_heap_control.free_list;
    int free_num = 0;
    
    while (free_block) {
        size_t offset = (size_t)((uint8_t*)free_block - g_heap_buffer);
        printf("Free block %d: offset=%zu, size=%zu\n",
               free_num++, offset, free_block->size);
        free_block = free_block->next;
    }
    
    printf("=============================\n\n");
}

/**
 * @brief Cambia la estrategia de asignación
 */
int mem_set_strategy(alloc_strategy_t strategy)
{
    if (!g_heap_initialized) {
        return -1;
    }
    
    g_heap_control.strategy = strategy;
    printf("Memory allocation strategy changed to %s\n",
           strategy == ALLOC_FIRST_FIT ? "First Fit" : "Best Fit");
    
    return 0;
}

/**
 * @brief Obtiene la estrategia actual
 */
alloc_strategy_t mem_get_strategy(void)
{
    return g_heap_control.strategy;
}

/**
 * @brief Limpia y destruye el heap
 */
void mem_cleanup(void)
{
    if (g_heap_initialized) {
        memset(&g_heap_control, 0, sizeof(heap_control_t));
        memset(g_heap_buffer, 0, HEAP_SIZE);
        g_heap_initialized = 0;
        printf("Memory manager cleaned up\n");
    }
}