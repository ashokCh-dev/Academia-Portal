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
// Function declarations
int authenticate_user(const char *username, const char *password, char *role);
int verify_credentials(const char *username, const char *password, struct Credentials *cred);
int change_password(const char *username, const char *new_password);
char* hash_password(const char *password);
int compare_passwords(const char *password, const char *hash);
// Add this to the function declarations section
int update_user_password(const char *username, const char *new_password);


// Function to check if a student is active
int check_student_active_status(const char *username) {
    struct Student student;
    int fd;
    int is_active = -1; // Default: -1 means not found, 0 means inactive, 1 means active
    
    // Open student file
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        return -1; // File error
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for the student record
    while (read(fd, &student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (strcmp(student.username, username) == 0) {
            // Found student - check active status
            is_active = student.active;
            break;
        }
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    return is_active;
}
// Modified handle_auth_request function to handle the extended error codes
int handle_auth_request(int client_socket, char *request) {
    char response[256];
    char username[50];
    char password[50];
    char role[10];
    int auth_result;
    
    // Parse authentication request
    if (sscanf(request, "AUTH:%[^:]:%s", username, password) != 2) {
        strcpy(response, "ERROR:Invalid authentication format");
        write(client_socket, response, strlen(response));
        return -1;
    }
    
    // Authenticate user
    auth_result = authenticate_user(username, password, role);
    if (auth_result == 0) {
        sprintf(response, "SUCCESS:%s", role);
    } else if (auth_result == -2) {
        strcpy(response, "ERROR:Account is inactive. Please contact administrator.");
    } else if (auth_result == -3) {
        strcpy(response, "ERROR:Student record not found. Please contact administrator.");
    } else {
        strcpy(response, "ERROR:Invalid username or password");
    }
    
    // Send response
    write(client_socket, response, strlen(response));
    return auth_result;
}

int authenticate_user(const char *username, const char *password, char *role) {
    struct Credentials cred;
    int fd;
    
    // Open credentials file
    fd = open(CREDENTIALS_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for user credentials
    while (read(fd, &cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
        if (strcmp(cred.username, username) == 0) {
            // Found user - verify password
            if (verify_credentials(username, password, &cred) == 0) {
                // Check if user is a student and if yes, verify active status
                if (strcmp(cred.role, "student") == 0) {
                    int active_status = check_student_active_status(username);
                    
                    if (active_status == 0) {
                        // Student exists but is inactive
                        flock(fd, LOCK_UN);
                        close(fd);
                        return -2; // Special error code for inactive student
                    } else if (active_status == -1) {
                        // Student credentials exist but no student record found (unusual case)
                        flock(fd, LOCK_UN);
                        close(fd);
                        return -3; // Special error code for missing student record
                    }
                }
                
                // If we get here, either user is not a student or is an active student
                strcpy(role, cred.role);
                flock(fd, LOCK_UN);
                close(fd);
                return 0; // Success
            }
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    return -1; // General authentication failure
}

int verify_credentials(const char *username, const char *password, struct Credentials *cred) {
    // For now, using simple string comparison
    // In production, this should use proper password hashing
    if (strcmp(cred->password_hash, password) == 0 || 
        strcmp(cred->password_hash, "default") == 0) {
        return 0;
    }
    return -1;
}

int change_password(const char *username, const char *new_password) {
    struct Credentials cred;
    int fd;
    off_t offset = 0;
    int found = 0;
    
    // Open credentials file for read/write
    fd = open(CREDENTIALS_FILE, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    
    // Find and update user credentials
    while (read(fd, &cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
        if (strcmp(cred.username, username) == 0) {
            // Update password
            strncpy(cred.password_hash, new_password, sizeof(cred.password_hash) - 1);
            
            // Seek back to the record position
            lseek(fd, offset, SEEK_SET);
            
            // Write updated record
            if (write(fd, &cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
                found = 1;
            }
            break;
        }
        offset += sizeof(struct Credentials);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

// Handle password change request
int handle_password_change(int client_socket, char *request, const char *username) {
    char response[256];
    char new_password[50];
    char command[256];
    char params[768];
    
    // Parse the request
    if (sscanf(request, "%[^:]:%[^\n]", command, params) != 2) {
        strcpy(response, "ERROR:Invalid password change format");
        write(client_socket, response, strlen(response));
        return -1;
    }
    
    // Extract new password
    strncpy(new_password, params, sizeof(new_password) - 1);
    new_password[sizeof(new_password) - 1] = '\0';
    
    // Update the password in credentials file
    if (update_user_password(username, new_password) < 0) {
        strcpy(response, "ERROR:Failed to update password");
    } else {
        strcpy(response, "SUCCESS:Password changed successfully");
    }
    
    write(client_socket, response, strlen(response));
    return 0;
}
int update_user_password(const char *username, const char *new_password) {
    struct Credentials cred;
    int fd;
    off_t offset = 0;
    int found = 0;
    
    fd = open("data/credentials.dat", O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    flock(fd, LOCK_EX);
    
    // Find and update user password
    while (read(fd, &cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
        if (strcmp(cred.username, username) == 0) {
            // Update password (using simple storage for academic project)
            strncpy(cred.password_hash, new_password, sizeof(cred.password_hash) - 1);
            
            // Seek back to the record position
            lseek(fd, offset, SEEK_SET);
            
            // Write updated record
            if (write(fd, &cred, sizeof(struct Credentials)) != sizeof(struct Credentials)) {
                flock(fd, LOCK_UN);
                close(fd);
                return -1;
            }
            found = 1;
            break;
        }
        offset += sizeof(struct Credentials);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}
// Simple password hashing (in production, use proper crypto library)

int compare_passwords(const char *password, const char *hash) {
    // For demo purposes, simple comparison
    return strcmp(password, hash);
}

// Create initial admin account if it doesn't exist
int create_initial_admin() {
    struct Credentials admin_cred;
    int fd;
    int admin_exists = 0;
    
    // Check if admin already exists
    fd = open(CREDENTIALS_FILE, O_RDONLY);
    if (fd >= 0) {
        flock(fd, LOCK_SH);
        while (read(fd, &admin_cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
            if (strcmp(admin_cred.username, "admin") == 0) {
                admin_exists = 1;
                break;
            }
        }
        flock(fd, LOCK_UN);
        close(fd);
    }
    
    if (!admin_exists) {
        // Create admin account
        strcpy(admin_cred.username, "admin");
        strcpy(admin_cred.password_hash, "admin123");
        strcpy(admin_cred.role, "admin");
        
        fd = open(CREDENTIALS_FILE, O_WRONLY | O_APPEND | O_CREAT, 0600);
        if (fd >= 0) {
            flock(fd, LOCK_EX);
            write(fd, &admin_cred, sizeof(struct Credentials));
            flock(fd, LOCK_UN);
            close(fd);
            printf("Initial admin account created (username: admin, password: admin123)\n");
        }
    }
    
    return 0;
}