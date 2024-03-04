#include "util/kernel.hpp"

// Detect kernel version. Breaks if minor version is greater than 100
double kernel_version() {
    struct utsname buffer;
    uname(&buffer);

    char *token;
    char *separator = (char*)".";
    double value = 0;

    token = strtok(buffer.release, separator);
    value += atof(token);
    token = strtok(NULL, separator);
    // the below if statement catches weird segfaults:
    // __GI_____strtod_l_internal (nptr=0x0, endptr=0x0, group=<optimized out>, loc=0x7f42a025e560 <_nl_global_locale>) at strtod_l.c:610
    // 610  strtod_l.c: No such file or directory.
    // #0  __GI_____strtod_l_internal (nptr=0x0, endptr=0x0, group=<optimized out>, loc=0x7f42a025e560 <_nl_global_locale>) at strtod_l.c:610
    // #1  0x000000000040e930 in kernel_version () at slothy.c:130
    // #2  0x000000000040e977 in get_fd_archdep () at slothy.c:146
    if (token == NULL) {
        return kernel_version();
    }
    value += atof(token)/100;

    return value;
}


