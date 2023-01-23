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