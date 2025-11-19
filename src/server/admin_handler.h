#ifndef ADMIN_HANDLER_H
#define ADMIN_HANDLER_H

// Main admin handler function
int handle_admin_request(int client_socket, char *request);

// Admin operation functions
int handle_add_student(char *request, char *response);
int handle_add_faculty(char *request, char *response);
int handle_update_student_status(char *request, char *response);
int handle_update_student_details(char *request, char *response);
int handle_update_faculty_details(char *request, char *response);

// Helper functions
int get_next_student_id();
int get_next_faculty_id();
int create_user_credentials(const char *username, const char *role);

#endif // ADMIN_HANDLER_H