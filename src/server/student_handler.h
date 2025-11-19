#ifndef STUDENT_HANDLER_H
#define STUDENT_HANDLER_H

// Main student handler function
int handle_student_request(int client_socket, char *request, const char *username);

// Student operation functions
int handle_enroll_course(char *request, char *response, const char *username);
int handle_unenroll_course(char *request, char *response, const char *username);
int handle_view_enrolled_courses(char *request, char *response, const char *username);

// REMOVE THIS LINE - it's now in auth.h with different signature
// int handle_password_change(char *request, char *response, const char *username);

// Helper functions
int get_student_id_by_username(const char *username);
int get_enrolled_courses(int student_id, char *buffer, size_t buffer_size);

#endif // STUDENT_HANDLER_H