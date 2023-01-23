# ini
Header-only C/C++ ini parser 

# Installation
1. Put ini.h in your project files
2. Include the "ini.h" file in your code:
```c
#include "ini.h"
```
3. Enjoy :D

# Examples
```c
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
```

```cpp
#include <fstream>

#include "ini.h"

int main()
{
	std::ifstream ifs("example.ini");

	if(ifs.is_open()) {
		ini::ini_t ini = ini::ini_parse_from_istream(ifs);
		
		if (ini) {
			ini::ini_set(ini, "database", "output", "latest.log");

			ini::ini_store_to_ostream(ini, std::cout);
			ini::ini_free(ini);
		}
	}

	return 0;
}
```

``` c
#include <stdio.h>

#include "ini.h"

static const char *example =	"build folder = \"build/\"		\n"
				"\n"
				"   [game_info]\n"
				"name=    my first game\n"
				"year	= 1997\n"
				"   version=1.0    ";

struct game_info
{
	const char *name;
	const char *year;
	const char *version;
};

static void game_info_from_ini(struct game_info *inf, ini_t ini)
{
	inf->name = ini_get(ini, "game_info", "name", "noname");
	inf->year = ini_get(ini, "game_info", "year", "19**");
	inf->version = ini_get(ini, "game_info", "version", "0.0");
}

int main(void)
{
	ini_t ini = ini_parse_from_str(example);
	struct game_info info = {0};
	const char *build_folder;

	if (ini) {
		build_folder = ini_get(ini, NULL, "build folder", "");

		puts("----------------------------------");

		printf("build_folder: %s\n", build_folder);

		puts("----------------------------------");

		game_info_from_ini(&info, ini);

		printf("   name:\t%s\n", info.name);
		printf("   year:\t%s\n", info.year);
		printf("version:\t%s\n", info.version);

		puts("----------------------------------");

		ini_store_to_file(ini, stdout);

		puts("----------------------------------");

		ini_free(ini);
	}

	return 0;
}
```