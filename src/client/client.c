#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include "../common/structures.h"
#include "../common/constants.h"
#include "ui.h"

// Global variables
int client_socket = -1;
char current_role[10];
char current_username[50];

// Function declarations
int connect_to_server(const char *server_ip, int port);
int authenticate_user();
void handle_admin_operations();
void handle_student_operations();
void handle_faculty_operations();
void send_request(const char *request);
int receive_response(char *buffer, size_t size);
void cleanup();
void signal_handler(int sig);
void disable_echo();
void enable_echo();

int main(int argc, char *argv[]) {
    char server_ip[16] = DEFAULT_SERVER_IP;
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        strncpy(server_ip, argv[1], 15);
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    
    // Set up signal handler for graceful exit
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Connect to server
    if (connect_to_server(server_ip, port) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return 1;
    }
    
    printf("Connected to Academia Portal\n");
    printf("============================\n\n");
    
    // Authenticate user
    if (authenticate_user() < 0) {
        fprintf(stderr, "Authentication failed\n");
        cleanup();
        return 1;
    }
    
    // Display role-based menu
    if (strcmp(current_role, "admin") == 0) {
        handle_admin_operations();
    } else if (strcmp(current_role, "student") == 0) {
        handle_student_operations();
    } else if (strcmp(current_role, "faculty") == 0) {
        handle_faculty_operations();
    }
    
    cleanup();
    return 0;
}

int connect_to_server(const char *server_ip, int port) {
    struct sockaddr_in server_addr;
    
    // Create socket using system call
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_socket);
        return -1;
    }
    
    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        return -1;
    }
    
    return 0;
}

int authenticate_user() {
    char username[50];
    char password[50];
    char request[256];
    char response[256];
    
    // Get login credentials
    printf("Username: ");
    fflush(stdout);
    
    // Use read system call
    int n = read(STDIN_FILENO, username, sizeof(username) - 1);
    if (n > 0) {
        username[n - 1] = '\0'; // Remove newline
    }
    
    printf("Password: ");
    fflush(stdout);
    
    // Disable echo for password input
    disable_echo();
    n = read(STDIN_FILENO, password, sizeof(password) - 1);
    if (n > 0) {
        password[n - 1] = '\0'; // Remove newline
    }
    enable_echo();
    printf("\n");
    
    // Send authentication request
    snprintf(request, sizeof(request), "AUTH:%s:%s", username, password);
    send_request(request);
    
    // Receive response
    receive_response(response, sizeof(response));
    
    // Parse response
    if (strncmp(response, "SUCCESS:", 8) == 0) {
        sscanf(response, "SUCCESS:%s", current_role);
        strcpy(current_username, username);
        printf("Login successful. Role: %s\n\n", current_role);
        return 0;
    } else {
        printf("Login failed: %s\n", response);
        return -1;
    }
}

// Modified version of the handle_admin_operations function
// Add this to client.c to fix the issue with adding students

void handle_admin_operations() {
    int choice;
    char buffer[1024];
    
    while (1) {
        display_admin_menu();
        
        // Use read system call for input
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
            choice = atoi(buffer);
        }
        
        switch (choice) {
            case 1: // Add Student
                {
                    struct Student student;
                    if (get_student_details(&student) == 0) {
                        char request[512];
                        
                        // Debug print to see what we're sending
                        printf("\nPreparing to add student with details:\n");
                        printf("Username: %s\n", student.username);
                        printf("Name: %s\n", student.name);
                        printf("Email: %s\n", student.email);
                        
                        // Ensure we have valid data before sending
                        if (strlen(student.username) > 0 && strlen(student.name) > 0 && strlen(student.email) > 0) {
                            snprintf(request, sizeof(request), "ADD_STUDENT:%s:%s:%s", 
                                    student.username, student.name, student.email);
                            
                            // Debug print the request
                            printf("Sending request: %s\n", request);
                            
                            // Send the request with timeout handling
                            send_request(request);
                            
                            printf("Request sent, waiting for response...\n");
                            
                            // Receive response with proper error handling
                            receive_response(buffer, sizeof(buffer));
                            printf("Server response: %s\n", buffer);
                        } else {
                            printf("Error: One or more required fields are empty\n");
                        }
                    } else {
                        printf("Error getting student details\n");
                    }
                }
                break;
                
            // Other cases remain the same
            case 2: // Add Faculty
                {
                    struct Faculty faculty;
                    if (get_faculty_details(&faculty) == 0) {
                        char request[512];
                        snprintf(request, sizeof(request), "ADD_FACULTY:%s:%s:%s:%s", 
                                faculty.username, faculty.name, faculty.email, faculty.department);
                        send_request(request);
                        receive_response(buffer, sizeof(buffer));
                        printf("Server response: %s\n", buffer);
                    }
                }
                break;
                
            // ... Rest of the function remains unchanged ...
            case 3: // Activate/Deactivate Student
                {
                    char username[50];
                    int status=0;
                    printf("Enter student username: ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)-1) > 0) {
                        buffer[sizeof(buffer) - 1] = '\0';
                                // Remove newline character if present
                        char *newline = strchr(buffer, '\n');
                        if (newline) *newline = '\0';
                        
                        // Copy input to username (safely)
                        strncpy(username, buffer, sizeof(username) - 1);
                        username[sizeof(username) - 1] = '\0';  // Ensure null-termination
                    }
                    printf("Enter status (1=Active, 0=Inactive): ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        status = atoi(buffer);
                    }
                    
                    char request[256];
                    snprintf(request, sizeof(request), "UPDATE_STUDENT_STATUS:%s:%d", username,status);;
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 4: // Update Student/Faculty details
                {
                
                int entity_type=0;
                char username[50];

                printf("Select entity to update:\n");
                printf("1. Student\n");
                printf("2. Faculty\n");
                printf("Choice: ");
                fflush(stdout);

                if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                entity_type = atoi(buffer);
                }

                printf("Enter username: ");
                fflush(stdout);
                int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                if (n > 0) {
                buffer[n - 1] = '\0'; // Remove newline
                strncpy(username, buffer, sizeof(username) - 1);
                }

                if (entity_type == 1) { // Update Student
                // For student: Ask for what to update
                int update_choice=0;
                printf("\nWhat would you like to update?\n");
                printf("1. Status (Active/Inactive)\n");
                printf("2. Name\n");
                printf("3. Email\n");
                printf("Choice: ");
                fflush(stdout);
                if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                update_choice = atoi(buffer);
                }

                char request[512];
                switch(update_choice) {
                case 1: // Status
                {
                    int status=0;
                    printf("Enter status (1=Active, 0=Inactive): ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        status = atoi(buffer);
                    }
                    
                    // Use the existing UPDATE_STUDENT_STATUS command that we know works
                    snprintf(request, sizeof(request), "UPDATE_STUDENT_STATUS:%s:%d", 
                            username, status);
                }
                break;

                case 2: // Name
                {
                    char name[50];
                    printf("Enter new name: ");
                    fflush(stdout);
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n - 1] = '\0'; // Remove newline
                        strncpy(name, buffer, sizeof(name) - 1);
                    }
                    
                    snprintf(request, sizeof(request), "UPDATE_STUDENT_NAME:%s:%s", 
                            username, name);
                }
                break;

                case 3: // Email
                {
                    char email[50];
                    printf("Enter new email: ");
                    fflush(stdout);
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n - 1] = '\0'; // Remove newline
                        strncpy(email, buffer, sizeof(email) - 1);
                    }
                    
                    snprintf(request, sizeof(request), "UPDATE_STUDENT_EMAIL:%s:%s", 
                            username, email);
                }
                break;

                default:
                printf("Invalid choice.\n");
                return;
                }

                printf("Sending request: %s\n", request);
                send_request(request);

                // Get response
                receive_response(buffer, sizeof(buffer));
                printf("Server response: %s\n", buffer);

                } else if (entity_type == 2) { // Update Faculty
                // For faculty: Ask for what to update
                int update_choice=0;
                printf("\nWhat would you like to update?\n");
                printf("1. Name\n");
                printf("2. Email\n");
                printf("3. Department\n");
                printf("Choice: ");
                fflush(stdout);
                if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                update_choice = atoi(buffer);
                }

                char request[512];
                switch(update_choice) {
                case 1: // Name
                {
                    char name[50];
                    printf("Enter new name: ");
                    fflush(stdout);
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n - 1] = '\0'; // Remove newline
                        strncpy(name, buffer, sizeof(name) - 1);
                    }
                    
                    snprintf(request, sizeof(request), "UPDATE_FACULTY_NAME:%s:%s", 
                            username, name);
                }
                break;

                case 2: // Email
                {
                    char email[50];
                    printf("Enter new email: ");
                    fflush(stdout);
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n - 1] = '\0'; // Remove newline
                        strncpy(email, buffer, sizeof(email) - 1);
                    }
                    
                    snprintf(request, sizeof(request), "UPDATE_FACULTY_EMAIL:%s:%s", 
                            username, email);
                }
                break;

                case 3: // Department
                {
                    char department[50];
                    printf("Enter new department: ");
                    fflush(stdout);
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n - 1] = '\0'; // Remove newline
                        strncpy(department, buffer, sizeof(department) - 1);
                    }
                    
                    snprintf(request, sizeof(request), "UPDATE_FACULTY_DEPT:%s:%s", 
                            username, department);
                }
                break;

                default:
                printf("Invalid choice.\n");
                return;
                }

                printf("Sending request: %s\n", request);
                send_request(request);

                // Get response
                receive_response(buffer, sizeof(buffer));
                printf("Server response: %s\n", buffer);
    } else {
        printf("Invalid choice.\n");
    }

                }
                break;
        case 5:
{
    char request[64] = "VIEW_STUDENTS:all"; // Add "all" as a parameter
    send_request(request);
    
    receive_response(buffer, sizeof(buffer));
    
    // Check if response starts with SUCCESS or INFO
    if (strncmp(buffer, "SUCCESS:", 8) == 0) {
        printf("\n%s\n", buffer + 8); // Skip the SUCCESS: prefix
    } else if (strncmp(buffer, "INFO:", 5) == 0) {
        printf("\n%s\n", buffer + 5); // Skip the INFO: prefix
    } else {
        printf("Error: %s\n", buffer);
    }
}
break;

// Also fix the VIEW_FACULTY case
case 6:
{
    char request[64] = "VIEW_FACULTY:all"; // Add "all" as a parameter
    send_request(request);
    
    receive_response(buffer, sizeof(buffer));
    
    // Check if response starts with SUCCESS or INFO
    if (strncmp(buffer, "SUCCESS:", 8) == 0) {
        printf("\n%s\n", buffer + 8); // Skip the SUCCESS: prefix
    } else if (strncmp(buffer, "INFO:", 5) == 0) {
        printf("\n%s\n", buffer + 5); // Skip the INFO: prefix
    } else {
        printf("Error: %s\n", buffer);
    }
}
break;
            case 7: // Exit
                printf("Logging out...\n");
                send_request("LOGOUT");
                return;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
        
        printf("\nPress Enter to continue...");
        fflush(stdout);
        read(STDIN_FILENO, buffer, sizeof(buffer));
    }
}
void handle_student_operations() {
    int choice;
    char buffer[1024];
    
    while (1) {
        display_student_menu();
        
        // Use read system call for input
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
            choice = atoi(buffer);
        }
        
        switch (choice) {
            case 1: // Enroll to new courses
                {
                    int course_id=0;
                    printf("Enter course ID to enroll: ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        course_id = atoi(buffer);
                    }
                    
                    char request[256];
                    snprintf(request, sizeof(request), "ENROLL_COURSE:%d", course_id);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 2: // Unenroll from courses
                {
                    int course_id=0;
                    printf("Enter course ID to unenroll: ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        course_id = atoi(buffer);
                    }
                    
                    char request[256];
                    snprintf(request, sizeof(request), "UNENROLL_COURSE:%d", course_id);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 3: // View enrolled courses
                send_request("VIEW_ENROLLED_COURSES");
                receive_response(buffer, sizeof(buffer));
                printf("Enrolled courses:\n%s\n", buffer);
                break;
                
            case 4: // Password change
                {
                    char new_password[50];
                    printf("Enter new password: ");
                    fflush(stdout);
                    disable_echo();
                    int n = read(STDIN_FILENO, new_password, sizeof(new_password) - 1);
                    if (n > 0) {
                        new_password[n - 1] = '\0';
                    }
                    enable_echo();
                    printf("\n");
                    
                    char request[256];
                    snprintf(request, sizeof(request), "CHANGE_PASSWORD:%s", new_password);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 5: // Exit
                printf("Logging out...\n");
                send_request("LOGOUT");
                return;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
        
        printf("\nPress Enter to continue...");
        fflush(stdout);
        read(STDIN_FILENO, buffer, sizeof(buffer));
    }
}

void handle_faculty_operations() {
    int choice;
    char buffer[1024];
    
    while (1) {
        display_faculty_menu();
        
        // Use read system call for input
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
            choice = atoi(buffer);
        }
        
        switch (choice) {
            case 1: // Add new course
                {
                    struct Course course;
                    if (get_course_details(&course) == 0) {
                        char request[512];
                        snprintf(request, sizeof(request), "ADD_COURSE:%s:%s:%d",
                                course.course_code, course.course_name, course.max_seats);
                        send_request(request);
                        receive_response(buffer, sizeof(buffer));
                        printf("Server response: %s\n", buffer);
                    }
                }
                break;
                
            case 2: // Remove offered course
                {
                    int course_id=0;
                    printf("Enter course ID to remove: ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        course_id = atoi(buffer);
                    }
                    
                    char request[256];
                    snprintf(request, sizeof(request), "REMOVE_COURSE:%d", course_id);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 3: // View enrollments
                {
                    int course_id=0;
                    printf("Enter course ID to view enrollments: ");
                    fflush(stdout);
                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) > 0) {
                        course_id = atoi(buffer);
                    }
                    
                    char request[256];
                    snprintf(request, sizeof(request), "VIEW_ENROLLMENTS:%d", course_id);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Course enrollments:\n%s\n", buffer);
                }
                break;
            case 4: // View all my courses
                {
                    char request[256];
                    snprintf(request, sizeof(request), "VIEW_MY_COURSES");
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("My courses:\n%s\n", buffer);
                    fflush(stdout);
                }
                break;
            case 5: // Password change
                {
                    char new_password[50];
                    printf("Enter new password: ");
                    fflush(stdout);
                    disable_echo();
                    int n = read(STDIN_FILENO, new_password, sizeof(new_password) - 1);
                    if (n > 0) {
                        new_password[n - 1] = '\0';
                    }
                    enable_echo();
                    printf("\n");
                    
                    char request[256];
                    snprintf(request, sizeof(request), "CHANGE_PASSWORD:%s", new_password);
                    send_request(request);
                    receive_response(buffer, sizeof(buffer));
                    printf("Server response: %s\n", buffer);
                }
                break;
                
            case 6: // Exit
                printf("Logging out...\n");
                send_request("LOGOUT");
                return;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
        
        printf("\nPress Enter to continue...");
        fflush(stdout);
        read(STDIN_FILENO, buffer, sizeof(buffer));
    }
}

// Updated send_request function
void send_request(const char *request) {
    // Use write system call
    ssize_t bytes_written = write(client_socket, request, strlen(request));
    if (bytes_written < 0) {
        perror("Failed to send request");
        printf("Error code: %d, Error message: %s\n", errno, strerror(errno));
    } else {
        printf("Request sent successfully. Bytes written: %zd\n", bytes_written);
    }
}

// Updated receive_response function
int receive_response(char *buffer, size_t size) {
    memset(buffer, 0, size);
    // Use read system call
    ssize_t bytes_read = read(client_socket, buffer, size - 1);
    if (bytes_read < 0) {
        perror("Failed to receive response");
        printf("Error code: %d, Error message: %s\n", errno, strerror(errno));
        strcpy(buffer, "ERROR: Failed to receive response");
        return -1;
    } else if (bytes_read == 0) {
        printf("Server closed connection\n");
        strcpy(buffer, "ERROR: Server closed connection");
        return -1;
    } else {
        buffer[bytes_read] = '\0';
        printf("Response received. Bytes read: %zd\n", bytes_read);
        return 0;
    }
}


void cleanup() {
    if (client_socket >= 0) {
        close(client_socket);
    }
}

void signal_handler(int sig) {
    printf("\nReceived signal %d. Cleaning up...\n", sig);
    cleanup();
    exit(0);
}

void disable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void enable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}