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

// Function declarations
int handle_add_course(char *request, char *response, const char *username);
int handle_remove_course(char *request, char *response, const char *username);
int handle_view_enrollments(char *request, char *response);
int handle_view_my_courses(char *response, const char *username);
int get_faculty_id_by_username(const char *username);
int get_next_course_id();
int count_course_enrollments(int course_id);
int is_course_owner(int course_id, const char *username);


// Updated main faculty handler function
int handle_faculty_request(int client_socket, char *request, const char *username) {
    char response[1024];
    char command[256];
    char params[768];
    
    // Initialize response buffer
    memset(response, 0, sizeof(response));
    
    // Parse the request
    if (sscanf(request, "%[^:]:%[^\n]", command, params) != 2) {
        strcpy(command, request); // Handle commands without parameters
        params[0] = '\0';
    }
    
    // Handle different faculty commands
    if (strcmp(command, "ADD_COURSE") == 0) {
        handle_add_course(params, response, username);
    } else if (strcmp(command, "REMOVE_COURSE") == 0) {
        handle_remove_course(params, response, username);
    } else if (strcmp(command, "VIEW_ENROLLMENTS") == 0) {
        handle_view_enrollments(params, response);
    } else if (strcmp(command, "VIEW_MY_COURSES") == 0) {
        handle_view_my_courses(response, username);
    } else if (strcmp(command, "CHANGE_PASSWORD") == 0) {
        handle_password_change(client_socket, request, username);
    } else {
        strcpy(response, "ERROR:Unknown faculty command");
    }
    
    // Send response
    write(client_socket, response, strlen(response));
    return 0;
}


// Check if course code already exists in the course file
int course_code_exists(const char *course_code) {
    struct Course course;
    int fd;
    int exists = 0;
    
    // Open course file
    fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        // If the file doesn't exist yet, course certainly doesn't exist
        if (errno == ENOENT) {
            return 0;
        }
        return -1; // Other error opening file
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for the course code
    while (read(fd, &course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (strcmp(course.course_code, course_code) == 0) {
            // Course code already exists
            exists = 1;
            break;
        }
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    return exists;
}

int handle_add_course(char *params, char *response, const char *username) {
    struct Course course;
    char course_code[20], course_name[100];
    int max_seats;
    int fd;
    int faculty_id;
    
    // Parse parameters
    if (sscanf(params, "%[^:]:%[^:]:%d", course_code, course_name, &max_seats) != 3) {
        strcpy(response, "ERROR:Invalid parameters for ADD_COURSE");
        return -1;
    }
    
    // Check if course code already exists
    int code_check = course_code_exists(course_code);
    if (code_check < 0) {
        sprintf(response, "ERROR:Failed to check course code existence");
        return -1;
    }
    if (code_check > 0) {
        sprintf(response, "ERROR:Course code '%s' already exists", course_code);
        return -1;
    }
    
    // Get faculty ID from username
    faculty_id = get_faculty_id_by_username(username);
    if (faculty_id < 0) {
        strcpy(response, "ERROR:Faculty not found");
        return -1;
    }
    
    // Fill course structure
    course.course_id = get_next_course_id();
    strncpy(course.course_code, course_code, sizeof(course.course_code) - 1);
    strncpy(course.course_name, course_name, sizeof(course.course_name) - 1);
    course.faculty_id = faculty_id;
    course.max_seats = max_seats;
    course.enrolled_count = 0;
    
    // Open file with write lock
    fd = open(COURSE_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        sprintf(response, "ERROR:Cannot open course file: %s", strerror(errno));
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock course file: %s", strerror(errno));
        return -1;
    }
    
    // Write course record
    if (write(fd, &course, sizeof(struct Course)) != sizeof(struct Course)) {
        flock(fd, LOCK_UN);
        close(fd);
        sprintf(response, "ERROR:Failed to write course record: %s", strerror(errno));
        return -1;
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    sprintf(response, "SUCCESS:Course added with ID %d", course.course_id);
    return 0;
}

int handle_remove_course(char *params, char *response, const char *username) {
    int course_id;
    struct Course course;
    int fd_read, fd_write;
    char temp_file[] = "data/courses.tmp";
    int found = 0;
    
    // Parse parameters
    if (sscanf(params, "%d", &course_id) != 1) {
        strcpy(response, "ERROR:Invalid course ID");
        return -1;
    }
    
    // Check if faculty owns the course
    if (!is_course_owner(course_id, username)) {
        strcpy(response, "ERROR:You can only remove courses you created");
        return -1;
    }
    
    // Open files
    fd_read = open(COURSE_FILE, O_RDONLY);
    fd_write = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd_read < 0 || fd_write < 0) {
        if (fd_read >= 0) close(fd_read);
        if (fd_write >= 0) close(fd_write);
        sprintf(response, "ERROR:Cannot open course files: %s", strerror(errno));
        return -1;
    }
    
    // Apply locks
    flock(fd_read, LOCK_EX);
    flock(fd_write, LOCK_EX);
    
    // Copy all courses except the one to remove
    while (read(fd_read, &course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (course.course_id != course_id) {
            write(fd_write, &course, sizeof(struct Course));
        } else {
            found = 1;
        }
    }
    
    // Release locks and close files
    flock(fd_read, LOCK_UN);
    flock(fd_write, LOCK_UN);
    close(fd_read);
    close(fd_write);
    
    if (found) {
        // Replace original file with temp file
        if (rename(temp_file, COURSE_FILE) == 0) {
            sprintf(response, "SUCCESS:Course removed successfully");
        } else {
            sprintf(response, "ERROR:Failed to update course file");
            unlink(temp_file);
        }
    } else {
        sprintf(response, "ERROR:Course not found");
        unlink(temp_file);
    }
    
    return 0;
}

int handle_view_enrollments(char *params, char *response) {
    int course_id;
    struct Enrollment enrollment;
    struct Student student;
    int fd_enrollment, fd_student;
    char enrollments_info[1024] = "";
    char line[256];
    int count = 0;
    
    // Parse parameters
    if (sscanf(params, "%d", &course_id) != 1) {
        strcpy(response, "ERROR:Invalid course ID");
        return -1;
    }
    
    // Open enrollment file
    fd_enrollment = open(ENROLLMENT_FILE, O_RDONLY);
    if (fd_enrollment < 0) {
        sprintf(response, "No enrollments found for course %d", course_id);
        return 0;
    }
    
    // Apply read lock
    flock(fd_enrollment, LOCK_SH);
    
    // Find enrollments for the course
    while (read(fd_enrollment, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (enrollment.course_id == course_id) {
            // Get student details
            fd_student = open(STUDENT_FILE, O_RDONLY);
            if (fd_student >= 0) {
                flock(fd_student, LOCK_SH);
                while (read(fd_student, &student, sizeof(struct Student)) == sizeof(struct Student)) {
                    if (student.id == enrollment.student_id) {
                        sprintf(line, "Student ID: %d, Name: %s, Email: %s\n", 
                                student.id, student.name, student.email);
                        strcat(enrollments_info, line);
                        count++;
                        break;
                    }
                }
                flock(fd_student, LOCK_UN);
                close(fd_student);
            }
        }
    }
    
    flock(fd_enrollment, LOCK_UN);
    close(fd_enrollment);
    
    if (count > 0) {
        sprintf(response, "Total enrollments: %d\n%s", count, enrollments_info);
    } else {
        sprintf(response, "No enrollments found for course %d", course_id);
    }
    
    return 0;
}

int get_faculty_id_by_username(const char *username) {
    struct Faculty faculty;
    int fd;
    int faculty_id = -1;
    
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    flock(fd, LOCK_SH);
    
    while (read(fd, &faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (strcmp(faculty.username, username) == 0) {
            faculty_id = faculty.id;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return faculty_id;
}

int get_next_course_id() {
    struct Course course;
    int fd, max_id = 0;
    
    fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        return 1; // First course
    }
    
    flock(fd, LOCK_SH);
    
    while (read(fd, &course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (course.course_id > max_id) {
            max_id = course.course_id;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return max_id + 1;
}

int is_course_owner(int course_id, const char *username) {
    struct Course course;
    int fd;
    int faculty_id;
    int is_owner = 0;
    
    // Get faculty ID from username
    faculty_id = get_faculty_id_by_username(username);
    if (faculty_id < 0) {
        return 0;
    }
    
    // Open course file
    fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    flock(fd, LOCK_SH);
    
    // Find the course and check ownership
    while (read(fd, &course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (course.course_id == course_id) {
            if (course.faculty_id == faculty_id) {
                is_owner = 1;
            }
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return is_owner;
}

int count_course_enrollments(int course_id) {
    struct Enrollment enrollment;
    int fd;
    int count = 0;
    
    fd = open(ENROLLMENT_FILE, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    flock(fd, LOCK_SH);
    
    while (read(fd, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (enrollment.course_id == course_id) {
            count++;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return count;
}

// Function to handle viewing courses offered by the logged-in faculty
int handle_view_my_courses(char *response, const char *username) {
    struct Course course;
    int fd;
    int faculty_id;
    char courses_info[1024] = "";
    char line[256];
    int count = 0;
    
    // Get faculty ID from username
    faculty_id = get_faculty_id_by_username(username);
    if (faculty_id < 0) {
        strcpy(response, "ERROR:Faculty not found");
        return -1;
    }
    
    // Open course file
    fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        // If file doesn't exist or can't be opened
        if (errno == ENOENT) {
            strcpy(response, "You haven't offered any courses yet");
        } else {
            sprintf(response, "ERROR:Cannot open course file: %s", strerror(errno));
        }
        return 0;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        sprintf(response, "ERROR:Cannot lock course file: %s", strerror(errno));
        return -1;
    }
    
    // Find courses for this faculty
    while (read(fd, &course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (course.faculty_id == faculty_id) {
            // Get enrollment count
            int enrolled = count_course_enrollments(course.course_id);
            
            sprintf(line, "ID: %d, Code: %s, Name: %s, Seats: %d/%d\n", 
                    course.course_id, course.course_code, course.course_name, 
                    enrolled, course.max_seats);
            strcat(courses_info, line);
            count++;
        }
    }
    
    // Release lock and close file
    flock(fd, LOCK_UN);
    close(fd);
    
    if (count > 0) {
        sprintf(response, "Your courses (%d):\n%s", count, courses_info);
    } else {
        strcpy(response, "You haven't offered any courses yet");
    }
    
    return 0;
}