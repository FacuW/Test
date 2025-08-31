
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{

    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", "Facundo");
    cJSON_AddNumberToObject(root, "age", 26);
    cJSON_AddBoolToObject(root, "is_student", 1);

    cJSON* hobbies = cJSON_CreateArray();

    cJSON_AddItemToArray(hobbies, cJSON_CreateString("playing guitar"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("watching movies"));
    cJSON_AddItemToObject(root, "hobbies", hobbies);

    char* json_string = cJSON_Print(root);
    printf("%s\n", json_string);

    cJSON_Delete(root);
    free(json_string);

    return 0;
}