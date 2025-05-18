#include <regex.h>


int main() {
    regex_t regex;
    int reti;

    regcomp(&regex, ".*", 0);
    regfree(&regex);
    return 0;
};
