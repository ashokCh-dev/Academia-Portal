#ifndef UI_H
#define UI_H

#include "../common/structures.h"

// Menu display functions
void display_admin_menu();
void display_student_menu();
void display_faculty_menu();

// Input functions
int get_student_details(struct Student *student);
int get_faculty_details(struct Faculty *faculty);
int get_course_details(struct Course *course);

// Utility functions
void clear_screen();
void display_error(const char *message);
void display_success(const char *message);
void display_info(const char *message);
int confirm_action(const char *message);

#endif // UI_H