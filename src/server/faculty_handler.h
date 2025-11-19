#ifndef FACULTY_HANDLER_H
#define FACULTY_HANDLER_H

// Main faculty handler function
int handle_faculty_request(int client_socket, char *request, const char *username);

// Faculty operation functions
int handle_add_course(char *request, char *response, const char *username);
int handle_remove_course(char *request, char *response, const char *username);
int handle_view_enrollments(char *request, char *response);

// Helper functions
int get_faculty_id_by_username(const char *username);
int get_next_course_id();
int count_course_enrollments(int course_id);
int is_course_owner(int course_id, const char *username);

#endif // FACULTY_HANDLER_H