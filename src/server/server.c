#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include "../common/structures.h"
#include "../common/constants.h"
#include "auth.h"
#include "admin_handler.h"
#include "student_handler.h"
#include "faculty_handler.h"

// Global variables
int server_socket = -1;
volatile sig_atomic_t running = 1;

// Client session structure
struct ClientSession {
    int socket;
    char username[50];
    char role[10];
    int authenticated;
};

// Function declarations
int create_server_socket(int port);
void handle_client(int client_socket);
void *client_thread(void *arg);
void handle_authentication(struct ClientSession *session, char *request);
void handle_request(struct ClientSession *session, char *request);
void signal_handler(int sig);
void cleanup_server();
void setup_data_directory();
void reap_zombies(int sig);

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, reap_zombies);  // Handle zombie processes
    signal(SIGPIPE, SIG_IGN);       // Ignore broken pipe signals
    
    // Setup data directory
    setup_data_directory();
    
    // Create server socket
    server_socket = create_server_socket(port);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to create server socket\n");
        return 1;
    }
    
    printf("Academia Portal Server started on port %d\n", port);
    printf("Press Ctrl+C to stop the server\n");
    
    // Main server loop
    while (running) {
        // Accept client connections
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, retry
            }
            perror("Accept failed");
            continue;
        }
        
        printf("New client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Create new thread for client
        pthread_t client_tid;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;
        
        if (pthread_create(&client_tid, NULL, client_thread, client_sock_ptr) != 0) {
            perror("Failed to create client thread");
            close(client_socket);
            free(client_sock_ptr);
        } else {
            pthread_detach(client_tid);  // Detach thread to clean up automatically
        }
    }
    
    cleanup_server();
    return 0;
}

int create_server_socket(int port) {
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set socket options
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sock);
        return -1;
    }
    
    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return -1;
    }
    
    // Listen for connections
    if (listen(sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(sock);
        return -1;
    }
    
    return sock;
}

void *client_thread(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    
    handle_client(client_socket);
    
    return NULL;
}

void handle_client(int client_socket) {
    struct ClientSession session;
    char request[1024];
    char response[1024];
    ssize_t bytes_read;
    
    // Initialize session
    memset(&session, 0, sizeof(session));
    session.socket = client_socket;
    session.authenticated = 0;
    
    // Client communication loop
    while (1) {
        // Read request from client
        memset(request, 0, sizeof(request));
        bytes_read = read(client_socket, request, sizeof(request) - 1);
        
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("Read error");
            }
            break;
        }
        
        // Remove trailing newline if present
        request[strcspn(request, "\n")] = '\0';
        
        // Handle authentication for first request
        if (!session.authenticated) {
            if (strncmp(request, "AUTH:", 5) == 0) {
                handle_authentication(&session, request);
            } else {
                strcpy(response, "ERROR: Not authenticated");
                write(client_socket, response, strlen(response));
            }
        } else {
            // Handle authenticated requests
            if (strcmp(request, "LOGOUT") == 0) {
                printf("User %s logged out\n", session.username);
                strcpy(response, "SUCCESS: Logged out");
                write(client_socket, response, strlen(response));
                break;
            } else {
                handle_request(&session, request);
            }
        }
    }
    
    // Clean up
    close(client_socket);
    printf("Client disconnected\n");
}

void handle_authentication(struct ClientSession *session, char *request) {
    char username[50];
    char password[50];
    char response[256];
    
    // Parse authentication request
    if (sscanf(request, "AUTH:%[^:]:%s", username, password) != 2) {
        strcpy(response, "ERROR:Invalid authentication format");
        write(session->socket, response, strlen(response));
        return;
    }
    
    // Authenticate user
    char role[10];
    if (authenticate_user(username, password, role) == 0) {
        // Authentication successful
        session->authenticated = 1;
        strncpy(session->username, username, sizeof(session->username) - 1);
        strncpy(session->role, role, sizeof(session->role) - 1);
        
        sprintf(response, "SUCCESS:%s", role);
        printf("User %s authenticated as %s\n", username, role);
    } else {
        strcpy(response, "ERROR:Invalid credentials");
    }
    
    write(session->socket, response, strlen(response));
}

void handle_request(struct ClientSession *session, char *request) {
    // Route request based on user role
    if (strcmp(session->role, "admin") == 0) {
        handle_admin_request(session->socket, request);
    } else if (strcmp(session->role, "student") == 0) {
        handle_student_request(session->socket, request, session->username);
    } else if (strcmp(session->role, "faculty") == 0) {
        handle_faculty_request(session->socket, request, session->username);
    } else {
        char response[256];
        strcpy(response, "ERROR:Unknown role");
        write(session->socket, response, strlen(response));
    }
}

void signal_handler(int sig) {
    printf("\nReceived signal %d. Shutting down server...\n", sig);
    running = 0;
    
    // Close server socket to interrupt accept()
    if (server_socket >= 0) {
        close(server_socket);
    }
}

void reap_zombies(int sig) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Reap zombie child processes
    }
}

void cleanup_server() {
    printf("Cleaning up resources...\n");
    
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    printf("Server shutdown complete.\n");
}

void setup_data_directory() {
    struct stat st = {0};
    
    // Create data directory if it doesn't exist
    if (stat("data", &st) == -1) {
        mkdir("data", 0755);
        printf("Created data directory\n");
    }
    
    // Initialize system with default admin account if no credentials exist
    int fd = open("data/credentials.dat", O_RDONLY);
    if (fd < 0) {
        // Create default admin credentials
        struct Credentials admin_cred;
        strcpy(admin_cred.username, "admin");
        strcpy(admin_cred.password_hash, "admin123");  // Default password
        strcpy(admin_cred.role, "admin");
        
        fd = open("data/credentials.dat", O_WRONLY | O_CREAT, 0600);
        if (fd >= 0) {
            write(fd, &admin_cred, sizeof(struct Credentials));
            close(fd);
            printf("Created default admin account (username: admin, password: admin123)\n");
        }
    } else {
        close(fd);
    }
}