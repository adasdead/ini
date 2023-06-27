#include <iostream>
#include <sstream>

#include "ini.h"

static void ini_io_ostream_putc(ini_io *io, int ch)
{
	if (io != nullptr) {
		char cch = static_cast<char>(ch);
		*(reinterpret_cast<std::ostream*>(io->raw)) << cch;
	}
}

int main()
{
    std::ostringstream ini_string;
    ini_string << "[hello]" << std::endl;
    ini_string << "language = c++" << std::endl;
    ini_string << "file_name = supports.cpp" << std::endl;

	ini_t ini = ini_parse_from_str(ini_string.str().data());

    ini_io console_output {0};
	console_output.raw = reinterpret_cast<void*>(&std::cout);
	console_output.type = INI_IO_WRITE;
	console_output.peek = INI_IO_PEEK;
	console_output.putc = ini_io_ostream_putc;

	ini_set(ini, "hello", "year", "2023");
	ini_store(ini, &console_output);
    return 0;
}