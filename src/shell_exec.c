#include "shell.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Whitelist de comandos permitidos (seguridad)
static const char* allowed_commands[] = {"ls",   "cat",    "grep", "head", "tail", "wc",     "echo", "pwd",
                                         "date", "whoami", "ps",   "df",   "free", "uptime", NULL};

/**
 * @brief Elimina espacios en blanco al inicio y final de una cadena
 */
static char* trim(char* str)
{
    // Eliminar espacios al inicio
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    // Eliminar espacios al final
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    *(end + 1) = '\0';
    return str;
}

/**
 * @brief Verifica si un comando está en la whitelist
 */
static int is_command_allowed(const char* cmd)
{
    for (int i = 0; allowed_commands[i] != NULL; i++)
    {
        if (strcmp(cmd, allowed_commands[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Tokeniza un comando en argumentos
 * @param command Comando a tokenizar
 * @param args Array para almacenar los argumentos (debe tener espacio para MAX_ARGS)
 * @return Número de argumentos
 */
static int tokenize_command(char* command, char** args, int max_args)
{
    int argc = 0;
    char* token = strtok(command, " \t\n");

    while (token != NULL && argc < max_args - 1)
    {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }

    args[argc] = NULL;
    return argc;
}

/**
 * @brief Parsea comando con redirección (ej: "ls -l > file.txt")
 * @param command Comando completo
 * @param cmd_part Buffer para la parte del comando (antes de >)
 * @param file_part Buffer para el nombre del archivo (después de >)
 * @param append 1 si es >>, 0 si es >
 * @return 0 en éxito, -1 en error
 */
static int parse_redirection(const char* command, char* cmd_part, char* file_part, int* append)
{
    const char* redirect_pos = strstr(command, ">>");

    if (redirect_pos != NULL)
    {
        // Redirección con append (>>)
        *append = 1;

        // Copiar parte del comando
        size_t cmd_len = (size_t)(redirect_pos - command);
        strncpy(cmd_part, command, cmd_len);
        cmd_part[cmd_len] = '\0';

        // Copiar nombre del archivo (saltar ">>")
        strncpy(file_part, redirect_pos + 2, MAX_COMMAND_LENGTH - 1);
        file_part[MAX_COMMAND_LENGTH - 1] = '\0';
    }
    else
    {
        redirect_pos = strchr(command, '>');

        if (redirect_pos == NULL)
        {
            return -1;
        }

        // Redirección normal (>)
        *append = 0;

        // Copiar parte del comando
        size_t cmd_len = (size_t)(redirect_pos - command);
        strncpy(cmd_part, command, cmd_len);
        cmd_part[cmd_len] = '\0';

        // Copiar nombre del archivo (saltar ">")
        strncpy(file_part, redirect_pos + 1, MAX_COMMAND_LENGTH - 1);
        file_part[MAX_COMMAND_LENGTH - 1] = '\0';
    }

    return 0;
}

/**
 * @brief Ejecuta un comando con redirección de salida
 * @param ctx Contexto del shell
 * @param command Comando con redirección (ej: "ls -l > file.txt")
 * @return 0 en éxito, -1 en error
 */
int execute_with_redirection(shell_context_t* ctx, const char* command)
{
    (void)ctx;

    char cmd_part[MAX_COMMAND_LENGTH];
    char file_part[MAX_COMMAND_LENGTH];
    int append = 0;

    if (parse_redirection(command, cmd_part, file_part, &append) != 0)
    {
        printf("Error parsing redirection\n");
        return -1;
    }

    // Trim ambas partes
    char* cmd_trimmed = trim(cmd_part);
    char* file_trimmed = trim(file_part);

    // Verificar que el archivo no esté vacío
    if (strlen(file_trimmed) == 0)
    {
        printf("Error: no output file specified\n");
        return -1;
    }

    // Tokenizar comando
    char cmd_copy[MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, cmd_trimmed, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    char* args[64];
    int argc = tokenize_command(cmd_copy, args, 64);

    if (argc == 0)
    {
        printf("No command specified\n");
        return -1;
    }

    // Verificar whitelist
    if (!is_command_allowed(args[0]))
    {
        printf("Command '%s' not allowed\n", args[0]);
        return -1;
    }

    // Fork y ejecutar con redirección
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("Error en fork");
        return -1;
    }

    if (pid == 0)
    {
        // Proceso hijo: redirigir stdout al archivo
        int fd;

        if (append)
        {
            fd = open(file_trimmed, O_WRONLY | O_CREAT | O_APPEND, 0644);
        }
        else
        {
            fd = open(file_trimmed, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }

        if (fd == -1)
        {
            perror("Error opening output file");
            exit(1);
        }

        // Redirigir stdout
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("Error redirecting stdout");
            close(fd);
            exit(1);
        }

        close(fd);

        // Ejecutar comando
        execvp(args[0], args);
        perror("Error en execvp");
        exit(1);
    }

    // Proceso padre: esperar
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        printf("Output saved to: %s\n", file_trimmed);
        return 0;
    }

    return -1;
}

/**
 * @brief Ejecuta un comando externo simple (sin pipes)
 */
int cmd_exec(shell_context_t* ctx, const char* command)
{
    (void)ctx;

    if (!command || strlen(command) == 0)
    {
        printf("Comandos externos permitidos:\n");
        for (int i = 0; allowed_commands[i] != NULL; i++)
        {
            printf("  - %s\n", allowed_commands[i]);
        }
        printf("\nEjemplo: ls /tmp\n");
        printf("Ejemplo: cat /etc/hostname\n");
        printf("Ejemplo: ps aux | grep monitoring\n");
        printf("Ejemplo: ls -l > lista.txt\n");
        return -1;
    }

    // Copiar comando para tokenizar (strtok modifica la cadena)
    char cmd_copy[MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    // Tokenizar
    char* args[64];
    int argc = tokenize_command(cmd_copy, args, 64);

    if (argc == 0)
    {
        printf("No command specified\n");
        return -1;
    }

    // Verificar whitelist
    if (!is_command_allowed(args[0]))
    {
        printf("Command '%s' not allowed. Permitted commands:\n", args[0]);
        for (int i = 0; allowed_commands[i] != NULL; i++)
        {
            printf("  - %s\n", allowed_commands[i]);
        }
        return -1;
    }

    // Fork y ejecutar
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("Error en fork");
        return -1;
    }

    if (pid == 0)
    {
        // Proceso hijo
        execvp(args[0], args);
        perror("Error en execvp");
        exit(1);
    }

    // Proceso padre: esperar
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return -1;
}

/**
 * @brief Cuenta el número de pipes en un comando
 */
static int count_pipes(const char* command_line)
{
    int count = 0;
    for (size_t i = 0; i < strlen(command_line); i++)
    {
        if (command_line[i] == '|')
        {
            count++;
        }
    }
    return count;
}

/**
 * @brief Ejecuta un pipeline de comandos (soporte para pipes)
 * @param ctx Contexto del shell
 * @param command_line Línea de comandos con pipes (ej: "ls | grep test")
 * @return 0 en éxito, -1 en error
 */
int parse_and_execute_pipeline(shell_context_t* ctx, const char* command_line)
{
    (void)ctx;

    int num_pipes = count_pipes(command_line);

    if (num_pipes == 0)
    {
        // Sin pipes, ejecutar directamente
        return cmd_exec(ctx, command_line);
    }

    // Copiar comando para modificar
    char cmd_copy[MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, command_line, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    // Dividir por pipes
    char* commands[10]; // Máximo 10 comandos en pipeline
    int num_commands = 0;

    char* token = strtok(cmd_copy, "|");
    while (token != NULL && num_commands < 10)
    {
        commands[num_commands++] = trim(token);
        token = strtok(NULL, "|");
    }

    // Validar que todos los comandos estén permitidos
    for (int i = 0; i < num_commands; i++)
    {
        char cmd_check[MAX_COMMAND_LENGTH];
        strncpy(cmd_check, commands[i], sizeof(cmd_check) - 1);
        cmd_check[sizeof(cmd_check) - 1] = '\0';

        char* args[64];
        tokenize_command(cmd_check, args, 64);

        if (!is_command_allowed(args[0]))
        {
            printf("Command '%s' not allowed in pipeline\n", args[0]);
            return -1;
        }
    }

    // Crear pipes
    int pipes[9][2]; // Máximo 9 pipes para 10 comandos
    for (int i = 0; i < num_commands - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Error creating pipe");
            return -1;
        }
    }

    // Ejecutar cada comando en el pipeline
    for (int i = 0; i < num_commands; i++)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Error en fork");
            return -1;
        }

        if (pid == 0)
        {
            // Proceso hijo

            // Redirigir stdin desde el pipe anterior (excepto primer comando)
            if (i > 0)
            {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Redirigir stdout al siguiente pipe (excepto último comando)
            if (i < num_commands - 1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Cerrar todos los pipes
            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Tokenizar y ejecutar
            char cmd_exec_buf[MAX_COMMAND_LENGTH];
            strncpy(cmd_exec_buf, commands[i], sizeof(cmd_exec_buf) - 1);
            cmd_exec_buf[sizeof(cmd_exec_buf) - 1] = '\0';

            char* args[64];
            tokenize_command(cmd_exec_buf, args, 64);

            execvp(args[0], args);
            perror("Error en execvp");
            exit(1);
        }
    }

    // Cerrar todos los pipes en el padre
    for (int i = 0; i < num_commands - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Esperar a que terminen todos los procesos
    for (int i = 0; i < num_commands; i++)
    {
        wait(NULL);
    }

    return 0;
}
