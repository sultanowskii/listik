#include <errno.h>
#include <stdio.h>

int main() {
    FILE *file = fopen("/proc/listik", "r");
    if (file == NULL) {
        perror("Can't open the /proc/listik");
        return errno;
    }

    char buffer[16];

    while (1) {
        size_t fread_result = fread(buffer, sizeof(char), 16, file);
        if (fread_result == 0) {
            if (feof(file)) {
                break;
            } else {
                perror("Error reading /proc/listik");
                return errno;
            }
        }

        size_t fwrite_result = fwrite(buffer, sizeof(char), fread_result, stdout);
        if (fwrite_result == 0) {
            perror("Can't write to stdout");
            return errno;
        }
    }

    return 0;
}
