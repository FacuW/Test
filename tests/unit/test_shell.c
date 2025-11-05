#include "shell.h"
#include "unity.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

// ========== TEST 1: IPC - Pipe Creation ==========
void test_pipe_creation(void)
{
    shell_context_t ctx;
    
    int result = shell_init(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que los file descriptors del pipe son válidos
    TEST_ASSERT_TRUE(ctx.pipe_fd[0] > 0);  // read end
    TEST_ASSERT_TRUE(ctx.pipe_fd[1] > 0);  // write end
    
    shell_cleanup(&ctx);
}

// ========== TEST 2: IPC - Comunicación por Pipe ==========
void test_pipe_communication(void)
{
    int pipe_fd[2];
    int result = pipe(pipe_fd);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Escribir en el pipe
    char write_msg = 'S';
    ssize_t written = write(pipe_fd[1], &write_msg, 1);
    TEST_ASSERT_EQUAL_INT(1, written);
    
    // Leer del pipe
    char read_msg;
    ssize_t read_bytes = read(pipe_fd[0], &read_msg, 1);
    TEST_ASSERT_EQUAL_INT(1, read_bytes);
    TEST_ASSERT_EQUAL_CHAR('S', read_msg);
    
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

// ========== TEST 3: Concurrencia - Mutex Initialization ==========
void test_mutex_initialization(void)
{
    shell_context_t ctx;
    
    int result = shell_init(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que el mutex funciona
    int lock_result = pthread_mutex_lock(&ctx.state_mutex);
    TEST_ASSERT_EQUAL_INT(0, lock_result);
    
    int unlock_result = pthread_mutex_unlock(&ctx.state_mutex);
    TEST_ASSERT_EQUAL_INT(0, unlock_result);
    
    shell_cleanup(&ctx);
}

// ========== TEST 4: Concurrencia - Estado Inicial ==========
void test_initial_monitor_state(void)
{
    shell_context_t ctx;
    
    int result = shell_init(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // El estado inicial debe ser STOPPED
    TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, ctx.state);
    
    shell_cleanup(&ctx);
}

// ========== TEST 5: Concurrencia - Cambio de Estado con Mutex ==========
void test_state_change_with_mutex(void)
{
    shell_context_t ctx;
    shell_init(&ctx);
    
    // Cambiar estado protegido por mutex
    pthread_mutex_lock(&ctx.state_mutex);
    ctx.state = MONITOR_RUNNING;
    pthread_mutex_unlock(&ctx.state_mutex);
    
    // Verificar el cambio
    pthread_mutex_lock(&ctx.state_mutex);
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);
    
    TEST_ASSERT_EQUAL_INT(MONITOR_RUNNING, state);
    
    shell_cleanup(&ctx);
}

// ========== TEST 6: Fork/Exec - cmd_status crea proceso ==========
void test_cmd_status_creates_process(void)
{
#ifdef GITHUB_ACTIONS
    TEST_IGNORE_MESSAGE("Skipping in CI - requires /var/lib/monitoreo/");
#endif
    
    shell_context_t ctx;
    shell_init(&ctx);
    
    // Verificar que cmd_status no crashea
    int result = cmd_status(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    shell_cleanup(&ctx);
}

// ========== TEST 7: Fork/Exec - cmd_psnode crea proceso ==========
void test_cmd_psnode_creates_process(void)
{
    shell_context_t ctx;
    shell_init(&ctx);
    
    // Verificar que cmd_psnode no crashea
    int result = cmd_psnode(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    shell_cleanup(&ctx);
}

// ========== TEST 8: Thread - cmd_start no falla ==========
void test_cmd_start_changes_state(void)
{
    shell_context_t ctx;
    shell_init(&ctx);
    
    // Iniciar monitoreo
    int result = cmd_start(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que el estado cambió
    pthread_mutex_lock(&ctx.state_mutex);
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);
    
    TEST_ASSERT_EQUAL_INT(MONITOR_RUNNING, state);
    
    // Detener para limpieza
    cmd_stop(&ctx);
    shell_cleanup(&ctx);
}

// ========== TEST 9: Thread - cmd_stop detiene thread ==========
void test_cmd_stop_stops_thread(void)
{
    shell_context_t ctx;
    shell_init(&ctx);
    
    // Iniciar y luego detener
    cmd_start(&ctx);
    usleep(100000);  // 100ms para que el thread inicie
    
    int result = cmd_stop(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verificar que el estado cambió a STOPPED
    pthread_mutex_lock(&ctx.state_mutex);
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);
    
    TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, state);
    
    shell_cleanup(&ctx);
}

// ========== TEST 10: IPC - Pipe usado en stop ==========
void test_pipe_used_in_stop(void)
{
    shell_context_t ctx;
    shell_init(&ctx);
    
    cmd_start(&ctx);
    usleep(100000);  // 100ms
    
    // cmd_stop debe escribir en el pipe
    int result = cmd_stop(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    shell_cleanup(&ctx);
}

// ========== MAIN ==========
int main(void)
{
    UNITY_BEGIN();

    // Tests de IPC
    RUN_TEST(test_pipe_creation);
    RUN_TEST(test_pipe_communication);
    RUN_TEST(test_pipe_used_in_stop);
    
    // Tests de Concurrencia (Mutex + Threads)
    RUN_TEST(test_mutex_initialization);
    RUN_TEST(test_initial_monitor_state);
    RUN_TEST(test_state_change_with_mutex);
    RUN_TEST(test_cmd_start_changes_state);
    RUN_TEST(test_cmd_stop_stops_thread);
    
    // Tests de Fork/Exec
    RUN_TEST(test_cmd_status_creates_process);
    RUN_TEST(test_cmd_psnode_creates_process);

    return UNITY_END();
}
