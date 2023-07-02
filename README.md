<pre align="center">
 ____  ____   ____ 
|    ||  _ \ |    |
 |  | |  |  | |  | 
|____||__|__||____|
</pre>

<div align="center">

![](https://img.shields.io/badge/made%20by-adasdead-lightgray.svg?style=flat-square)
![](https://img.shields.io/github/languages/top/adasdead/ini.svg?color=lightgray&style=flat-square)

</div>

`ini.h` is a header-only `C/C++` library for working with INI files. It allows you to read, write and modify INI files using a simple and user friendly interface.

## Requirements
- `C/C++` compiler supporting `C99/C++11` or higher
- `ini.h` has no external dependencies

## Installation
To use the library, just copy the `ini.h` file to your project and include it using the #include directive:
```c
#include "ini.h"
```

## Usage
### example.ini
```ini
; https://en.wikipedia.org/wiki/INI_file
; last modified 1 April 2001 by John Doe
[owner]
name = John Doe
organization = Acme Widgets Inc.

[database]
; use IP address in case network name resolution is not working
server = 192.0.2.62     
port = 143
file = "payroll.dat"
```
### main.c
```c
#include <stdio.h>

#include "ini.h"

int main(void)
{
    ini_t ini = ini_parse_from_path("example.ini");

    if (ini != NULL) {
        ini_set(ini, "account", "user", "example");
        ini_set(ini, "account", "email", "example@example.com");
        ini_store_to_file(ini, stdout);
        ini_free(ini);
    }

    return 0;
}
```
### stdout:
```ini
[owner]
name = John Doe
organization = Acme Widgets Inc.
[account]
user = example
email = example@example.com
[database]
file = payroll.dat
port = 143
server = 192.0.2.62
```

## License
The project is distributed under the MIT license. See [LICENSE](LICENSE) file for details.