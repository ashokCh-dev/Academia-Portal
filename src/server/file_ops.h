#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "../common/structures.h"

// File paths
#define STUDENT_FILE "data/students.dat"
#define FACULTY_FILE "data/faculty.dat"
#define COURSE_FILE "data/courses.dat"
#define ENROLLMENT_FILE "data/enrollments.dat"
#define CREDENTIALS_FILE "data/credentials.dat"

// Student file operations
int read_student_by_id(int id, struct Student *student);
int read_student_by_username(const char *username, struct Student *student);

// Faculty file operations
int read_faculty_by_id(int id, struct Faculty *faculty);

// Course file operations
int read_course_by_id(int id, struct Course *course);
int update_course(struct Course *course);

// Enrollment file operations
int add_enrollment(struct Enrollment *enrollment);
int remove_enrollment(int student_id, int course_id);
int check_enrollment_exists(int student_id, int course_id);
int get_next_enrollment_id();

// Credentials file operations
int update_credentials(const char *username, const char *new_password_hash);

// General helper functions
int get_next_student_id();
int get_next_faculty_id();
int get_next_course_id();

#endif // FILE_OPS_H