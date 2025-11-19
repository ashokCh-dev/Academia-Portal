#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <time.h>

struct Student {
    int id;
    char username[50];
    char name[100];
    char email[100];
    int active;
};

struct Faculty {
    int id;
    char username[50];
    char name[100];
    char email[100];
    char department[50];
};

struct Course {
    int course_id;
    char course_code[20];
    char course_name[100];
    int faculty_id;
    int max_seats;
    int enrolled_count;
};

struct Enrollment {
    int enrollment_id;
    int student_id;
    int course_id;
    time_t enrollment_date;
};

struct Credentials {
    char username[50];
    char password_hash[65];
    char role[10];
};

#endif // STRUCTURES_H