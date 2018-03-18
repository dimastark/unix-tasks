#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

enum reading { ZEROS, BYTES };

int main(int argc, char **argv) {
    int fdi = STDIN_FILENO;
    int fdo = STDOUT_FILENO;

    if (argc == 2) {
        fdo = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (fdo < 0) {
            perror("Error while output open");
            return 1;
        }
    }

    enum reading state;
    char buffer[BUFFER_SIZE];

    int count = read(fdi, buffer, BUFFER_SIZE);

    while (count != 0) {
        int handled_count = 0;
        char* from = buffer;

        for (char* p = buffer; p < buffer + count; p++, handled_count++) {
            if (*p && handled_count && state == ZEROS) {
                from = p - 1;
                lseek(fdo, handled_count, SEEK_CUR);
                handled_count = 0;
                state = BYTES;
            } else if (!*p && handled_count && state == BYTES) {
                write(fdo, from, handled_count);
                handled_count = 0;
                state = ZEROS;
            }
        }

        if (handled_count && state == BYTES) {
            write(fdo, from, handled_count);
        } else if (handled_count && state == ZEROS) {
            lseek(fdo, handled_count, SEEK_CUR);
        }

        count = read(fdi, buffer, BUFFER_SIZE);
    }

    close(fdo);
}
