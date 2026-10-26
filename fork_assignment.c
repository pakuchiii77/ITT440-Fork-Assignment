#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256
#define NUM_CHILDREN 5

void sigint_handler(int sig) {
    const char msg[] = "\n[INTERRUPT DETECTED] Ctrl+C signal received (SIGINT). Cleaning up and exiting safely...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    exit(0);
}

int main() {
    int pipefds[NUM_CHILDREN][2];
    pid_t pids[NUM_CHILDREN];
    char input_buffer[BUFFER_SIZE];

    // Create pipes
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipefds[i]) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Fork child processes
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            // --- CHILD PROCESS ---
            // Revert SIGINT back to default behavior for children so only parent handles Ctrl+C UI message
            signal(SIGINT, SIG_DFL);

            for (int j = 0; j < NUM_CHILDREN; j++) {
                close(pipefds[j][1]);
                if (j != i) {
                    close(pipefds[j][0]);
                }
            }

            char read_buffer[BUFFER_SIZE];
            ssize_t bytes_read = read(pipefds[i][0], read_buffer, sizeof(read_buffer) - 1);

            if (bytes_read > 0) {
                read_buffer[bytes_read] = '\0';
                printf("[Child %d | PID: %d] Received message from Parent: %s", i + 1, getpid(), read_buffer);
            }

            close(pipefds[i][0]);
            exit(0);
        }
    }

    // --- PARENT PROCESS ---
    // Register custom interrupt handler ONLY for Parent process
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Failed to catch SIGINT");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        close(pipefds[i][0]);
    }

    printf("=========================================\n");
    printf("  ITT440 Fork & Pipe Program (Parent PID: %d)\n", getpid());
    printf("=========================================\n");
    printf("Enter a message to pass to child processes (or press Ctrl+C to test interrupt): ");
    
    if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
        for (int i = 0; i < NUM_CHILDREN; i++) {
            write(pipefds[i][1], input_buffer, strlen(input_buffer));
            close(pipefds[i][1]);
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    printf("\n[Parent Process] All child processes completed successfully.\n");
    return 0;
}
