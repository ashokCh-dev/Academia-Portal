#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "../common/structures.h"
#include "../common/constants.h"

// Network constants
#define DEFAULT_PORT 8080
#define DEFAULT_SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

// File paths
#define DATA_DIR "data/"
#define STUDENT_FILE "data/students.dat"
#define FACULTY_FILE "data/faculty.dat"
#define COURSE_FILE "data/courses.dat"
#define ENROLLMENT_FILE "data/enrollments.dat"
#define CREDENTIALS_FILE "data/credentials.dat"

// Limits
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_NAME_LENGTH 100
#define MAX_EMAIL_LENGTH 100
#define MAX_DEPARTMENT_LENGTH 50
#define MAX_COURSE_CODE_LENGTH 20
#define MAX_COURSE_NAME_LENGTH 100

// Default values
#define DEFAULT_PASSWORD "password123"
#define MAX_LOGIN_ATTEMPTS 3

#endif // CONSTANTS_H