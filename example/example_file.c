#include <stdio.h>

#include "ini.h"

int main(void)
{
	FILE *fp = fopen("example.ini", "r");

	if (fp) {
		ini_t ini = ini_parse_from_file(fp);

		if (ini) {
			ini_set(ini, "database", "output", "latest.log");
			
			ini_store_to_file(ini, stdout);
			ini_free(ini);
		}
		
		fclose(fp);
	}

	return 0;
}