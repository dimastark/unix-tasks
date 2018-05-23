#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "list.c"

#define MAX_RETRIES 50
#define BUFFER_SIZE 4096
#define RESPAWN_CONFIG_PATH "respawn.conf"
#define WAIT_CONFIG_PATH "wait.conf"

FILE* respawn_config;
List* respawn_commands;
int respawn_commands_count;
pid_t* respawn_pids;

FILE* wait_config;
List* wait_commands;
int wait_commands_count;
pid_t* wait_pids;

void error_exit(const char* message) {
    syslog(LOG_ERR, "%s", message);
    exit(EXIT_FAILURE);
}

bool daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        error_exit("Unable to create subprocess.");
    }
    
    if (pid > 0) {
        exit(printf("Start visor daemon with pid = %d\n", pid));
    }

    if (setsid() < 0) {
        return false;
    }
    
    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return true;
}

List* read_lines(FILE* file) {
	List* list = new_List();

	char* line = NULL;
    size_t len, nread;

    rewind(file);

	while ((nread = getline(&line, &len, file)) != -1) {
        char* copy = (char*) malloc(nread * sizeof(char));

        if (copy == NULL) {
            error_exit("Unable to allocate memory.");
        }

        strcpy(copy, line);
        copy[nread - 1] = '\0';

        push(list, copy);
	}

    return list;
}

char* get_pid_filename(int i, bool need_respawn) {
    char* process_type = need_respawn ? "respawn" : "wait";
    char* filename = malloc(sizeof(char) * BUFFER_SIZE);

    if (filename == NULL) {
        error_exit("Unable to allocate memory.");
    }

    sprintf(filename, "/tmp/%s-%d.pid", process_type, i + 1);

    return filename;
}

void write_pid(int i, bool need_respawn, int pid) {
    char* filename = get_pid_filename(i, need_respawn);
    FILE* file = fopen(filename, "w");

    if (!file) {
        error_exit("Can't write pid to file.");
    }

    fprintf(file, "%d", pid);
    fclose(file);
}

void delete_pid(int i, bool need_respawn) {
    char* filename = get_pid_filename(i, need_respawn);

    if (remove(filename) != 0) {
        error_exit("Can't remove pid file.");
    }
}

int system_with_retries(char* command) {
    struct timespec start, stop;

    int retries;
    int return_code = -1;

    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        return EXIT_FAILURE;
    }

    while (return_code != 0) {
        return_code = system(command);

        if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
            return EXIT_FAILURE;
        }

        if (retries >= MAX_RETRIES && stop.tv_sec - start.tv_sec < 5) {
            return EXIT_FAILURE;
        }

        if (stop.tv_sec - start.tv_sec >= 5) {
            return EXIT_SUCCESS;
        }

        retries++;
    }

    return EXIT_SUCCESS;
}

int spawn_one(List* commands, int i, bool need_respawn) {
    int pid = fork();
    char* command = item(commands, i);

    if (pid == -1) {
        error_exit("Fail to create subprocess.");
    }
    
    if (pid == 0) {
        exit(system_with_retries(command));
    } else {
        write_pid(i, need_respawn, pid);

        syslog(LOG_INFO, "Run `%s` with pid = %d.", command, pid);

        return pid;
    }

    return 0;
}

void spawn(pid_t** pids, List* commands, int commands_count, bool need_respawn) {
    (*pids) = (pid_t*) malloc(commands_count * sizeof(pid_t));

    if ((*pids) == NULL) {
        error_exit("Unable to allocate memory.");
    }

    for (int i = 0; i < commands_count; i++) {
        (*pids)[i] = spawn_one(commands, i, need_respawn);
    }
}

void kill_all(pid_t* pids, int count, bool need_respawn) {
    for (int i = 0; i < count; i++) {
        kill(pids[i], SIGKILL);
        delete_pid(i, need_respawn);
    }
}

void setup() {
    respawn_commands = read_lines(respawn_config);
    respawn_commands_count = length(respawn_commands);
    spawn(&respawn_pids, respawn_commands, respawn_commands_count, true);
    syslog(LOG_INFO, "Run processes for respawn finished.");

    wait_commands = read_lines(wait_config);
    wait_commands_count = length(wait_commands);
    spawn(&wait_pids, wait_commands, wait_commands_count, false);
    syslog(LOG_INFO, "Run processes for wait finished.");
}

void hup_handler(int sig) {
    signal(SIGHUP, hup_handler);
    syslog(LOG_INFO, "Reloading...");
    
    kill_all(wait_pids, wait_commands_count, false);
    kill_all(respawn_pids, respawn_commands_count, true);

    free_List(wait_commands);
    free_List(respawn_commands);

    free(wait_pids);
    free(respawn_pids);

    setup();
}

int main(int argc, char const *argv[]) {
    respawn_config = fopen(RESPAWN_CONFIG_PATH, "r");
    wait_config = fopen(WAIT_CONFIG_PATH, "r");

    if (!daemonize()) {
        puts("Daemonize error.");
        exit(EXIT_FAILURE);
    }

    openlog("visord", LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

    if (!respawn_config || !wait_config) {
        syslog(LOG_ERR, "Can't read config files.");
        exit(EXIT_FAILURE);
    }

    setup();

    signal(SIGHUP, hup_handler);

    while (respawn_commands_count + wait_commands_count) {
        int return_code;
        int child_pid = waitpid(-1, &return_code, 0);

        for (int i = 0; i < respawn_commands_count; i++) {
            if (respawn_pids[i] == child_pid) {
                delete_pid(i, true);

                if (return_code == 0) {
                    respawn_pids[i] = spawn_one(respawn_commands, i, true);
                } else {
                    respawn_commands_count--;
                }
            }
        }
        
        for (int i = 0; i < wait_commands_count; i++) {
            if (wait_pids[i] == child_pid) {
                delete_pid(i, false);
                wait_commands_count--;
            }
        }
    }

    syslog(LOG_INFO, "Exiting...");

    return EXIT_SUCCESS;
}
