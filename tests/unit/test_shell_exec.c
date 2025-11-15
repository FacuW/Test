// tests para comandos externos

#include "shell.h"
#include "unity.h"
#include <string.h>
#include <unistd.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// Test de comando exec simple
void test_exec_simple_command(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Ejecutar comando simple: ls
    int result = cmd_exec(&ctx, "ls /tmp");
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// Test de comando no permitido
void test_exec_disallowed_command(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Intentar ejecutar comando no permitido
    int result = cmd_exec(&ctx, "rm -rf /");
    TEST_ASSERT_EQUAL_INT(-1, result);

    shell_cleanup(&ctx);
}

// Test de pipeline simple
void test_pipeline_simple(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Pipeline: ls | head
    int result = parse_and_execute_pipeline(&ctx, "ls /tmp | head -n 3");
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// Test de pipeline con grep
void test_pipeline_with_grep(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Pipeline: ps | grep monitoring
    int result = parse_and_execute_pipeline(&ctx, "ps aux | grep monitoring");
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// Test de pipeline con comando no permitido
void test_pipeline_disallowed_command(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Pipeline con comando no permitido
    int result = parse_and_execute_pipeline(&ctx, "ls | rm -rf");
    TEST_ASSERT_EQUAL_INT(-1, result);

    shell_cleanup(&ctx);
}

// Test de exec con argumentos
void test_exec_with_arguments(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Comando con argumentos: cat /etc/hostname
    int result = cmd_exec(&ctx, "cat /etc/hostname");
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

// Test de pipeline de 3 comandos
void test_pipeline_three_commands(void)
{
    shell_context_t ctx;

    if (shell_init(&ctx) != 0)
    {
        TEST_IGNORE_MESSAGE("shell_init failed - permissions issue");
        return;
    }

    // Pipeline de 3 comandos: ls | grep test | wc -l
    int result = parse_and_execute_pipeline(&ctx, "ls /tmp | grep tmp | head -n 1");
    TEST_ASSERT_EQUAL_INT(0, result);

    shell_cleanup(&ctx);
}

int main(void)
{
    UNITY_BEGIN();

    // Tests de exec
    RUN_TEST(test_exec_simple_command);
    RUN_TEST(test_exec_disallowed_command);
    RUN_TEST(test_exec_with_arguments);

    // Tests de pipelines
    RUN_TEST(test_pipeline_simple);
    RUN_TEST(test_pipeline_with_grep);
    RUN_TEST(test_pipeline_disallowed_command);
    RUN_TEST(test_pipeline_three_commands);

    return UNITY_END();
}