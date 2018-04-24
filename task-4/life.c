#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int ms = 1000 * 1000;
int directions[16] = { 
    -1, -1, 
     0, -1, 
     1, -1, 
    -1,  0, 
     1,  0, 
    -1,  1, 
     0,  1, 
     1,  1,
};

typedef struct life {
    int* map;
    int width;
    int height;
} life_t;

life_t life_g;

void error_exit(const char* message) {
    printf("%s", message);
    exit(EXIT_FAILURE);
}

void malloc_error_exit() {
    error_exit("Unable to allocate memory.\n");
}

void usage_error_exit() {
    error_exit("Usage: <filename> <width> <height> <port>.\n");
}

void network_error_exit() {
    error_exit("Networking error.\n");
}

void time_error_exit() {
    error_exit("Timing error.\n");
}

int get(life_t life, int x, int y) {
    return life.map[y * life.width + x];
}

void print_life(life_t life) {
    for (int i = 0; i < life.height; i++) {
        for (int j = 0; j < life.width; j++) {
            printf("%d", get(life, j, i));
        }
        printf("\n");
    }

    printf("\n");
}

void set(life_t life, int x, int y, int val) {
    life.map[y * life.width + x] = val;
}

void init(life_t** life, int w, int h) {
    (*life) = malloc(sizeof(life_t));

    if (*life == NULL) {
        malloc_error_exit();
    }

    (*life)->map = malloc(sizeof(int) * w * h);

    if ((*life)->map == NULL) {
        malloc_error_exit();
    }
    
    (*life)->width = w;
    (*life)->height = h;
}

int count_alive_neighbours(life_t life, int x, int y) {
    int alive_count = 0;

    for (int d = 0; d < 16; d += 2) {
        int dx = directions[d];
        int dy = directions[d + 1];
        int nx = (x + dx + life.width) % life.width;
        int ny = (y + dy + life.height) % life.height;
        alive_count += get(life, nx, ny);
    }

    return alive_count;
}

void next_generation() {
    life_t* new_life;
    init(&new_life, life_g.width, life_g.height);

    for (int y = 0; y < life_g.height; y++) {
        for (int x = 0; x < life_g.width; x++) {
            int alive_count = count_alive_neighbours(life_g, x, y);

            if (get(life_g, x, y) == 0) {
                set(*new_life, x, y, alive_count == 3);
            } else {
                set(*new_life, x, y, alive_count == 2 || alive_count == 3);
            }
        }
    }

    memcpy(life_g.map, new_life->map, sizeof(int) * life_g.width * life_g.height);
}

char* read_all(FILE* file) {
    size_t read_size;
    long string_size;

    char* buffer = NULL;
    
    fseek(file, 0, SEEK_END);
    string_size = ftell(file);
    rewind(file);

    buffer = (char*) malloc(sizeof(char) * (string_size + 1));

    if (buffer == NULL) {
        malloc_error_exit();
    }

    read_size = fread(buffer, sizeof(char), (size_t) string_size, file);

    buffer[string_size] = '\0';

    if (string_size != read_size) {
        free(buffer);
        buffer = NULL;
    }

    fclose(file);

    return buffer;
}

int* read_map(const char* path) {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        error_exit("Error while open file with model.\n");
    }

    char* content = read_all(file);

    int length = strlen(content);
    int* model = malloc(sizeof(int) * length);

    if (model == NULL) {
        malloc_error_exit();
    }
    
    for (int i = 0, j = 0; i < length; i++) {
        if (content[i] == '0' || content[i] == '1') {
            model[j++] = content[i] == '1';
        }
    }

    return model;
}

void *model_thread(void* var) {
    struct timespec start, stop;
 
    while (1) {
        if (getppid() == 1) {
            exit(EXIT_FAILURE);
        }

        if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
            time_error_exit();
        }

        next_generation();
        printf(".");
        fflush(stdout);

        if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
            time_error_exit();
        }

        float secs = (float)(stop.tv_sec - start.tv_sec);
        float nsecs = (float)(stop.tv_nsec - start.tv_nsec) / (ms * 1000);

        if (secs + nsecs > 1) {
            printf("Calculation of next generation took more than second.\n");
        } else {
            usleep((1.0 - secs - nsecs) * ms);
        }
    }
}

void *server_thread(void *var) {
    int port = * (int*) var;
    int server = socket(AF_INET, SOCK_STREAM, 0);

    if (server < 0) {
        network_error_exit();
    }

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        network_error_exit();
    }

    struct sockaddr_in addr = { 
        .sin_family = AF_INET, 
        .sin_addr.s_addr = INADDR_ANY, 
        .sin_port = htons(port),
    };

    if (bind(server, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        network_error_exit();
    }

    if (listen(server, 5) < 0) {
        network_error_exit();
    }

    while (1) {
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);
        int sock = accept(server, (struct sockaddr*)&addr, &addr_size);

        if (sock < 0) {
            network_error_exit();
        }

        for (int y = 0; y < life_g.height; y++) {
            write(sock, "┃", 3);

            for (int x = 0; x < life_g.width; x++) {
                write(sock, get(life_g, x, y) == 0 ? " " : "*", 1);
            }

            write(sock, "┃\n", 4);
        }

        close(sock);
    }
}

int main(int argc, char** argv) {
    if (argc != 5) 
        usage_error_exit();

    int port = atoi(argv[4]);
    life_t life = {
        .map = read_map(argv[1]),
        .width = atoi(argv[2]),
        .height = atoi(argv[3]),
    };

    if (life.width == 0 || life.height == 0 || port == 0) {
        usage_error_exit();
    }

    life_g = life;

    pthread_t tid[2];
    pthread_create(&tid[0], NULL, model_thread, NULL);
    pthread_create(&tid[1], NULL, server_thread, (void*)&port);
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    return 0;
}
