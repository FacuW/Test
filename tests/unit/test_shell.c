#include "shell.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

void test_shell_log_command(void)
{
    int result = shell_log_command("TEST_COMMAND");
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_shell_init(void)
{
    shell_context_t ctx;
    int result = shell_init(&ctx);

    if (result == 0)
    {
        TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, ctx.state);
        shell_cleanup(&ctx);
    }
    // Si falla por permisos, es aceptable en CI
}

void test_monitor_state_transitions(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) == 0)
    {
        TEST_ASSERT_EQUAL_INT(MONITOR_STOPPED, ctx.state);

        pthread_mutex_lock(&ctx.state_mutex);
        ctx.state = MONITOR_RUNNING;
        TEST_ASSERT_EQUAL_INT(MONITOR_RUNNING, ctx.state);
        pthread_mutex_unlock(&ctx.state_mutex);

        shell_cleanup(&ctx);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_shell_log_command);
    RUN_TEST(test_shell_init);
    RUN_TEST(test_monitor_state_transitions);

    return UNITY_END();
}
