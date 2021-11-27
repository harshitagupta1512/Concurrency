## An alternate course allocation portal

**Context** - Sanchirat and Pratrey, students of the college, have been asked to use their skills to design an unconventional course registration system for the college, where a student can take trial classes of a course and can withdraw and opt for a different course if he/she does not like the course. Different labs in the college have been asked to provide students who can act as course TA mentors temporarily and take the trial tutorials.

### How to run the program
1. Run the command `gcc -pthread portal.c` 
2. Run `./a.out`
3. Input the description of the courses, labs, TAs and students. 

### Variables

1. `struct ta` - describes a TA.
   ```c
    int lab_id;
    int ta_id;       // [0, LabList[lab_id],num_TAs]
    int num_taships; // [0, LabList[lab_id].max_taships]
    int is_busy; //is the ta is taking a tutorial for a course
    pthread_mutex_t ta_mutex;
   ````
2. `struct lab` - describes a lab
    ```c
    int id;
    char name[MAX];
    int num_TAs;
    int max_taships;
    int total_tut; //total number of tutorials that have taken place by the TAs of this lab
    ta *TAs;
    pthread_mutex_t lab_mutex;
    ````
3. `struct course` - describes a course
    ```c
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
    ````
4. `struct student` - describes a student
    ```cpp
    int id;
    float calibre;
    int c_id[3];
    int registration_time;
    int curr_cid; //the id of the course the student is currently waiting on to get slots
    int is_allocated_curr_slot;
    int is_attending_curr_tut;
    int selected_course; //1 if the student decided on a course
    pthread_t thread_id;          //thread
    pthread_mutex_t student_lock; //lock

    pthread_cond_t get_slot; //after getting slots student threads wait in tut-happening until the course thread finds a TA and signals all the waiting student threads (use broadcast)

    ````

### Routines
1. In the `main thread`, we take the input, create a thread for each `course` and each `student` and only when each of the student thread join the main thread back, the simulation ends.

2. In `get_input`, we make sure to `initialize` all the `locks` and `condition variables`.

2. In the `student thread`, 