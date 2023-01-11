#include <stdio.h>

#include "ini.h"

static const char *example_str = "[example]	\n"		\
				 "hello = world		\n"	\
				 "no_value";

int main(void)
{
	char *line;
	struct ini__io io = {0};
	FILE *fp = fopen("example.ini", "r");

	if (!fp) return -1;

	ini__io_file(&io, fp, INI__IO_READ);

	while (line = ini__io_line(&io)) {
		puts(line);
		free(line);
	}
	
	fclose(fp);
	return 0;
}

// ini.h:201:37: warning: leak of '<unknown>' [CWE-401] [-Wanalyzer-malloc-leak]