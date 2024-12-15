// myshell - Debugged Version with Proper Function Declarations ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_ARGS 32
#define MAX_LINE 128
#define HISTORY_SIZE 10

// Function Prototypes

void parse_command(char *line, char **args, char **pipe_args, char **input_file, char **output_file, char **error_file, int *append_output, int *background);
void execute_with_redirection(char **args, char *input_file, char *output_file, char *error_file, int append_output, int background);
void execute_command(char **args, int background);
int handle_internal_commands(char **args);
char *find_executable(char *command);
void execute_pipe_command(char **args1, char **args2); 
int already_in_list(pid_t pid);

pid_t running_foreground_pid = -1;

// Struct to keep track of background processes
typedef struct {
    pid_t pid;
    char command[MAX_LINE];
} BackgroundProcess;

BackgroundProcess bg_processes[MAX_ARGS];
int bg_count = 0;

char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;

// Function to find executable in PATH
char *find_executable(char *command) {
    char *path = getenv("PATH");
    if (!path) {
        return NULL;
    }

    char *path_copy = strdup(path);
    char *dir;
    static char full_path[4096];
    dir = strtok(path_copy, ":");

    while (dir) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

// Execute a command using execv
void execute_command(char **args, int background) {
    char *executable = find_executable(args[0]);
    if (!executable) {
        fprintf(stderr, "Command not found: %s\n", args[0]);
        return;
    }

    pid_t pid = fork();

    if (pid == 0) { // Child process
        signal(SIGTSTP, SIG_DFL); 
        
        if (execv(executable, args) == -1) {
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { // Parent process
        if (background) {
            printf("Background process started: PID=%d\n", pid);
            bg_processes[bg_count].pid = pid;
            snprintf(bg_processes[bg_count].command, MAX_LINE, "%s", args[0]);
            bg_count++;
        } else {
            running_foreground_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);

            if (WIFSTOPPED(status)) {
                // İşlem durduruldu
                bg_processes[bg_count].pid = pid;
                snprintf(bg_processes[bg_count].command, MAX_LINE, "Suspended process");
                bg_count++;
            }

            running_foreground_pid = -1;
        }
    } else {
        perror("Fork failed");
    }
}



int handle_internal_commands(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        if (bg_count > 0) {
            fprintf(stderr, "Cannot exit: Background processes are running.\n");
            return 1;
        }
        exit(0);
    } else if (strcmp(args[0], "history") == 0) {
        if (args[1] && strcmp(args[1], "-i") == 0) {
            if (args[2]) {
                int index = atoi(args[2]);
                if (index >= 0 && index < history_count) {
                    printf("Executing history[%d]: %s", index, history[index]);

                    
                    char *history_line = strdup(history[index]);
                    char *history_args[MAX_ARGS];
                    char *pipe_args[MAX_ARGS];
                    char *input_file, *output_file, *error_file;
                    int append_output, background;

                   
                    parse_command(history_line, history_args, pipe_args, &input_file, &output_file, &error_file, &append_output, &background);

                  
                    if (pipe_args[0] != NULL) {
                        execute_pipe_command(history_args, pipe_args);
                    } else if (input_file || output_file || error_file) {
                        execute_with_redirection(history_args, input_file, output_file, error_file, append_output, background);
                    } else {
                        execute_command(history_args, background);
                    }

                    free(history_line);
           
                    if (history_count < HISTORY_SIZE) {
                        // Diziyi aşağı kaydır
                        for (int i = history_count; i > 0; i--) {
                            strcpy(history[i], history[i - 1]);
                        }
                        // Çalıştırılan komutu başa ekle
                        strcpy(history[0], history[index]);
                        history_count++;
                    } else {
                        // Diziyi aşağı kaydır
                        for (int i = HISTORY_SIZE - 1; i > 0; i--) {
                            strcpy(history[i], history[i - 1]);
                        }
                        // Çalıştırılan komutu başa ekle
                        strcpy(history[0], history[index]);
                    }

                    return 1;
                } else {
                    fprintf(stderr, "Invalid history index: %s\n", args[2]);
                    return 1;
                }
            } else {
                fprintf(stderr, "Usage: history -i num\n");
                return 1;
            }
        } else {
            
            for (int i = 0; i < history_count; i++) {
                printf("%d %s", i, history[i]);
            }
            return 1;
        }
    } 
   if (strcmp(args[0], "fg") == 0 && args[1]) {
        int job_num = atoi(args[1] + 1);  // %num şeklinde gelen komutları işlemek için
        if (job_num < bg_count && job_num >= 0) {
            pid_t fg_pid = bg_processes[job_num].pid;

            // Foreground'a al
            running_foreground_pid = fg_pid;
            kill(fg_pid, SIGCONT);  // Foreground işlemi başlat
            printf("Foreground process started: PID %d\n", fg_pid);

            // Foreground işlemi bitene kadar bekle
            int status;
            waitpid(fg_pid, &status, WUNTRACED);  // İşlem duraklatılabilir

            // Eğer işlem duraklatıldıysa (CTRL+Z ile)
            if (WIFSTOPPED(status)) {
                printf("Foreground process (PID %d) stopped. Returning to prompt...\n", fg_pid);

                // Eğer işlem zaten listede değilse, tekrar listeye ekle
                if (!already_in_list(fg_pid)) {
                    if (bg_count < MAX_ARGS) {
                        bg_processes[bg_count].pid = fg_pid;
                        snprintf(bg_processes[bg_count].command, MAX_LINE, "Suspended process");
                        bg_count++;
                    } else {
                        fprintf(stderr, "Background process list is full, cannot add PID %d.\n", fg_pid);
                    }
                }
            } 
            // Eğer işlem tamamlandıysa (normal çıkış veya sinyal ile)
            else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                printf("Foreground process (PID %d) exited normally.\n", fg_pid);

                // Process listeden tamamen sil
                for (int i = job_num; i < bg_count - 1; i++) {
                    bg_processes[i] = bg_processes[i + 1];
                }
                bg_count--;
            }

            running_foreground_pid = -1;  // Foreground pid'yi sıfırla
        } else {
            fprintf(stderr, "Invalid job number\n");
        }
        return 1;
    }
    return 0;
    }

// Parse input and handle redirection
void parse_command(char *line, char **args, char **pipe_args, char **input_file, char **output_file, char **error_file, int *append_output, int *background) {
    *input_file = NULL;
    *output_file = NULL;
    *error_file = NULL;
    *append_output = 0;
    *background = 0;


    char *pipe_pos = strchr(line, '|');
    if (pipe_pos) {
        *pipe_pos = '\0'; 
        pipe_pos++;       

       
        char *token = strtok(pipe_pos, " \t\n");
        int j = 0;
        while (token != NULL) {
            pipe_args[j++] = token;
            token = strtok(NULL, " \t\n");
        }
        pipe_args[j] = NULL;
    } else {
        pipe_args[0] = NULL; 
    }

    int i = 0;
    char *token = strtok(line, " \t\n");

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            *output_file = token;
            *append_output = 0;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t\n");
            *output_file = token;
            *append_output = 1;
        } else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " \t\n");
            *error_file = token;
        } else if (strcmp(token, "&") == 0) {
            *background = 1;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

// Execute command with redirection
void execute_with_redirection(char **args, char *input_file, char *output_file, char *error_file, int append_output, int background) {
    char *executable = find_executable(args[0]);
    if (!executable) {
        fprintf(stderr, "Command not found: %s\n", args[0]);
        return;
    }

    pid_t pid = fork();

    if (pid == 0) { // Child process
        if (input_file) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (output_file) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC), 0644);
            if (fd_out < 0) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        if (error_file) {
            int fd_err = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_err < 0) {
                perror("Error opening error file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_err, STDERR_FILENO);
            close(fd_err);
        }

        execv(executable, args);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        if (background) {
            printf("Background process started: PID=%d\n", pid);
            bg_processes[bg_count].pid = pid;
            snprintf(bg_processes[bg_count].command, MAX_LINE, "%s", args[0]);
            bg_count++;
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("Fork failed");
    }
}
void execute_pipe_command(char **args1, char **args2) {
    char *executable1 = find_executable(args1[0]);
    char *executable2 = find_executable(args2[0]);

    if (!executable1) {
        fprintf(stderr, "Command not found: %s\n", args1[0]);
        return;
    }

    if (!executable2) {
        fprintf(stderr, "Command not found: %s\n", args2[0]);
        return;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) { // Child process for the first command
        close(pipe_fd[0]); // Close unused read end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the pipe write end
        close(pipe_fd[1]); // Close the pipe after duplicating

        if (execv(executable1, args1) == -1) {
            perror("Exec failed for first command");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == 0) { // Child process for the second command
        close(pipe_fd[1]); // Close unused write end of the pipe
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin to the pipe read end
        close(pipe_fd[0]); // Close the pipe after duplicating

        if (execv(executable2, args2) == -1) {
            perror("Exec failed for second command");
            exit(EXIT_FAILURE);
        }
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    waitpid(pid1, NULL, 0); // Wait for the first process to complete
    waitpid(pid2, NULL, 0); // Wait for the second process to complete
}

int already_in_list(pid_t pid) {
    for (int i = 0; i < bg_count; i++) {
        if (bg_processes[i].pid == pid) {
            return 1; // Process zaten listede
        }
    }
    return 0; // Process listede değil
}

void sigtstp_handler(int sig) {
    if (running_foreground_pid > 0) {
        // Process durdur (her durumda durdurulmalı)
        kill(running_foreground_pid, SIGSTOP);
        printf("\nForeground process with PID %d stopped.\n", running_foreground_pid);

        // Eğer process listede değilse, ekle
        if (!already_in_list(running_foreground_pid)) {
            bg_processes[bg_count].pid = running_foreground_pid;
            snprintf(bg_processes[bg_count].command, MAX_LINE, "Suspended process");
            bg_count++;
        } else {
            printf("\nProcess with PID %d is already in the background list.\n", running_foreground_pid);
        }

        running_foreground_pid = -1;
    } else {
        printf("\nNo foreground process to stop.");
    }
}


int main() {
    signal(SIGTSTP, sigtstp_handler);

    char line[MAX_LINE];
    char *args[MAX_ARGS], *pipe_args[MAX_ARGS];
    char *input_file, *output_file, *error_file;
    int append_output, background;

    while (1) {
        printf("myshell: ");
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        if (strncmp(line, "history", 7) != 0) { // Eğer ilk kelime "history" değilse kaydet
            if (history_count < HISTORY_SIZE) {
                for (int i = history_count; i > 0; i--) {
                    strcpy(history[i], history[i - 1]);
                }
                strcpy(history[0], line);
                history_count++;
            } else {
                for (int i = HISTORY_SIZE - 1; i > 0; i--) {
                    strcpy(history[i], history[i - 1]);
                }
                strcpy(history[0], line);
            }
        }

        parse_command(line, args, pipe_args, &input_file, &output_file, &error_file, &append_output, &background);

        if (args[0] == NULL) {
            continue;
        }

        if (handle_internal_commands(args)) {
            continue;
        }

        if (pipe_args[0] != NULL) { 
            execute_pipe_command(args, pipe_args);
        } else if (input_file || output_file || error_file) { 
            execute_with_redirection(args, input_file, output_file, error_file, append_output, background);
        } else { 
            execute_command(args, background);
        }
    }

    return 0;
}

