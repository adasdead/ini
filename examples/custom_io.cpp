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

static void ini_store_to_stream(ini_t ini, std::ostream &stream)
{
    ini_io io {0};

    io.raw = reinterpret_cast<void*>(&std::cout);
    io.mode = INI_IO_MODE_WRITE;
    io.putc = ini_io_ostream_putc;

    ini_store(ini, &io);
}

int main()
{
    std::ostringstream ini_string;
    ini_string << "[hello]" << std::endl;
    ini_string << "language = c++" << std::endl;
    ini_string << "file_name = supports.cpp" << std::endl;

    ini_t ini = ini_parse_from_str(ini_string.str().data());
    ini_set(ini, nullptr, "hello", "world");
    ini_store_to_stream(ini, std::cout);
    ini_free(ini);
    return 0;
}