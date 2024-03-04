#include "util/concat.hpp"


char* concat(const char *s1, const char *s2){
    char *result = (char*)malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    snprintf(result, strlen(s1)+strlen(s2)+1, "%s%s", s1, s2);
    return result;
}