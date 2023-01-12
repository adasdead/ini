#include <stdio.h>

#include "ini.h"

int main(void)
{
	ini_t ini = ini_parse_from_path("example.ini");

	if (ini) {
		puts(ini_get(ini, "owner", "name", "noname"));
		puts(ini_get(ini, "owner", "organization", "none"));

		puts(ini_get(ini, "database", "server", "0.0.0.0"));
		puts(ini_get(ini, "database", "port", "127"));
		puts(ini_get(ini, "database", "file", "null.dat"));
		
		ini_store_to_path(ini, "output.ini");
		
		ini_free(ini);
	}

	return 0;
}