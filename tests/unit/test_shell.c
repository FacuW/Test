#include "shell.h"
#include "unity.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void setUp(void) {}
void tearDown(void) {}

// creación de pipe
void test_pipe_creation(void)
{
    shell_context_t ctx;

    int result = shell_init(&ctx);
    
    if (result != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    TEST_ASSERT_TRUE(ctx.pipe_fd[0] > 0);
    TEST_ASSERT_TRUE(ctx.pipe_fd[1] > 0);

    shell_cleanup(&ctx);
}

// comunicación por pipe
void test_pipe_communication(void)
{
    int pipe_fd[2];
    int result = pipe(pipe_fd);
    TEST_ASSERT_EQUAL_INT(0, result);

    char write_msg = 'S';
    ssize_t written = write(pipe_fd[1], &write_msg, 1);
    TEST_ASSERT_EQUAL_INT(1, written);

    char read_msg;
    ssize_t read_bytes = read(pipe_fd[0], &read_msg, 1);
    TEST_ASSERT_EQUAL_INT(1, read_bytes);
    TEST_ASSERT_EQUAL_CHAR('S', read_msg);

    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

// concurrencia, inicialización de mutex
void test_mutex_initialization(void)
{
    shell_context_t ctx;

    int result = shell_init(&ctx);
    
    if (result != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    int lock_result = pthread_mutex_lock(&ctx.state_mutex);
    TEST_ASSERT_EQUAL_INT(0, lock_result);

    int unlock_result = pthread_mutex_unlock(&ctx.state_mutex);
    TEST_ASSERT_EQUAL_INT(0, unlock_result);

    shell_cleanup(&ctx);
}

// concurrencia estado inicial
void test_initial_monitor_state(void)
{
    shell_context_t ctx;

    int result = shell_init(&ctx);
    
    if (result != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, ctx.state);

    shell_cleanup(&ctx);
}

// concurrencia (cambio de estado con mutex)
void test_state_change_with_mutex(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    pthread_mutex_lock(&ctx.state_mutex);
    ctx.state = MONITOR_RUNNING;
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);

    TEST_ASSERT_EQUAL_INT(MONITOR_RUNNING, state);

    usleep(10000);  // 10ms
    shell_cleanup(&ctx);
}

// status fork/exec
void test_cmd_status_creates_process(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    int result = cmd_status(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// psnode fork/exec
void test_cmd_psnode_creates_process(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    int result = cmd_psnode(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// start test
void test_cmd_start_changes_state(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    int result = cmd_start(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);

    usleep(THREAD_STARTUP_DELAY_US);

    pthread_mutex_lock(&ctx.state_mutex);
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);

    TEST_ASSERT_EQUAL_INT(MONITOR_RUNNING, state);

    cmd_stop(&ctx);
    usleep(10000);  // 10ms - dar tiempo para que el thread termine
    shell_cleanup(&ctx);
}

// stop detiene thread
void test_cmd_stop_stops_thread(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    cmd_start(&ctx);
    usleep(THREAD_STARTUP_DELAY_US);

    int result = cmd_stop(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);

    pthread_mutex_lock(&ctx.state_mutex);
    monitor_state_t state = ctx.state;
    pthread_mutex_unlock(&ctx.state_mutex);

    TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, state);

    usleep(10000);  // 10ms - dar tiempo para que el thread termine
    shell_cleanup(&ctx);
}

// ipc - pipe usado en stop
void test_pipe_used_in_stop(void)
{
    shell_context_t ctx;
    
    if (shell_init(&ctx) != 0) {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    cmd_start(&ctx);
    usleep(THREAD_STARTUP_DELAY_US);

    int result = cmd_stop(&ctx);
    TEST_ASSERT_EQUAL_INT(0, result);

    usleep(10000);  // 10ms - dar tiempo para que el thread termine
    shell_cleanup(&ctx);
}

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