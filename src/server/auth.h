#ifndef AUTH_H
#define AUTH_H

#include "../common/structures.h"
// Add to auth.h
/**
 * Update a user's password
 * @param username Username whose password to update
 * @param new_password New password to set
 * @return 0 on success, -1 on failure
 */
int update_user_password(const char *username, const char *new_password);
// Function declarations for authentication

// Main authentication handler
int handle_auth_request(int client_socket, char *request);

// Authentication functions
int authenticate_user(const char *username, const char *password, char *role);
int verify_credentials(const char *username, const char *password, struct Credentials *cred);

// Password management
int change_password(const char *username, const char *new_password);
int handle_password_change(int client_socket, char *request, const char *username);
int compare_passwords(const char *password, const char *hash);

// Initial setup
int create_initial_admin();

// User management
int create_user_credentials(const char *username, const char *role);
int get_user_role(const char *username, char *role);
int user_exists(const char *username);

#endif // AUTH_H