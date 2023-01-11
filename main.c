#include <stdio.h>

#include "ini.h"

static const char *example_str = "[example]	\n"		\
				 "hello = world		\n"	\
				 "no_value";

int main(void)
{
	ini_t ini = ini_parse_from_path("example.ini");

	if (ini) {
		puts(ini_get(ini, "owner", "name", "noname"));
		puts(ini_get(ini, "owner", "organization", "none"));

		puts(ini_get(ini, "database", "server", "0.0.0.0"));
		puts(ini_get(ini, "database", "port", "127"));
		puts(ini_get(ini, "database", "file", "null.dat"));
		ini_free(ini);
	}

	return 0;
}

// ini.h:201:37: warning: leak of '<unknown>' [CWE-401] [-Wanalyzer-malloc-leak]