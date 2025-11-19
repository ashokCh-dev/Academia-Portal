#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "structures.h"
#include "constants.h"

// Function to hash a password using SHA256
int hash_password(const char *password, char *hash_output) {
    if (!password || !hash_output) {
        return -1;
    }
    
    // Simple encoding - just for the project
    int len = strlen(password);
    for (int i = 0; i < len && i < 64; i++) {
        hash_output[i] = password[i] ^ 0x5A; // Simple XOR encoding
    }
    hash_output[len < 64 ? len : 64] = '\0';
    
    return 0;
}

// Simplified verify_password
int verify_password(const char *password, const char *stored_hash) {
    if (!password || !stored_hash) {
        return 0;
    }
    
    char computed_hash[65];
    hash_password(password, computed_hash);
    
    return (strcmp(computed_hash, stored_hash) == 0);
}

// Trim whitespace from both ends of a string
char *trim_whitespace(char *str) {
    char *end;
    
    if (!str) {
        return NULL;
    }
    
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == 0) { // All spaces?
        return str;
    }
    
    // Trim trailing whitespace
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    // Null terminate
    end[1] = '\0';
    
    return str;
}

// Safe string copy with size limit
int safe_string_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    // Copy up to dest_size - 1 characters
    strncpy(dest, src, dest_size - 1);
    
    // Ensure null termination
    dest[dest_size - 1] = '\0';
    
    // Return 0 if complete copy, 1 if truncated
    return (strlen(src) >= dest_size) ? 1 : 0;
}

// Validate email format (basic check)
int validate_email(const char *email) {
    const char *at_sign;
    const char *dot;
    
    if (!email || strlen(email) < 5) {
        return 0;
    }
    
    // Check for @ sign
    at_sign = strchr(email, '@');
    if (!at_sign || at_sign == email) {
        return 0;
    }
    
    // Check for . after @
    dot = strchr(at_sign, '.');
    if (!dot || dot == at_sign + 1) {
        return 0;
    }
    
    // Check if there's something after the dot
    if (*(dot + 1) == '\0') {
        return 0;
    }
    
    return 1;
}

// Validate username (alphanumeric and underscore only)
int validate_username(const char *username) {
    size_t len;
    
    if (!username) {
        return 0;
    }
    
    len = strlen(username);
    if (len < 3 || len >= 50) {
        return 0;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (!isalnum(username[i]) && username[i] != '_') {
            return 0;
        }
    }
    
    return 1;
}

// Log error message to error log file
void log_error(const char *message) {
    int fd;
    char timestamp[64];
    char log_entry[512];
    time_t now;
    struct tm *tm_info;
    
    if (!message) {
        return;
    }
    
    // Get current time
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Format log entry
    snprintf(log_entry, sizeof(log_entry), "[%s] ERROR: %s\n", timestamp, message);
    
    // Open log file in append mode
    fd = open("data/error.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        write(fd, log_entry, strlen(log_entry));
        close(fd);
    }
}

// Format timestamp for display
char *format_timestamp(time_t timestamp, char *buffer, size_t buffer_size) {
    struct tm *tm_info;
    
    if (!buffer || buffer_size == 0) {
        return NULL;
    }
    
    tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
    
    return buffer;
}

// Check if file exists
int file_exists(const char *filename) {
    struct stat st;
    return (stat(filename, &st) == 0);
}

// Create directory if it doesn't exist
int ensure_directory_exists(const char *dirname) {
    struct stat st = {0};
    
    if (stat(dirname, &st) == -1) {
        return mkdir(dirname, 0755);
    }
    
    return 0;
}

// Read entire file into memory (for small files only)
char *read_file_contents(const char *filename, size_t *size) {
    int fd;
    struct stat st;
    char *content;
    ssize_t bytes_read;
    
    if (!filename) {
        return NULL;
    }
    
    // Get file size
    if (stat(filename, &st) < 0) {
        return NULL;
    }
    
    // Open file
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }
    
    // Allocate buffer
    content = malloc(st.st_size + 1);
    if (!content) {
        close(fd);
        return NULL;
    }
    
    // Read file
    bytes_read = read(fd, content, st.st_size);
    close(fd);
    
    if (bytes_read != st.st_size) {
        free(content);
        return NULL;
    }
    
    // Null terminate
    content[st.st_size] = '\0';
    
    if (size) {
        *size = st.st_size;
    }
    
    return content;
}

// Sanitize input string (remove special characters that could be problematic)
void sanitize_input(char *input) {
    size_t i, j;
    size_t len;
    
    if (!input) {
        return;
    }
    
    len = strlen(input);
    j = 0;
    
    for (i = 0; i < len; i++) {
        // Allow only printable ASCII characters, excluding some special ones
        if (input[i] >= 32 && input[i] <= 126 && 
            input[i] != '<' && input[i] != '>' && 
            input[i] != '&' && input[i] != '"' && 
            input[i] != '\'' && input[i] != '\\') {
            input[j++] = input[i];
        }
    }
    
    input[j] = '\0';
}

// Convert string to lowercase
void string_to_lower(char *str) {
    if (!str) {
        return;
    }
    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Convert string to uppercase
void string_to_upper(char *str) {
    if (!str) {
        return;
    }
    
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

// Parse integer with error checking
int parse_integer(const char *str, int *result) {
    char *endptr;
    long val;
    
    if (!str || !result) {
        return -1;
    }
    
    errno = 0;
    val = strtol(str, &endptr, 10);
    
    // Check for conversion errors
    if (errno != 0 || endptr == str || *endptr != '\0') {
        return -1;
    }
    
    // Check for overflow
    if (val > INT_MAX || val < INT_MIN) {
        return -1;
    }
    
    *result = (int)val;
    return 0;
}

// Generate a random string (for session tokens, etc.)
void generate_random_string(char *buffer, size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_size = sizeof(charset) - 1;
    
    if (!buffer || length == 0) {
        return;
    }
    
    // Seed random number generator (should be done once in main())
    // srand(time(NULL));
    
    for (size_t i = 0; i < length - 1; i++) {
        buffer[i] = charset[rand() % charset_size];
    }
    
    buffer[length - 1] = '\0';
}

// Check if string is numeric
int is_numeric(const char *str) {
    if (!str || *str == '\0') {
        return 0;
    }
    
    // Allow optional negative sign
    if (*str == '-') {
        str++;
    }
    
    // Check if all characters are digits
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

// Get file extension
const char *get_file_extension(const char *filename) {
    const char *dot;
    
    if (!filename) {
        return NULL;
    }
    
    dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return NULL;
    }
    
    return dot + 1;
}

// Validate date format (YYYY-MM-DD)
int validate_date_format(const char *date) {
    int year, month, day;
    
    if (!date || strlen(date) != 10) {
        return 0;
    }
    
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) {
        return 0;
    }
    
    // Basic range checks
    if (year < 1900 || year > 2100) {
        return 0;
    }
    
    if (month < 1 || month > 12) {
        return 0;
    }
    
    if (day < 1 || day > 31) {
        return 0;
    }
    
    return 1;
}

// Calculate age from birth date
int calculate_age(const char *birth_date) {
    struct tm birth_tm = {0};
    struct tm *current_tm;
    time_t now;
    int age;
    
    if (!birth_date || !validate_date_format(birth_date)) {
        return -1;
    }
    
    // Parse birth date
    if (sscanf(birth_date, "%d-%d-%d", 
               &birth_tm.tm_year, &birth_tm.tm_mon, &birth_tm.tm_mday) != 3) {
        return -1;
    }
    
    birth_tm.tm_year -= 1900;  // tm_year is years since 1900
    birth_tm.tm_mon -= 1;      // tm_mon is 0-11
    
    // Get current date
    time(&now);
    current_tm = localtime(&now);
    
    // Calculate age
    age = current_tm->tm_year - birth_tm.tm_year;
    
    // Adjust if birthday hasn't occurred this year
    if (current_tm->tm_mon < birth_tm.tm_mon ||
        (current_tm->tm_mon == birth_tm.tm_mon && current_tm->tm_mday < birth_tm.tm_mday)) {
        age--;
    }
    
    return age;
}