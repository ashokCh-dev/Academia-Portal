#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>
#include "../common/structures.h"
#include "../common/constants.h"
#include "auth.h"
#include "file_ops.h"

// File paths
#define STUDENT_FILE "data/students.dat"
#define FACULTY_FILE "data/faculty.dat"
#define CREDENTIALS_FILE "data/credentials.dat"

// Function declarations
int handle_add_student(char *request, char *response);
int handle_add_faculty(char *request, char *response);
int handle_update_student_status(char *request, char *response);
int handle_update_student_name(char *request, char *response);
int handle_update_student_email(char *request, char *response);
int handle_update_faculty_name(char *request, char *response);
int handle_update_faculty_email(char *request, char *response);
int handle_update_faculty_dept(char *request, char *response);
int get_next_student_id();
int get_next_faculty_id();
int create_user_credentials(const char *username, const char *role);
int handle_view_students(char *params, char *response);
int handle_view_faculty(char *params, char *response);
// int handle_view_student_by_username(char *params, char *response);
// int handle_view_faculty_by_username(char *params, char *response);


// Main admin handler function
int handle_admin_request(int client_socket, char *request) {
    char response[1024];
    char command[256];
    char params[768];
    int result = 0;
    
    // Parse the request
    if (sscanf(request, "%[^:]:%[^\n]", command, params) != 2) {
        strcpy(response, "ERROR:Invalid request format");
        write(client_socket, response, strlen(response));
        return -1;
    }
    
    // Handle different admin commands
    if (strcmp(command, "ADD_STUDENT") == 0) {
        result = handle_add_student(params, response);
    } else if (strcmp(command, "ADD_FACULTY") == 0) {
        result = handle_add_faculty(params, response);
    } else if (strcmp(command, "UPDATE_STUDENT_STATUS") == 0) {
        result = handle_update_student_status(params, response);
    } else if (strcmp(command, "UPDATE_STUDENT_NAME") == 0) {
        result = handle_update_student_name(params, response);
    } else if (strcmp(command, "UPDATE_STUDENT_EMAIL") == 0) {
        result = handle_update_student_email(params, response);
    } else if (strcmp(command, "UPDATE_FACULTY_NAME") == 0) {
        result = handle_update_faculty_name(params, response);
    } else if (strcmp(command, "UPDATE_FACULTY_EMAIL") == 0) {
        result = handle_update_faculty_email(params, response);
    } else if (strcmp(command, "UPDATE_FACULTY_DEPT") == 0) {
        result = handle_update_faculty_dept(params, response);
    } else if (strcmp(command, "VIEW_STUDENTS") == 0) {
        result = handle_view_students(params, response);
    } else if (strcmp(command, "VIEW_FACULTY") == 0) {
        result = handle_view_faculty(params, response);
    // } else if (strcmp(command, "VIEW_STUDENT") == 0) {
    //     result = handle_view_student_by_username(params, response);
    // } else if (strcmp(command, "VIEW_FACULTY_MEMBER") == 0) {
    //     result = handle_view_faculty_by_username(params, response);
    } else {
        strcpy(response, "ERROR:Unknown admin command");
    }
    
    // Send response to client
    write(client_socket, response, strlen(response));
    return result;
}

// Helper function to find a student by username
int handle_view_students(char *params, char *response) {
    struct Student student;
    int fd;
    char temp_buffer[2048] = ""; // Temporary buffer to hold all student data
    char student_info[256];
    
    // Open file with read lock
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open student file: %s", strerror(errno));
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock student file: %s", strerror(errno));
        return -1;
    }
    
    // Read and format student records
    int count = 0;
    strcat(temp_buffer, "ID | Username | Name | Email | Status\n");
    strcat(temp_buffer, "----------------------------------------\n");
    
    while (read(fd, &student, sizeof(struct Student)) == sizeof(struct Student)) {
        snprintf(student_info, sizeof(student_info), "%d | %s | %s | %s | %s\n", 
                student.id, 
                student.username, 
                student.name, 
                student.email, 
                student.active ? "Active" : "Inactive");
        
        // Check if we have enough space in the buffer
        if (strlen(temp_buffer) + strlen(student_info) < sizeof(temp_buffer) - 1) {
            strcat(temp_buffer, student_info);
            count++;
        } else {
            // Response buffer is getting full
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    if (count == 0) {
        strcpy(response, "INFO:No students found");
    } else {
        snprintf(response, 1024, "SUCCESS:%s", temp_buffer);
    }
    
    return 0;
}

// Function to view all faculty members
int handle_view_faculty(char *params, char *response) {
    struct Faculty faculty;
    int fd;
    char temp_buffer[2048] = ""; // Temporary buffer to hold all faculty data
    char faculty_info[256];
    
    // Open file with read lock
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Read and format faculty records
    int count = 0;
    strcat(temp_buffer, "ID | Username | Name | Email | Department\n");
    strcat(temp_buffer, "----------------------------------------\n");
    
    while (read(fd, &faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        snprintf(faculty_info, sizeof(faculty_info), "%d | %s | %s | %s | %s\n", 
                faculty.id, 
                faculty.username, 
                faculty.name, 
                faculty.email, 
                faculty.department);
        
        // Check if we have enough space in the buffer
        if (strlen(temp_buffer) + strlen(faculty_info) < sizeof(temp_buffer) - 1) {
            strcat(temp_buffer, faculty_info);
            count++;
        } else {
            // Response buffer is getting full
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    if (count == 0) {
        strcpy(response, "INFO:No faculty members found");
    } else {
        snprintf(response, 1024, "SUCCESS:%s", temp_buffer);
    }
    
    return 0;
}

// Function to view a specific student by username
// int handle_view_student_by_username(char *params, char *response) {
//     char username[50];
//     struct Student student;
//     off_t offset;
    
//     // Parse parameters
//     if (sscanf(params, "%s", username) != 1) {
//         strcpy(response, "ERROR:Invalid parameters for VIEW_STUDENT");
//         return -1;
//     }
    
//     // Find student by username
//     if (find_student_by_username(username, &student, &offset) < 0) {
//         sprintf(response, "ERROR:Student with username '%s' not found", username);
//         return -1;
//     }
    
//     // Format student information
//     char student_info[512];
//     snprintf(student_info, sizeof(student_info), 
//              "Student Information:\nID: %d\nUsername: %s\nName: %s\nEmail: %s\nStatus: %s", 
//              student.id, 
//              student.username, 
//              student.name, 
//              student.email, 
//              student.active ? "Active" : "Inactive");
    
//     sprintf(response, "SUCCESS:%s", student_info);
//     return 0;
// }
// int find_student_by_username(const char *username, struct Student *student, off_t *offset) {
//     int fd;
//     *offset = 0;
    
//     fd = open(STUDENT_FILE, O_RDONLY);
//     if (fd < 0) {
//         return -1;
//     }
    
//     // Apply read lock
//     if (flock(fd, LOCK_SH) < 0) {
//         close(fd);
//         return -1;
//     }
    
//     while (read(fd, student, sizeof(struct Student)) == sizeof(struct Student)) {
//         if (strcmp(student->username, username) == 0) {
//             flock(fd, LOCK_UN);
//             close(fd);
//             return 0; // Found
//         }
//         *offset += sizeof(struct Student);
//     }
    
//     flock(fd, LOCK_UN);
//     close(fd);
//     return -1; // Not found
// }

/**
 * Finds a faculty member by username and returns the record and its offset
//  */
// int find_faculty_by_username(const char *username, struct Faculty *faculty, off_t *offset) {
//     int fd;
//     *offset = 0;
    
//     fd = open(FACULTY_FILE, O_RDONLY);
//     if (fd < 0) {
//         return -1;
//     }
    
//     // Apply read lock
//     if (flock(fd, LOCK_SH) < 0) {
//         close(fd);
//         return -1;
//     }
    
//     while (read(fd, faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
//         if (strcmp(faculty->username, username) == 0) {
//             flock(fd, LOCK_UN);
//             close(fd);
//             return 0; // Found
//         }
//         *offset += sizeof(struct Faculty);
//     }
    
//     flock(fd, LOCK_UN);
//     close(fd);
//     return -1; // Not found
// }
// Function to view a specific faculty member by username
// int handle_view_faculty_by_username(char *params, char *response) {
//     char username[50];
//     struct Faculty faculty;
//     off_t offset;
    
//     // Parse parameters
//     if (sscanf(params, "%s", username) != 1) {
//         strcpy(response, "ERROR:Invalid parameters for VIEW_FACULTY_MEMBER");
//         return -1;
//     }
    
//     // Find faculty by username
//     if (find_faculty_by_username(username, &faculty, &offset) < 0) {
//         sprintf(response, "ERROR:Faculty with username '%s' not found", username);
//         return -1;
//     }
    
//     // Format faculty information
//     char faculty_info[512];
//     snprintf(faculty_info, sizeof(faculty_info), 
//              "Faculty Information:\nID: %d\nUsername: %s\nName: %s\nEmail: %s\nDepartment: %s", 
//              faculty.id, 
//              faculty.username, 
//              faculty.name, 
//              faculty.email, 
//              faculty.department);
    
//     sprintf(response, "SUCCESS:%s", faculty_info);
//     return 0;
// }
int find_student_by_username(const char *username, struct Student *student, off_t *offset) {
    int fd;
    *offset = 0;
    
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    while (read(fd, student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (strcmp(student->username, username) == 0) {
            flock(fd, LOCK_UN);
            close(fd);
            return 0; // Found
        }
        *offset += sizeof(struct Student);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    return -1; // Not found
}

// Helper function to find a faculty by username
int find_faculty_by_username(const char *username, struct Faculty *faculty, off_t *offset) {
    int fd;
    *offset = 0;
    
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    while (read(fd, faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (strcmp(faculty->username, username) == 0) {
            flock(fd, LOCK_UN);
            close(fd);
            return 0; // Found
        }
        *offset += sizeof(struct Faculty);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    return -1; // Not found
}

// Check if student username already exists in the students file
int student_username_exists(const char *username) {
    struct Student student;
    int fd;
    int exists = 0;
    
    // Open student file
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        // If the file doesn't exist yet, username certainly doesn't exist
        if (errno == ENOENT) {
            return 0;
        }
        return -1; // Other error opening file
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for the username
    while (read(fd, &student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (strcmp(student.username, username) == 0) {
            // Username already exists
            exists = 1;
            break;
        }
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    return exists;
}

// Updated handle_add_student function with duplicate username check
// and improved debugging
int handle_add_student(char *params, char *response) {
    struct Student student;
    char username[50], name[100], email[100];
    int fd;
    int check_result;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^:]:%s", username, name, email) != 3) {
        strcpy(response, "ERROR:Invalid parameters for ADD_STUDENT");
        return -1;
    }
    
    // Check if username already exists in student file
    check_result = student_username_exists(username);
    if (check_result < 0) {
        strcpy(response, "ERROR:Failed to check for existing username");
        return -1;
    } else if (check_result > 0) {
        strcpy(response, "ERROR:Username already exists. Please choose a different username");
        return -2;
    }
    
    // Fill student structure
    student.id = get_next_student_id();
    strncpy(student.username, username, sizeof(student.username) - 1);
    student.username[sizeof(student.username) - 1] = '\0'; // Ensure null termination
    
    strncpy(student.name, name, sizeof(student.name) - 1);
    student.name[sizeof(student.name) - 1] = '\0'; // Ensure null termination
    
    strncpy(student.email, email, sizeof(student.email) - 1);
    student.email[sizeof(student.email) - 1] = '\0'; // Ensure null termination
    
    student.active = 1;
    
    // Open file with write lock
    fd = open(STUDENT_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open student file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock student file: %s", strerror(errno));
        return -1;
    }
    
    // Write student record
    if (write(fd, &student, sizeof(struct Student)) != sizeof(struct Student)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to write student record: %s", strerror(errno));
        return -1;
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    // Create user credentials
    if (create_user_credentials(username, "student") < 0) {
        sprintf(response, "WARNING:Student added but failed to create credentials");
        return 0;
    }
    
    sprintf(response, "SUCCESS:Student added with ID %d", student.id);
    return 0;
}

// Check if faculty username already exists in the faculty file
int faculty_username_exists(const char *username) {
    struct Faculty faculty;
    int fd;
    int exists = 0;
    
    // Open faculty file
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        // If the file doesn't exist yet, username certainly doesn't exist
        if (errno == ENOENT) {
            return 0;
        }
        return -1; // Other error opening file
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for the username
    while (read(fd, &faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (strcmp(faculty.username, username) == 0) {
            // Username already exists
            exists = 1;
            break;
        }
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    return exists;
}

int handle_add_faculty(char *params, char *response) {
    struct Faculty faculty;
    char username[50], name[100], email[100], department[50];
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^:]:%[^:]:%s", username, name, email, department) != 4) {
        strcpy(response, "ERROR:Invalid parameters for ADD_FACULTY");
        return -1;
    }
    
    // Check if username already exists
    int username_check = faculty_username_exists(username);
    if (username_check < 0) {
        sprintf(response, "ERROR:Failed to check username existence");
        return -1;
    }
    if (username_check > 0) {
        sprintf(response, "ERROR:Faculty username '%s' already exists", username);
        return -1;
    }
    
    // Fill faculty structure
    faculty.id = get_next_faculty_id();
    strncpy(faculty.username, username, sizeof(faculty.username) - 1);
    strncpy(faculty.name, name, sizeof(faculty.name) - 1);
    strncpy(faculty.email, email, sizeof(faculty.email) - 1);
    strncpy(faculty.department, department, sizeof(faculty.department) - 1);
    
    // Open file with write lock
    fd = open(FACULTY_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Write faculty record
    if (write(fd, &faculty, sizeof(struct Faculty)) != sizeof(struct Faculty)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to write faculty record: %s", strerror(errno));
        return -1;
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    // Create user credentials
    if (create_user_credentials(username, "faculty") < 0) {
        sprintf(response, "WARNING:Faculty added but failed to create credentials");
        return 0;
    }
    
    sprintf(response, "SUCCESS:Faculty added with ID %d", faculty.id);
    return 0;
}

int handle_update_student_status(char *params, char *response) {
    char username[50];
    int status;
    struct Student student;
    off_t offset;
    int fd;
    
    // Parse parameters - Updated to expect username instead of ID
    if (sscanf(params, "%[^:]:%d", username, &status) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_STUDENT_STATUS");
        return -1;
    }
    
    // Find student by username
    if (find_student_by_username(username, &student, &offset) < 0) {
        sprintf(response, "ERROR:Student with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(STUDENT_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open student file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock student file: %s", strerror(errno));
        return -1;
    }
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Update status
    student.active = status;
    
    // Write updated record
    if (write(fd, &student, sizeof(struct Student)) != sizeof(struct Student)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update student status: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Student status updated for %s", username);
    return 0;
}

int handle_update_student_name(char *params, char *response) {
    char username[50], name[100];
    struct Student student;
    off_t offset;
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^\n]", username, name) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_STUDENT_NAME");
        return -1;
    }
    
    // Find student by username
    if (find_student_by_username(username, &student, &offset) < 0) {
        sprintf(response, "ERROR:Student with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(STUDENT_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open student file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock student file: %s", strerror(errno));
        return -1;
    }
    
    // Update student name
    strncpy(student.name, name, sizeof(student.name) - 1);
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Write updated record
    if (write(fd, &student, sizeof(struct Student)) != sizeof(struct Student)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update student name: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Student name updated for %s", username);
    return 0;
}

int handle_update_student_email(char *params, char *response) {
    char username[50], email[100];
    struct Student student;
    off_t offset;
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^\n]", username, email) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_STUDENT_EMAIL");
        return -1;
    }
    
    // Find student by username
    if (find_student_by_username(username, &student, &offset) < 0) {
        sprintf(response, "ERROR:Student with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(STUDENT_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open student file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock student file: %s", strerror(errno));
        return -1;
    }
    
    // Update student email
    strncpy(student.email, email, sizeof(student.email) - 1);
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Write updated record
    if (write(fd, &student, sizeof(struct Student)) != sizeof(struct Student)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update student email: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Student email updated for %s", username);
    return 0;
}

int handle_update_faculty_name(char *params, char *response) {
    char username[50], name[100];
    struct Faculty faculty;
    off_t offset;
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^\n]", username, name) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_FACULTY_NAME");
        return -1;
    }
    
    // Find faculty by username
    if (find_faculty_by_username(username, &faculty, &offset) < 0) {
        sprintf(response, "ERROR:Faculty with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(FACULTY_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Update faculty name
    strncpy(faculty.name, name, sizeof(faculty.name) - 1);
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Write updated record
    if (write(fd, &faculty, sizeof(struct Faculty)) != sizeof(struct Faculty)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update faculty name: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Faculty name updated for %s", username);
    return 0;
}

int handle_update_faculty_email(char *params, char *response) {
    char username[50], email[100];
    struct Faculty faculty;
    off_t offset;
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^\n]", username, email) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_FACULTY_EMAIL");
        return -1;
    }
    
    // Find faculty by username
    if (find_faculty_by_username(username, &faculty, &offset) < 0) {
        sprintf(response, "ERROR:Faculty with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(FACULTY_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Update faculty email
    strncpy(faculty.email, email, sizeof(faculty.email) - 1);
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Write updated record
    if (write(fd, &faculty, sizeof(struct Faculty)) != sizeof(struct Faculty)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update faculty email: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Faculty email updated for %s", username);
    return 0;
}

int handle_update_faculty_dept(char *params, char *response) {
    char username[50], department[100];
    struct Faculty faculty;
    off_t offset;
    int fd;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^\n]", username, department) != 2) {
        strcpy(response, "ERROR:Invalid parameters for UPDATE_FACULTY_DEPT");
        return -1;
    }
    
    // Find faculty by username
    if (find_faculty_by_username(username, &faculty, &offset) < 0) {
        sprintf(response, "ERROR:Faculty with username '%s' not found", username);
        return -1;
    }
    
    // Open file with read/write access
    fd = open(FACULTY_FILE, O_RDWR);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock faculty file: %s", strerror(errno));
        return -1;
    }
    
    // Update faculty department
    strncpy(faculty.department, department, sizeof(faculty.department) - 1);
    
    // Seek to the record position
    lseek(fd, offset, SEEK_SET);
    
    // Write updated record
    if (write(fd, &faculty, sizeof(struct Faculty)) != sizeof(struct Faculty)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to update faculty department: %s", strerror(errno));
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    sprintf(response, "SUCCESS:Faculty department updated for %s", username);
    return 0;
}

int get_next_student_id() {
    struct Student student;
    int fd, max_id = 0;
    
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        return 1; // First student
    }
    
    // Apply read lock
    flock(fd, LOCK_SH);
    
    while (read(fd, &student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (student.id > max_id) {
            max_id = student.id;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return max_id + 1;
}

int get_next_faculty_id() {
    struct Faculty faculty;
    int fd, max_id = 0;
    
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        return 1; // First faculty
    }
    
    // Apply read lock
    flock(fd, LOCK_SH);
    
    while (read(fd, &faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (faculty.id > max_id) {
            max_id = faculty.id;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return max_id + 1;
}

int create_user_credentials(const char *username, const char *role) {
    struct Credentials cred;
    int fd;
    
    // Fill credentials structure
    strncpy(cred.username, username, sizeof(cred.username) - 1);
    strncpy(cred.password_hash, "default", sizeof(cred.password_hash) - 1); // Default password
    strncpy(cred.role, role, sizeof(cred.role) - 1);
    
    // Open credentials file
    fd = open(CREDENTIALS_FILE, O_WRONLY | O_APPEND | O_CREAT, 0600);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    
    // Write credentials
    if (write(fd, &cred, sizeof(struct Credentials)) != sizeof(struct Credentials)) {
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return 0;
}