#ifndef HEADER_FILE
#define HEADER_FILE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#define CYAN "\x1B[36m"
#define CLEAR "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAG "\x1B[35m"
#define MAX 100

typedef struct ta
{
    int lab_id;
    int ta_id;       // [0, LabList[lab_id],num_TAs]
    int num_taships; // [0, LabList[lab_id].max_taships]

    int is_busy; //the ta is taking a tutorial for a course
    //pthread_mutex_t is_available;
    pthread_mutex_t ta_mutex;
    //we need a lock for each ta as it is a global variable that can be accessed by multiple course threads
} ta;

typedef struct lab
{
    int id;
    char name[MAX];
    int num_TAs;
    int max_taships;

    int total_tut; //total number of tutorials that have taken place by the TAs of this lab
    ta *TAs;

    //int is_exhausted; //have all TAs of the lab have exhausted their maximum taships
    pthread_mutex_t lab_mutex;

} lab;

lab *LabList;

typedef struct course
{
    int id;
    char name[MAX];
    float interest; //[0, 1]
    int max_slots;
    int num_labs;
    int *lab_id;

    int is_removed;
    pthread_t thread_id;

    pthread_cond_t tut_happening; //after getting slots student threads wait in tut-happening until the course thread finds a TA
    //and signals all the waiting student threads (use broadcast)
    int curr_ta_id;  //id of the TA currently assigned to the course
    int curr_lab_id; //lab Id of the TA currently assigned

    int curr_total_slots; //total number slots decided using rand
    int curr_slots_allocated;
    pthread_mutex_t course_lock;

    // FILE *cf;

} course;

course *CourseList;
int num_students, num_courses, num_labs;

typedef struct student
{
    //FILE *sf;
    int id;
    float calibre;
    int c_id[3];
    int registration_time;

    int curr_cid; //the id of the course the student is currently waiting on to get slots

    int is_allocated_curr_slot;
    int is_attending_curr_tut;
    int selected_course;          //1 if the student decided on a course
    pthread_t thread_id;          //thread
    pthread_mutex_t student_lock; //lock

    //pthread_mutex_t student_lock_slots;
    //pthread_mutex_t student_lock_tuts;

    pthread_cond_t get_slot; //after getting slots student threads wait in tut-happening until the course thread finds a TA
    //and signals all the waiting student threads (use broadcast)

} student;

student *StudentList;
int simulation; //1 after simultion begins, 0 after it ends, flag to end all course threads

#endif