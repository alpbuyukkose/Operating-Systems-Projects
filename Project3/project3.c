//Ayse Nisa Sen 150121040
//Damla Kundak 150121001
//Alp Buyukkose 150121055

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_LINES 1000          // Maximum number of lines that can be read from the file
#define MAX_LINE_LENGTH 1024    // Maximum length of each line

// Arrays to track the status of each line for different operations
int upper_done[MAX_LINES];      // Tracks if the line has been converted to uppercase
int replace_done[MAX_LINES];    // Tracks if spaces in the line have been replaced with underscores
char *lines[MAX_LINES];         // Array to store the lines read from the file
int line_status[MAX_LINES];     // Status of each line: 0 (not read), 1 (read), 2 (uppercased), 3 (replaced), 4 (written)
int total_lines = 0;            // Total number of lines read from the file

// Mutexes for each line to ensure thread safety
pthread_mutex_t line_mutex[MAX_LINES];  
pthread_mutex_t processed_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for the global processed line counter
int processed_lines = 0;        // Counter for the number of lines processed
pthread_cond_t cond_done = PTHREAD_COND_INITIALIZER;  // Condition variable to signal when all operations are done

unsigned int seed;              // Seed for random number generation

// Function prototypes for thread functions
void *read_thread(void *arg);
void *upper_thread(void *arg);
void *replace_thread(void *arg);
void *write_thread(void *arg);
void read_file(const char *filename);
char *remove_newline_copy(const char *line);

int main(int argc, char *argv[]) {
    // Check command line arguments for correct usage
    if (argc < 7 || strcmp(argv[1], "-d") != 0 || strcmp(argv[3], "-n") != 0) {
        fprintf(stderr, "Usage: %s -d <file> -n <read_threads> <upper_threads> <replace_threads> <write_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse command line arguments
    char *filename = argv[2]; // File to read
    int read_count = atoi(argv[4]); // Number of read threads
    int upper_count = atoi(argv[5]); // Number of upper case threads
    int replace_count = atoi(argv[6]); // Number of replace threads
    int write_count = atoi(argv[7]); // Number of write threads

    // Validate thread counts
    if (read_count <= 0 || upper_count <= 0 || replace_count <= 0 || write_count <= 0) {
        fprintf(stderr, "Error: Thread counts must be positive integers.\n");
        return EXIT_FAILURE;
    }

    // Read lines from the specified file
    read_file(filename);

    // Initialize mutexes and line statuses
    for (int i = 0; i < total_lines; i++) {
        pthread_mutex_init(&line_mutex[i], NULL);
        line_status[i] = 0; // Set initial status to "not read"
    }

    // Seed the random number generator
    seed = time(NULL);

    // Create arrays for thread IDs
    pthread_t read_threads[read_count], upper_threads[upper_count], replace_threads[replace_count], write_threads[write_count];
    int read_ids[read_count], upper_ids[upper_count], replace_ids[replace_count], write_ids[write_count];

    // Create read threads
    for (int i = 0; i < read_count; i++) {
        read_ids[i] = i + 1; // Assign thread IDs
        pthread_create(&read_threads[i], NULL, read_thread, &read_ids[i]);
    }

    // Create upper case threads
    for (int i = 0; i < upper_count; i++) {
        upper_ids[i] = i + 1; // Assign thread IDs
        pthread_create(&upper_threads[i], NULL, upper_thread, &upper_ids[i]);
    }

    // Create replace threads
    for (int i = 0; i < replace_count; i++) {
        replace_ids[i] = i + 1; // Assign thread IDs
        pthread_create(&replace_threads[i], NULL, replace_thread, &replace_ids[i]);
    }

    // Create write threads
    for (int i = 0; i < write_count; i++) {
        write_ids[i] = i + 1; // Assign thread IDs
        pthread_create(&write_threads[i], NULL, write_thread, &write_ids[i]);
    }

    // Wait for all read threads to finish
    for (int i = 0; i < read_count; i++) {
        pthread_join(read_threads[i], NULL);
    }

    // Wait for all upper case threads to finish
    for (int i = 0; i < upper_count; i++) {
        pthread_join(upper_threads[i], NULL);
    }

    // Wait for all replace threads to finish
    for (int i = 0; i < replace_count; i++) {
        pthread_join(replace_threads[i], NULL);
    }

    // Wait for all write threads to finish
    for (int i = 0; i < write_count; i++) {
        pthread_join(write_threads[i], NULL);
    }

    // Rename the output file to match the input file name
    if (rename("temp_output.txt", filename) != 0) {
        perror("Error renaming output file");
        return EXIT_FAILURE;
    }

    // Clean up: destroy mutexes and free allocated memory
    for (int i = 0; i < total_lines; i++) {
        pthread_mutex_destroy(&line_mutex[i]);
        if (lines[i]) free(lines[i]); // Free each line
    }

    return 0; 
}

// Function to read lines from a file
void read_file(const char *filename) {
    FILE *file = fopen(filename, "r"); // Open the file for reading
    if (!file) {
        perror("Error opening file"); // Handle file open error
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_LINE_LENGTH]; // Buffer to hold each line
    while (fgets(buffer, sizeof(buffer), file)) { // Read each line
        lines[total_lines] = strdup(buffer); // Duplicate the line and store it
        total_lines++; 
    }
    fclose(file); 
}

// Function for the read thread
void *read_thread(void *arg) {
    int thread_id = *(int *)arg; // Get the thread ID
    for (int index = 0; index < total_lines; index++) {
        pthread_mutex_lock(&line_mutex[index]); // Lock the mutex for the current line
        if (line_status[index] == 0) { // Check if the line has not been read
            // Print the line read by the thread
            printf("Read_%d Read_%d read the line %d which is \"%s\"\n", thread_id, thread_id, index + 1, remove_newline_copy(lines[index]));
            line_status[index] = 1;  // Mark the line as read
        }
        pthread_mutex_unlock(&line_mutex[index]); // Unlock the mutex
        usleep(rand() % 1000);  // Simulate a delay
    }
    return NULL; 
} 

// Function for the replace thread
void *replace_thread(void *arg) {
    int thread_id = *(int *)arg; // Get the thread ID
    for (int index = 0; index < total_lines; index++) {
        pthread_mutex_lock(&line_mutex[index]); // Lock the mutex for the current line
        if ((line_status[index] == 1 || upper_done[index]) && !replace_done[index]) {  // Check if the line is ready for replacement
            char *original_line = strdup(lines[index]); // Duplicate the original line
            for (int i = 0; lines[index][i]; i++) { // Replace spaces with underscores
                if (lines[index][i] == ' ') {
                    lines[index][i] = '_';
                }
            }
            // Print the replacement operation
            printf("Replace_%d Replace_%d read index %d and converted \"%s\" to \"%s\"\n", thread_id, thread_id, index + 1, original_line, lines[index]);
            free(original_line); // Free the original line
            replace_done[index] = 1; // Mark the line as replaced
            // Check if both upper and replace operations are done
            if (upper_done[index]) {
                line_status[index] = 3; // Mark the line as ready for writing
            }
        }
        pthread_mutex_unlock(&line_mutex[index]); // Unlock the mutex
        usleep(rand() % 1000);  // Simulate a delay
    }
    return NULL;
}

// Function for the upper case thread
void *upper_thread(void *arg) {
    int thread_id = *(int *)arg; // Retrieve the thread ID from the argument
    // Loop through all lines
    for (int index = 0; index < total_lines; index++) {
        pthread_mutex_lock(&line_mutex[index]); // Lock the mutex for the current line
        // Check if the line has been read and not yet converted to uppercase
        if (line_status[index] == 1 && !upper_done[index]) {  
            char *original_line = strdup(lines[index]); // Duplicate the original line for logging
            // Convert each character in the line to uppercase
            for (int i = 0; lines[index][i]; i++) {
                lines[index][i] = toupper(lines[index][i]);
            }
            // Print the conversion operation
            printf("Upper_%d Upper_%d read index %d and converted \"%s\" to \"%s\"\n", 
                   thread_id, thread_id, index + 1, original_line, lines[index]);
            free(original_line); // Free the duplicated original line
            upper_done[index] = 1; // Mark the line as converted to uppercase
            // Check if the replace operation for this line is also done
            if (replace_done[index]) {
                line_status[index] = 3; // Mark the line as ready for writing
            }
        }
        pthread_mutex_unlock(&line_mutex[index]); // Unlock the mutex for the current line
        usleep(rand() % 1000);  // Simulate a delay to mimic processing time
    }
    return NULL; 
}

// Function for the write thread
void *write_thread(void *arg) {
    int thread_id = *(int *)arg; // Retrieve the thread ID from the argument
    while (1) { // Infinite loop until all lines are processed
        int processed = 0; // Flag to check if any line was processed
        // Loop through all lines
        for (int index = 0; index < total_lines; index++) {
            pthread_mutex_lock(&line_mutex[index]); // Lock the mutex for the current line
            // Check if the line is ready for writing (i.e., replace operation is done)
            if (line_status[index] == 3) { 
                FILE *file = fopen("temp_output.txt", "a"); // Open the output file for appending
                if (!file) {
                    perror("Error opening output file"); // Handle file open error
                    pthread_mutex_unlock(&line_mutex[index]); // Unlock the mutex before exiting
                    pthread_exit(NULL); // Exit the thread on error
                }
                // Write the line to the file
                if (fprintf(file, "%s\n", lines[index]) < 0) {
                    perror("Error writing to file"); // Handle write error
                } else {
                    // Print the write operation
                    printf("Writer_%d Writer_%d write line %d back which is \"%s\"\n", 
                           thread_id, thread_id, index + 1, lines[index]);
                    fflush(file); // Flush the file buffer to ensure data is written
                    line_status[index] = 4; // Mark the line as written
                    pthread_mutex_lock(&processed_mutex); // Lock the processed mutex
                    processed_lines++; // Increment the count of processed lines
                    pthread_mutex_unlock(&processed_mutex); // Unlock the processed mutex
                }
                fclose(file); // Close the output file
                processed = 1; // Set processed flag to indicate a line was processed
            }
            pthread_mutex_unlock(&line_mutex[index]); // Unlock the mutex for the current line
        }
        // Check if all lines have been processed
        pthread_mutex_lock(&processed_mutex); // Lock the processed mutex
        if (processed_lines >= total_lines) { // If all lines are processed
            pthread_mutex_unlock(&processed_mutex); // Unlock the processed mutex
            break; // Exit the loop
        }
        pthread_mutex_unlock(&processed_mutex); // Unlock the processed mutex
        if (!processed) usleep(100); // If no lines were processed, wait briefly
    }
    return NULL; 
}

// Function to remove the newline character from a line and return a copy
char *remove_newline_copy(const char *line) {
    size_t len = strlen(line); // Get the length of the line
    char *temp_line = malloc(len + 1); // Allocate memory for the new line (including null terminator)
    if (!temp_line) {
        perror("Memory allocation failed"); // Handle memory allocation error
        exit(EXIT_FAILURE); // Exit the program on failure
    }
    strcpy(temp_line, line); // Copy the original line to the new memory
    // Check if the last character is a newline and remove it
    if (len > 0 && temp_line[len - 1] == '\n') { 
        temp_line[len - 1] = '\0'; // Replace newline with null terminator
    }
    return temp_line; // Return the modified line
}
