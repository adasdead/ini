# ini
Header-only C/C++ ini parser 

# Installation
1. Put ini.h in your project files
2. Include the "ini.h" file in your code:
```c
#include "ini.h"
```
3. Enjoy :D

# Example
## example.ini
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
## main.c
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
## output:
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