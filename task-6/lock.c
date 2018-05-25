#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXT ".lck"
#define READER "/bin/cat"
#define WRITER "/usr/bin/nano"

void error_exit(const char* message) {
    printf("%s", message);
    exit(EXIT_FAILURE);
}

char* get_command(const char* path, const char* mode) {
    char* executer = strcmp(mode, "read") == 0 ? READER : WRITER;
    char* command = malloc(sizeof(char) * (strlen(path) + strlen(executer)));

    if (command == NULL) {
        error_exit("Unable to allocate memory.");
    }

    sprintf(command, "%s %s", executer, path);

    return command;
}

char* get_lock_name(const char* path) {
    char* lock_name = malloc(sizeof(char) * (strlen(path) + strlen(EXT)));

    if (lock_name == NULL) {
        error_exit("Unable to allocate memory.");
    }

    sprintf(lock_name, "%s%s", path, EXT);

    return lock_name;
}

void accuire(const char* lock_file, const char* mode) {
    FILE* lock = fopen(lock_file, "w");
    fprintf(lock, "%d\n", getpid());
    fprintf(lock, "%s\n", mode);
    fclose(lock);
}

int main(int argc, const char* argv[]) {
    if (argc < 3 || (strcmp(argv[2], "read") != 0 && strcmp(argv[2], "write") != 0)) {
        puts("usage: lock [filename][read|write] (Read with `cat`, Write with `nano`)");
        exit(EXIT_FAILURE);
    }

    char* lock_file = get_lock_name(argv[1]);
    char* command = get_command(argv[1], argv[2]);

    FILE* lock;
    while ((lock = fopen(lock_file, "r")) != NULL) {
        fclose(lock);
        sleep(1);
    }

    accuire(lock_file, argv[2]);
    system(command);
    remove(lock_file);

    return EXIT_SUCCESS;
}
