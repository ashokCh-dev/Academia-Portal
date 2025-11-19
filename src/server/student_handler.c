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
#include "auth.h"
#include "file_ops.h"

// Function declarations
int handle_enroll_course(char *request, char *response, const char *username);
int handle_unenroll_course(char *request, char *response, const char *username);
int handle_view_enrolled_courses(char *request, char *response, const char *username);

int get_student_id_by_username(const char *username);
int get_enrolled_courses(int student_id, char *buffer, size_t buffer_size);

// Main student handler function
// In student_handler.c
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
#include "auth.h"  // Include auth.h for handle_password_change
#include "file_ops.h"

// NO handle_password_change implementation here - it's in auth.c

// Main student handler function
int handle_student_request(int client_socket, char *request, const char *username) {
    char response[1024];
    char command[256];
    char params[768];
    
    // Parse the request
    if (sscanf(request, "%[^:]:%[^\n]", command, params) != 2) {
        strcpy(command, request); // Handle commands without parameters
        params[0] = '\0';
    }
    
    // Handle different student commands
    if (strcmp(command, "ENROLL_COURSE") == 0) {
        handle_enroll_course(params, response, username);
        write(client_socket, response, strlen(response));
        return 0;
    } else if (strcmp(command, "UNENROLL_COURSE") == 0) {
        handle_unenroll_course(params, response, username);
        write(client_socket, response, strlen(response));
        return 0;
    } else if (strcmp(command, "VIEW_ENROLLED_COURSES") == 0) {
        handle_view_enrolled_courses(params, response, username);
        write(client_socket, response, strlen(response));
        return 0;
    } else if (strcmp(command, "CHANGE_PASSWORD") == 0) {
        // Call the handle_password_change from auth.c
        return handle_password_change(client_socket, request, username);
    } else {
        strcpy(response, "ERROR:Unknown student command");
        write(client_socket, response, strlen(response));
        return -1;
    }
}

// Rest of your student_handler.c functions remain the same...

int handle_enroll_course(char *params, char *response, const char *username) {
    int course_id;
    int student_id;
    struct Course course;
    struct Enrollment enrollment;
    
    // Parse course ID
    if (sscanf(params, "%d", &course_id) != 1) {
        strcpy(response, "ERROR:Invalid course ID");
        return -1;
    }
    
    // Get student ID
    student_id = get_student_id_by_username(username);
    if (student_id < 0) {
        strcpy(response, "ERROR:Student not found");
        return -1;
    }
    
    // Check if student is active
    struct Student student;
    if (read_student_by_id(student_id, &student) < 0) {
        strcpy(response, "ERROR:Cannot read student data");
        return -1;
    }
    
    if (!student.active) {
        strcpy(response, "ERROR:Student account is inactive");
        return -1;
    }
    
    // Check if already enrolled
    if (check_enrollment_exists(student_id, course_id)) {
        strcpy(response, "ERROR:Already enrolled in this course");
        return -1;
    }
    
    // Read course details
    if (read_course_by_id(course_id, &course) < 0) {
        strcpy(response, "ERROR:Course not found");
        return -1;
    }
    
    // Check if course has available seats
    if (course.enrolled_count >= course.max_seats) {
        strcpy(response, "ERROR:Course is full");
        return -1;
    }
    
    // Create enrollment
    enrollment.enrollment_id = get_next_enrollment_id();
    enrollment.student_id = student_id;
    enrollment.course_id = course_id;
    enrollment.enrollment_date = time(NULL);
    
    // Add enrollment
    if (add_enrollment(&enrollment) < 0) {
        strcpy(response, "ERROR:Failed to enroll");
        return -1;
    }
    
    // Update course enrollment count
    course.enrolled_count++;
    if (update_course(&course) < 0) {
        // Rollback enrollment if course update fails
        remove_enrollment(student_id, course_id);
        strcpy(response, "ERROR:Failed to update course");
        return -1;
    }
    
    sprintf(response, "SUCCESS:Enrolled in course %s (%s)", 
            course.course_code, course.course_name);
    return 0;
}

int handle_unenroll_course(char *params, char *response, const char *username) {
    int course_id;
    int student_id;
    struct Course course;
    
    // Parse course ID
    if (sscanf(params, "%d", &course_id) != 1) {
        strcpy(response, "ERROR:Invalid course ID");
        return -1;
    }
    
    // Get student ID
    student_id = get_student_id_by_username(username);
    if (student_id < 0) {
        strcpy(response, "ERROR:Student not found");
        return -1;
    }
    
    // Check if enrolled
    if (!check_enrollment_exists(student_id, course_id)) {
        strcpy(response, "ERROR:Not enrolled in this course");
        return -1;
    }
    
    // Read course details
    if (read_course_by_id(course_id, &course) < 0) {
        strcpy(response, "ERROR:Course not found");
        return -1;
    }
    
    // Remove enrollment
    if (remove_enrollment(student_id, course_id) < 0) {
        strcpy(response, "ERROR:Failed to unenroll");
        return -1;
    }
    
    // Update course enrollment count
    course.enrolled_count--;
    if (update_course(&course) < 0) {
        strcpy(response, "WARNING:Unenrolled but failed to update course count");
        return 0;
    }
    
    sprintf(response, "SUCCESS:Unenrolled from course %s", course.course_code);
    return 0;
}

int handle_view_enrolled_courses(char *params, char *response, const char *username) {
    int student_id;
    char courses_info[1024];
    
    // Get student ID
    student_id = get_student_id_by_username(username);
    if (student_id < 0) {
        strcpy(response, "ERROR:Student not found");
        return -1;
    }
    
    // Get enrolled courses
    if (get_enrolled_courses(student_id, courses_info, sizeof(courses_info)) < 0) {
        strcpy(response, "No courses enrolled");
        return 0;
    }
    
    strcpy(response, courses_info);
    return 0;
}



int get_student_id_by_username(const char *username) {
    struct Student student;
    
    if (read_student_by_username(username, &student) < 0) {
        return -1;
    }
    
    return student.id;
}

int get_enrolled_courses(int student_id, char *buffer, size_t buffer_size) {
    struct Enrollment enrollment;
    struct Course course;
    int fd_enrollment;
    char line[256];
    int count = 0;
    
    buffer[0] = '\0';
    sprintf(buffer, "Enrolled Courses:\n");
    strcat(buffer, "=================\n");
    
    fd_enrollment = open(ENROLLMENT_FILE, O_RDONLY);
    if (fd_enrollment < 0) {
        return -1;
    }
    
    // Apply read lock
    flock(fd_enrollment, LOCK_SH);
    
    // Find all enrollments for the student
    while (read(fd_enrollment, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (enrollment.student_id == student_id) {
            // Get course details
            if (read_course_by_id(enrollment.course_id, &course) == 0) {
                sprintf(line, "Course ID: %d | Code: %s | Name: %s | Seats: %d/%d\n",
                        course.course_id, course.course_code, course.course_name,
                        course.enrolled_count, course.max_seats);
                
                if (strlen(buffer) + strlen(line) < buffer_size) {
                    strcat(buffer, line);
                    count++;
                }
            }
        }
    }
    
    flock(fd_enrollment, LOCK_UN);
    close(fd_enrollment);
    
    if (count == 0) {
        return -1;
    }
    
    sprintf(line, "\nTotal courses enrolled: %d\n", count);
    strcat(buffer, line);
    
    return 0;
}