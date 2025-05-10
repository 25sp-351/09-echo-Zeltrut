#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int client_fd;
    int verbose_mode;
    struct sockaddr_in client_address;
} client_info_t;

static void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-p port] [-v]\n", program_name);
}

void *handle_client(void *arg)
{
    client_info_t *client_info = (client_info_t *)arg;
    int client_fd = client_info->client_fd;
    int verbose_mode = client_info->verbose_mode;
    char buffer[BUFFER_SIZE];
    char line_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    size_t line_index = 0;

    printf("Connected by %s\n", inet_ntoa(client_info->client_address.sin_addr));
    free(client_info);

    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; ++i) {
            line_buffer[line_index++] = buffer[i];

            if (buffer[i] == '\n') {
                if (verbose_mode) {
                    // Null-terminate for printing
                    line_buffer[line_index - 1] = '\0';
                    printf("Received: %s\n", line_buffer);
                    line_buffer[line_index - 1] = '\n'; // Restore for echo
                }

                if (write(client_fd, line_buffer, line_index) < 0) {
                    perror("write");
                    break;
                }

                line_index = 0; // Reset line buffer
            }
        }
    }

    if (bytes_read < 0) {
        perror("read");
    }

    close(client_fd);
    printf("Connection closed.\n\n");
    return NULL;
}


int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    int verbose_mode = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose_mode = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            if (sscanf(argv[i], "%d", &port) != 1 || port <= 0) {
                fprintf(stderr, "Invalid port.\n");
                return EXIT_FAILURE;
            }
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (!client_info) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        client_info->client_fd = client_fd;
        client_info->verbose_mode = verbose_mode;
        client_info->client_address = client_address;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_info) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_info);
        } else {
            pthread_detach(thread_id);  // doesn't need to join, auto clean up
        }
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
