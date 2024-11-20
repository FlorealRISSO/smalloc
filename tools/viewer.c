#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// Terminal colors
#define COLOR_RESET "\033[0m"
#define COLOR_FREE "\033[44m"    // Blue background
#define COLOR_USED1 "\033[41m"   // Red background
#define COLOR_USED2 "\033[42m"   // Green background
#define COLOR_USED3 "\033[43m"   // Yellow background

#define DEFAULT_NB_STATUS 124
#define DEFAULT_COLUMNS 6

void enableAlternateScreen() {
    printf("\e[?1049h");
    fflush(stdout);
}

void disableAlternateScreen() {
    printf("\e[?1049l");
    fflush(stdout);
}

char get_input() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

const char* get_status_color(uint8_t status) {
    switch (status) {
        case 0: return COLOR_FREE;
        case 1: return COLOR_USED1;
        case 2: return COLOR_USED2;
        case 3: return COLOR_USED3;
        default: return COLOR_RESET;
    }
}

void render_grid(uint8_t* data, size_t data_len, size_t grid_size, size_t start_index, size_t colums) {
    size_t end_index = start_index + grid_size;
    if (end_index > data_len) {
        end_index = data_len;
    }

    printf("\033[2J\033[H"); // Clear screen and move cursor to the top-left
    printf("Status Viewer\n");
    printf("Page: %zu / %zu\n\n", start_index / grid_size + 1, (data_len + grid_size - 1) / grid_size);

    int count = 1;
    for (size_t i = start_index; i < end_index; ++i) {
        for (int bit = 0; bit < 4; ++bit) {
            uint8_t status = (data[i] >> (bit * 2)) & 0b11;
            printf("%s %d " COLOR_RESET, get_status_color(status), status);
        }
        if (count % colums == 0) {
            printf("\n");
        }
        count++;
    }
    printf("\nUse Left/Right arrows to navigate, 'q' to quit.\n");
}

int parse_args(int argc, char **argv, const char **file, int *nb_status, int *columns) {
    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Usage: %s <file> [-n <NB_STATUS = %d>] [-c <columns = %d>]\n", argv[0], DEFAULT_NB_STATUS, DEFAULT_COLUMNS);
        return 1;
    }

    *file = NULL;
    *nb_status = DEFAULT_NB_STATUS;
    *columns = DEFAULT_COLUMNS;

    *file = argv[1]; // First argument is always the file

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                *nb_status = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: Missing value for -n option.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                *columns = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: Missing value for -c option.\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'.\n", argv[i]);
            return 1;
        }
    }

    if (*file == NULL) {
        fprintf(stderr, "Error: Missing <file> argument.\n");
        return 1;
    }

    return 0; // Success
}

int main(int argc, char* argv[]) {
    const char* filename = NULL;
    int grid_size = DEFAULT_NB_STATUS;
    int columns = DEFAULT_COLUMNS;

    if (parse_args(argc, argv, &filename, &grid_size, &columns)) {
        return 1;
    }

    // Open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    enableAlternateScreen();

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Load file data into memory
    uint8_t* data = malloc(file_size);
    if (!data) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }
    fread(data, 1, file_size, file);
    fclose(file);

    // Navigate through the file
    size_t current_index = 0;
    char input;

    do {
        render_grid(data, file_size, grid_size, current_index, columns);

        input = get_input();
        switch (input) {
            case 'q': // Quit
                break;
            case 'C': // Right arrow
                if (current_index + grid_size < file_size) {
                    current_index += grid_size;
                }
                break;
            case 'D': // Left arrow
                if (current_index >= grid_size) {
                    current_index -= grid_size;
                }
                break;
            default:
                break;
        }
    } while (input != 'q');

    // Clean up
    free(data);
    disableAlternateScreen();
    return 0;
}
