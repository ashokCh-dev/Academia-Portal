#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../common/structures.h"
#include "ui.h"

void clear_screen() {
    // Using ANSI escape codes
    printf("\033[H\033[J");
}

void display_admin_menu() {
    clear_screen();
    printf("=================================\n");
    printf("     ADMIN MENU - Academia       \n");
    printf("=================================\n");
    printf("1. Add Student\n");
    printf("2. Add Faculty\n");
    printf("3. Activate/Deactivate Student\n");
    printf("4. Update Student/Faculty Details\n");
    printf("5. View Students\n");
    printf("6. View Faculty\n");
    printf("7. Exit\n");
    printf("=================================\n");
    printf("Enter your choice: ");
    fflush(stdout);
}

void display_student_menu() {
    clear_screen();
    printf("=================================\n");
    printf("    STUDENT MENU - Academia      \n");
    printf("=================================\n");
    printf("1. Enroll to New Courses\n");
    printf("2. Unenroll from Courses\n");
    printf("3. View Enrolled Courses\n");
    printf("4. Change Password\n");
    printf("5. Exit\n");
    printf("=================================\n");
    printf("Enter your choice: ");
    fflush(stdout);
}

void display_faculty_menu() {
    clear_screen();
    printf("=================================\n");
    printf("    FACULTY MENU - Academia      \n");
    printf("=================================\n");
    printf("1. Add New Course\n");
    printf("2. Remove Offered Course\n");
    printf("3. View Course Enrollments\n");
    printf("4. View Courses offered\n");  
    printf("5. Change Password\n");
    printf("6. Exit\n");
    printf("=================================\n");
    printf("Enter your choice: ");
    fflush(stdout);
}

int get_student_details(struct Student *student) {
    char buffer[256];
    int n;
    
    printf("\n--- Add New Student ---\n");
    
    printf("Username: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0'; // Remove newline
        strncpy(student->username, buffer, sizeof(student->username) - 1);
    }
    
    printf("Full Name: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(student->name, buffer, sizeof(student->name) - 1);
    }
    
    printf("Email: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(student->email, buffer, sizeof(student->email) - 1);
    }
    
    student->active = 1; // Default active
    
    return 0;
}

int get_faculty_details(struct Faculty *faculty) {
    char buffer[256];
    int n;
    
    printf("\n--- Add New Faculty ---\n");
    
    printf("Username: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(faculty->username, buffer, sizeof(faculty->username) - 1);
    }
    
    printf("Full Name: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(faculty->name, buffer, sizeof(faculty->name) - 1);
    }
    
    printf("Email: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(faculty->email, buffer, sizeof(faculty->email) - 1);
    }
    
    printf("Department: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(faculty->department, buffer, sizeof(faculty->department) - 1);
    }
    
    return 0;
}

int get_course_details(struct Course *course) {
    char buffer[256];
    int n;
    
    printf("\n--- Add New Course ---\n");
    
    printf("Course Code: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(course->course_code, buffer, sizeof(course->course_code) - 1);
    }
    
    printf("Course Name: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        strncpy(course->course_name, buffer, sizeof(course->course_name) - 1);
    }
    
    printf("Maximum Seats: ");
    fflush(stdout);
    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n - 1] = '\0';
        course->max_seats = atoi(buffer);
    }
    
    course->enrolled_count = 0; // Default
    
    return 0;
}

void display_error(const char *message) {
    fprintf(stderr, "\n[ERROR] %s\n", message);
    printf("Press Enter to continue...");
    fflush(stdout);
    char buffer[10];
    read(STDIN_FILENO, buffer, sizeof(buffer));
}

void display_success(const char *message) {
    printf("\n[SUCCESS] %s\n", message);
    printf("Press Enter to continue...");
    fflush(stdout);
    char buffer[10];
    read(STDIN_FILENO, buffer, sizeof(buffer));
}

void display_info(const char *message) {
    printf("\n[INFO] %s\n", message);
}

int confirm_action(const char *message) {
    char buffer[10];
    printf("%s (y/n): ", message);
    fflush(stdout);
    int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        return (buffer[0] == 'y' || buffer[0] == 'Y');
    }
    return 0;
}