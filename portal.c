#include "header_portal.h"
void allocate_slots(int cid)
{
    //there are a particular number of students threads waiting on slots_available
    //there is a upper limit on the number of slots we allocate this time = CourseList[cid].curr_total_slots

    int aslots = 0;
    do
    {
        //printf(MAG "HERE %d\n", CourseList[cid].curr_total_slots);

        int i = 0;
        while (aslots < CourseList[cid].curr_total_slots && i < num_students)
        {
            //printf(RED "HERE %d %d %d\n", StudentList[i].id, i, aslots);
            if (StudentList[i].curr_cid == cid && StudentList[i].is_allocated_curr_slot == 0)
            {
                //student is waiting on this course and has not been allocated any slot yet
                //printf(MAG "HERE %d\n", StudentList[i].id);
                aslots++;

                //pthread_mutex_lock(&StudentList[i].student_lock);
                StudentList[i].is_allocated_curr_slot = 1;
                //pthread_mutex_unlock(&StudentList[i].student_lock);

                int rc = pthread_cond_signal(&StudentList[i].get_slot);
                assert(rc == 0);
            }

            i++;
        }

    } while (aslots == 0);

    pthread_mutex_lock(&CourseList[cid].course_lock);
    CourseList[cid].curr_slots_allocated = aslots;
    pthread_mutex_unlock(&CourseList[cid].course_lock);

    printf(GREEN "Course %s has been allocated %d slots\n", CourseList[cid].name, aslots);
}

void tutorial(int cid)
{
    //students have been allocated slots and are waiting in tut_happening condition variable

    for (int i = 0; i < num_students; i++)
    {
        if (StudentList[i].curr_cid == cid && StudentList[i].is_allocated_curr_slot == 1 && StudentList[i].is_attending_curr_tut == 0)
        {
            pthread_mutex_lock(&StudentList[i].student_lock);
            StudentList[i].is_attending_curr_tut = 1;
            pthread_mutex_unlock(&StudentList[i].student_lock);
        }
    }
    int rc = pthread_cond_broadcast(&CourseList[cid].tut_happening);
    assert(rc == 0);

    //the TA is free now

    int lab_id = CourseList[cid].curr_lab_id;
    int ta_id = CourseList[cid].curr_ta_id;
    printf(YELLOW "Tutorial has started for Course %s with %d seats filled out of %d\n", CourseList[cid].name, CourseList[cid].curr_slots_allocated, CourseList[cid].curr_total_slots);
    sleep(2);
    printf(YELLOW "TA %d from lab %s has completed the tutorial for course %s\n", CourseList[cid].curr_ta_id, LabList[CourseList[cid].curr_lab_id].name, CourseList[cid].name);

    pthread_mutex_lock(&CourseList[cid].course_lock);
    CourseList[cid].curr_slots_allocated = -1;
    CourseList[cid].curr_total_slots = -1;
    CourseList[cid].curr_ta_id = -1;
    CourseList[cid].curr_lab_id = -1;
    pthread_mutex_unlock(&CourseList[cid].course_lock);

    pthread_mutex_lock(&LabList[lab_id].TAs[ta_id].ta_mutex);
    LabList[lab_id].TAs[ta_id].is_busy = 0;
    pthread_mutex_unlock(&LabList[lab_id].TAs[ta_id].ta_mutex);
}

void remove_course(int cid)
{
    pthread_mutex_lock(&CourseList[cid].course_lock);
    CourseList[cid].is_removed = 1;
    pthread_mutex_unlock(&CourseList[cid].course_lock);

    for (int i = 0; i < num_students; i++)
    {
        if (StudentList[i].curr_cid == cid)
        {
            //StudentList[i].curr_cid = -1;
            int rc = pthread_cond_signal(&StudentList[i].get_slot);
            assert(rc == 0);
        }
    }
}

int find_ta(int cid) // find a ta for the course given by cid
{
    int flag = -1;

    for (int i = 0; i < CourseList[cid].num_labs; i++)
    {
        int lid = CourseList[cid].lab_id[i];
        pthread_mutex_lock(&LabList[lid].lab_mutex);
        if (LabList[lid].total_tut == LabList[lid].num_TAs * LabList[lid].max_taships)
        {
            pthread_mutex_unlock(&LabList[lid].lab_mutex);
            continue;
        }
        pthread_mutex_unlock(&LabList[lid].lab_mutex);

        int max_taships = LabList[lid].max_taships; // constant parameter

        for (int j = 0; j < LabList[lid].num_TAs; j++)
        {
            pthread_mutex_lock(&LabList[lid].TAs[j].ta_mutex);
            if (LabList[lid].TAs[j].num_taships < max_taships)
            {
                //the ta has not exhausted his/her maximum number of taships
                flag = 0;
                if (LabList[lid].TAs[j].is_busy == 0)
                {
                    //this ta is available
                    //select this TA

                    CourseList[cid].curr_ta_id = j;
                    CourseList[cid].curr_lab_id = lid;

                    pthread_mutex_lock(&LabList[lid].lab_mutex);
                    LabList[lid].total_tut++;
                    pthread_mutex_unlock(&LabList[lid].lab_mutex);

                    LabList[lid].TAs[j].num_taships++;
                    LabList[lid].TAs[j].is_busy = 1;

                    pthread_mutex_unlock(&LabList[lid].TAs[j].ta_mutex);
                    printf(YELLOW "TA %d from lab %s has been allocated to course %s for his %d TA ship\n", j, LabList[lid].name, CourseList[cid].name, LabList[lid].TAs[j].num_taships);
                    return 1;
                }
            }
            pthread_mutex_unlock(&LabList[lid].TAs[j].ta_mutex);
        }
    }

    return flag;
}

void *course_thread(void *arg)
{
    int cid = *(int *)arg;
    printf(BLUE "Course %s is added on the portal\n", CourseList[cid].name);
    int flag;
    // = 1, if we can find a ta
    // = 0 if all tas have "not" finished their TAships yet but still we cannot assign any ta(all are busy) taking other tutorials
    // = -1 if all tas have exhausted their maximum number of TAships
    // flag_ta = value returned by find_ta()

    while (simulation)
    {
        //while the simulation is running, if there is no TA assigned to the course currently, we traverse through
        //all labs to find a eligible TA
        flag = 0;

        if (CourseList[cid].curr_ta_id == -1)
        {
            do
            {
                flag = find_ta(cid);

            } while (flag == 0);

            if (flag == -1)
            {
                remove_course(cid);
                break;
            }
            //flag_ta = 1 which implies we have found a TA for the course

            int slots = rand() % (CourseList[cid].max_slots + 1) + 1;
            CourseList[cid].curr_total_slots = slots;

            allocate_slots(cid); // returns the number of students attending the tutorial
            tutorial(cid);       //allocate_slots() and tutorial() will majorly involve signaling the corresponding condition variable;
        }
    }

    if (CourseList[cid].is_removed == 1)
        printf(RED "Course %s doesn’t have any TA’s eligible and is removed from course offerings\n", CourseList[cid].name);

    return NULL;
}

void *student_thread(void *arg)
{
    int sid = *(int *)arg;

    sleep(StudentList[sid].registration_time);

    printf(BLUE "Student %d has filled in preferences for course registration\n", sid);

    for (int i = 0; i < 3; i++)
    {
        int cid = StudentList[sid].c_id[i];

        pthread_mutex_lock(&StudentList[sid].student_lock);
        //printf("%d Here_1\n", StudentList[sid].id);
        StudentList[sid].curr_cid = cid;
        StudentList[sid].is_allocated_curr_slot = 0;
        StudentList[sid].is_attending_curr_tut = 0;
        pthread_mutex_unlock(&StudentList[sid].student_lock);

        pthread_mutex_lock(&StudentList[sid].student_lock);
        //printf("%d Here_2\n", StudentList[sid].id);

        while (CourseList[cid].is_removed == 0 && StudentList[sid].is_allocated_curr_slot == 0)
        {
            pthread_cond_wait(&StudentList[sid].get_slot, &StudentList[sid].student_lock);
        }
        pthread_mutex_unlock(&StudentList[sid].student_lock);
        //printf("%d Here_2.1\n", StudentList[sid].id);

        if (CourseList[cid].is_removed == 1)
        {
            if (i == 0 || i == 1)
            {
                printf(CYAN "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n", sid, CourseList[StudentList[sid].c_id[i]].name, i + 1, CourseList[StudentList[sid].c_id[i + 1]].name, i + 2);
                continue;
            }
            else
                break;
        }
        else if (StudentList[sid].is_allocated_curr_slot == 1)
        {
            //printf("%d Here_3\n", StudentList[sid].id);

            printf(YELLOW "Student %d has been allocated a seat in course %s\n", sid, CourseList[cid].name);

            pthread_mutex_lock(&StudentList[sid].student_lock);
            //printf("%d Here_4\n", StudentList[sid].id);
            while (StudentList[sid].is_attending_curr_tut == 0)
            {
                pthread_cond_wait(&CourseList[cid].tut_happening, &StudentList[sid].student_lock);
            }
            pthread_mutex_unlock(&StudentList[sid].student_lock);

            printf(YELLOW "Student %d has attended the tutorial for course %s\n", sid, CourseList[cid].name);

            float x = StudentList[sid].calibre * CourseList[StudentList[sid].c_id[i]].interest * 100;
            int is_selected = (rand() % 101) < x;

            if (is_selected)
            {
                //printf("%d Here_5\n", StudentList[sid].id);
                pthread_mutex_lock(&StudentList[sid].student_lock);
                StudentList[sid].selected_course = 1;
                StudentList[sid].curr_cid = -1;
                StudentList[sid].is_allocated_curr_slot = 0;
                StudentList[sid].is_attending_curr_tut = 0;
                pthread_mutex_unlock(&StudentList[sid].student_lock);

                printf(GREEN "Student %d has selected course %s permanently\n", sid, CourseList[cid].name);
                break;
            }
            else
                printf(MAG "Student %d has withdrawn from course %s\n", sid, CourseList[cid].name);
        }
        if (i == 0 || i == 1)
            printf(CYAN "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n", sid, CourseList[StudentList[sid].c_id[i]].name, i + 1, CourseList[StudentList[sid].c_id[i + 1]].name, i + 2);
    }

    if (StudentList[sid].selected_course == 0)
        printf(RED "Student %d could not get any of his preferred courses\n", sid);

    pthread_mutex_lock(&StudentList[sid].student_lock);
    StudentList[sid].curr_cid = -1;
    StudentList[sid].is_allocated_curr_slot = -1;
    StudentList[sid].is_attending_curr_tut = -1;
    pthread_mutex_unlock(&StudentList[sid].student_lock);

    printf(RED "Student %d is exiting the simulation\n", sid);
    // fclose(StudentList[sid].sf);
    return NULL;
}

void get_input()
{
    scanf("%d %d %d", &num_students, &num_labs, &num_courses);

    CourseList = (course *)malloc(num_courses * sizeof(course));

    for (int i = 0; i < num_courses; i++)
    {
        scanf("%s %f %d %d", CourseList[i].name, &CourseList[i].interest, &CourseList[i].max_slots, &CourseList[i].num_labs);

        CourseList[i].lab_id = (int *)malloc(sizeof(int) * CourseList[i].num_labs);

        for (int j = 0; j < CourseList[i].num_labs; j++)
            scanf("%d", &CourseList[i].lab_id[j]);

        CourseList[i].id = i;
        CourseList[i].is_removed = 0;
        CourseList[i].curr_slots_allocated = -1;
        CourseList[i].curr_ta_id = -1;
        CourseList[i].curr_total_slots = -1;
        CourseList[i].curr_lab_id = -1;

        // int rc = pthread_mutex_init(&CourseList[i].smutex, NULL);
        // assert(rc == 0);
        // rc = pthread_mutex_init(&CourseList[i].tmutex, NULL);
        // assert(rc == 0);

        int rc = pthread_cond_init(&CourseList[i].tut_happening, NULL);
        assert(rc == 0);

        rc = pthread_mutex_init(&CourseList[i].course_lock, NULL);
        assert(rc == 0);

        // rc = pthread_cond_init(&CourseList[i].slots_available, NULL);
        // assert(rc == 0);
    }

    StudentList = (student *)malloc(num_students * sizeof(student));

    for (int i = 0; i < num_students; i++)
    {
        scanf("%f %d %d %d %d", &StudentList[i].calibre, &StudentList[i].c_id[0], &StudentList[i].c_id[1], &StudentList[i].c_id[2], &StudentList[i].registration_time);
        StudentList[i].id = i;
        StudentList[i].selected_course = 0;
        StudentList[i].curr_cid = -1;
        StudentList[i].is_allocated_curr_slot = -1;
        StudentList[i].is_attending_curr_tut = -1;

        // int rc = pthread_mutex_init(&StudentList[i].student_lock_slots, NULL);
        // assert(rc == 0);
        // rc = pthread_mutex_init(&StudentList[i].student_lock_tuts, NULL);
        // assert(rc == 0);

        int rc = pthread_mutex_init(&StudentList[i].student_lock, NULL);
        assert(rc == 0);
        rc = pthread_cond_init(&StudentList[i].get_slot, NULL);
        assert(rc == 0);
    }

    LabList = (lab *)malloc(num_labs * sizeof(lab));

    for (int i = 0; i < num_labs; i++)
    {
        scanf("%s %d %d", LabList[i].name, &LabList[i].num_TAs, &LabList[i].max_taships);
        LabList[i].id = i;
        LabList[i].total_tut = 0;
        LabList[i].TAs = (ta *)malloc(LabList[i].num_TAs * sizeof(ta));

        int rc = pthread_mutex_init(&LabList[i].lab_mutex, NULL);
        assert(rc == 0);

        for (int j = 0; j < LabList[i].num_TAs; j++)
        {
            LabList[i].TAs[j].ta_id = j;
            LabList[i].TAs[j].lab_id = i;
            LabList[i].TAs[j].num_taships = 0;
            LabList[i].TAs[j].is_busy = 0;
            //int rc = pthread_mutex_init(&LabList[i].TAs[j].is_available, NULL);
            //assert(rc == 0);

            rc = pthread_mutex_init(&LabList[i].TAs[j].ta_mutex, NULL);
            assert(rc == 0);
        }
    }
}

int main(void)
{
    srand(time(0));
    get_input();

    printf(CLEAR "Simulation begins\n");
    simulation = 1;

    for (int i = 0; i < num_courses; i++)
        pthread_create(&(CourseList[i].thread_id), NULL, course_thread, (void *)(&(CourseList[i].id)));

    for (int i = 0; i < num_students; i++)
        pthread_create(&(StudentList[i].thread_id), NULL, student_thread, (void *)(&(StudentList[i].id)));

    for (int i = 0; i < num_students; i++)
        pthread_join(StudentList[i].thread_id, NULL);

    simulation = 0;
    sleep(2); //wait for all course threads to end
    printf(CLEAR "Simulation ends\n");
}