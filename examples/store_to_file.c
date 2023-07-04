#include "ini.h"

int main(void)
{
    ini_t ini = ini_new();

    ini_set(ini, NULL, "version", "1.0");
    ini_set(ini, NULL, "language", "c");
    ini_set(ini, NULL, "hello", "world");

    ini_set(ini, "account", "user", "example");
    ini_set(ini, "account", "email", "example@example.com");

    ini_store_to_path(ini, "result.ini");

    ini_free(ini);
    return 0;
}