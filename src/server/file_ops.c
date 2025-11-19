#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include "../common/structures.h"
#include "../common/constants.h"

// File paths


// Function implementations

int read_student_by_id(int id, struct Student *student) {
    int fd;
    int found = 0;
    
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for student
    while (read(fd, student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (student->id == id) {
            found = 1;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

int read_student_by_username(const char *username, struct Student *student) {
    int fd;
    int found = 0;
    
    fd = open(STUDENT_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for student
    while (read(fd, student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (strcmp(student->username, username) == 0) {
            found = 1;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

int read_faculty_by_id(int id, struct Faculty *faculty) {
    int fd;
    int found = 0;
    
    fd = open(FACULTY_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for faculty
    while (read(fd, faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (faculty->id == id) {
            found = 1;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

int read_course_by_id(int id, struct Course *course) {
    int fd;
    int found = 0;
    
    fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return -1;
    }
    
    // Search for course
    while (read(fd, course, sizeof(struct Course)) == sizeof(struct Course)) {
        if (course->course_id == id) {
            found = 1;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

int update_course(struct Course *course) {
    int fd;
    struct Course temp;
    off_t offset = 0;
    int found = 0;
    
    fd = open(COURSE_FILE, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    
    // Find and update course
    while (read(fd, &temp, sizeof(struct Course)) == sizeof(struct Course)) {
        if (temp.course_id == course->course_id) {
            // Seek back to the record position
            lseek(fd, offset, SEEK_SET);
            
            // Write updated record
            if (write(fd, course, sizeof(struct Course)) != sizeof(struct Course)) {
                flock(fd, LOCK_UN);
                close(fd);
                return -1;
            }
            found = 1;
            break;
        }
        offset += sizeof(struct Course);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}

int add_enrollment(struct Enrollment *enrollment) {
    int fd;
    
    fd = open(ENROLLMENT_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    
    // Write enrollment record
    if (write(fd, enrollment, sizeof(struct Enrollment)) != sizeof(struct Enrollment)) {
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return 0;
}

int remove_enrollment(int student_id, int course_id) {
    int fd_read, fd_write;
    struct Enrollment enrollment;
    char temp_file[] = "data/enrollments.tmp";
    int found = 0;
    
    fd_read = open(ENROLLMENT_FILE, O_RDONLY);
    fd_write = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd_read < 0 || fd_write < 0) {
        if (fd_read >= 0) close(fd_read);
        if (fd_write >= 0) close(fd_write);
        return -1;
    }
    
    // Apply locks
    flock(fd_read, LOCK_EX);
    flock(fd_write, LOCK_EX);
    
    // Copy all enrollments except the one to remove
    while (read(fd_read, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (!(enrollment.student_id == student_id && enrollment.course_id == course_id)) {
            write(fd_write, &enrollment, sizeof(struct Enrollment));
        } else {
            found = 1;
        }
    }
    
    flock(fd_read, LOCK_UN);
    flock(fd_write, LOCK_UN);
    close(fd_read);
    close(fd_write);
    
    if (found) {
        rename(temp_file, ENROLLMENT_FILE);
        return 0;
    } else {
        unlink(temp_file);
        return -1;
    }
}

int check_enrollment_exists(int student_id, int course_id) {
    int fd;
    struct Enrollment enrollment;
    int exists = 0;
    
    fd = open(ENROLLMENT_FILE, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    // Apply read lock
    if (flock(fd, LOCK_SH) < 0) {
        close(fd);
        return 0;
    }
    
    // Check if enrollment exists
    while (read(fd, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (enrollment.student_id == student_id && enrollment.course_id == course_id) {
            exists = 1;
            break;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return exists;
}

int get_next_enrollment_id() {
    struct Enrollment enrollment;
    int fd, max_id = 0;
    
    fd = open(ENROLLMENT_FILE, O_RDONLY);
    if (fd < 0) {
        return 1; // First enrollment
    }
    
    flock(fd, LOCK_SH);
    
    while (read(fd, &enrollment, sizeof(struct Enrollment)) == sizeof(struct Enrollment)) {
        if (enrollment.enrollment_id > max_id) {
            max_id = enrollment.enrollment_id;
        }
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return max_id + 1;
}

int update_credentials(const char *username, const char *new_password_hash) {
    int fd;
    struct Credentials cred;
    off_t offset = 0;
    int found = 0;
    
    fd = open(CREDENTIALS_FILE, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    // Apply write lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    
    // Find and update credentials
    while (read(fd, &cred, sizeof(struct Credentials)) == sizeof(struct Credentials)) {
        if (strcmp(cred.username, username) == 0) {
            // Update password
            strncpy(cred.password_hash, new_password_hash, sizeof(cred.password_hash) - 1);
            
            // Seek back to the record position
            lseek(fd, offset, SEEK_SET);
            
            // Write updated record
            if (write(fd, &cred, sizeof(struct Credentials)) != sizeof(struct Credentials)) {
                flock(fd, LOCK_UN);
                close(fd);
                return -1;
            }
            found = 1;
            break;
        }
        offset += sizeof(struct Credentials);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
    
    return found ? 0 : -1;
}