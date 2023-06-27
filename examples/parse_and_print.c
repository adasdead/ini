#include <stdio.h>

#include "ini.h"

int main(void)
{
    ini_t ini = ini_parse_from_path("example.ini");

    if (ini != NULL) {
        ini_store_to_file(ini, stdout);
        ini_free(ini);
    }

    return 0;
}