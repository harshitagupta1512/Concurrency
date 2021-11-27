#include "header_clasico.h"
#include <stdio.h>

void *goals_signal_thread()
{
    //while the simultion is running
    while (simulation == 1)
    {
        for (int i = 0; i < num_people; i++)
        {
            char fan = match_specs[i]->fan_type;

            if (match_specs[i]->is_seated > 0) // possible values for is_seated = 0, 1, 2, 3, -1
            {
                switch (fan)
                {
                case 'H':
                    pthread_mutex_lock(&goals_mutex);
                    if (goals_away >= match_specs[i]->patience_num_goals)
                        pthread_cond_signal(&match_specs[i]->opponent_goals);
                    pthread_mutex_unlock(&goals_mutex);
                    break;
                case 'A':
                    pthread_mutex_lock(&goals_mutex);
                    if (goals_home >= match_specs[i]->patience_num_goals)
                        pthread_cond_signal(&match_specs[i]->opponent_goals);
                    pthread_mutex_unlock(&goals_mutex);
                    break;
                case 'N':
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void *zone_home_thread(void *arg)
{
    int sid = *(int *)arg;
    //printf("Here for person %s\n", match_specs[sid]->name);

    //get a seat in home zone
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        perror("Error : ");
        return NULL;
    }
    ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;
    int s;

    while ((s = sem_timedwait(&zoneH_sem, &ts)) == -1 && errno == EINTR)
        continue;

    if (s == -1)
    {
        if (errno == ETIMEDOUT)
        {
            //printf("sem_timedwait() timed out\n");
            return NULL;
        }
        else
        {

            //perror("sem_timedwait");
            return NULL;
        }
    }
    else
    {
        //printf("sem_timedwait() succeeded\n");
        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        if (match_specs[sid]->is_seated == 0)
        {
            match_specs[sid]->is_seated = 1;
            pthread_cond_signal(&match_specs[sid]->get_seat);
        }
        else
            sem_post(&zoneH_sem);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);
    }
}

void *zone_away_thread(void *arg)
{
    int sid = *(int *)arg;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        perror("Error : ");
        return NULL;
    }
    ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;
    int s;

    while ((s = sem_timedwait(&zoneA_sem, &ts)) == -1 && errno == EINTR)
        continue;

    if (s == -1)
    {
        if (errno == ETIMEDOUT)
        {
            //printf("sem_timedwait() timed out\n");
            return NULL;
        }
        else
        {

            //perror("sem_timedwait");
            return NULL;
        }
    }
    else
    {
        //printf("sem_timedwait() succeeded\n");

        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        if (match_specs[sid]->is_seated == 0)
        {
            match_specs[sid]->is_seated = 2;
            pthread_cond_signal(&match_specs[sid]->get_seat);
        }
        else
            sem_post(&zoneA_sem);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);
    }
}

void *zone_neutral_thread(void *arg)
{
    int sid = *(int *)arg;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        perror("Error : ");
        return NULL;
    }
    ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;
    int s;
    while ((s = sem_timedwait(&zoneN_sem, &ts)) == -1 && errno == EINTR)
        continue;

    if (s == -1)
    {
        if (errno == ETIMEDOUT)
        {
            //printf("sem_timedwait() timed out\n");
            return NULL;
        }
        else
        {

            //perror("sem_timedwait");
            return NULL;
        }
    }
    else
    {
        //printf("sem_timedwait() succeeded\n");
        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        if (match_specs[sid]->is_seated == 0)
        {
            match_specs[sid]->is_seated = 3;
            pthread_cond_signal(&match_specs[sid]->get_seat);
        }
        else
            sem_post(&zoneN_sem);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);
    }
}

int get_seat(int sid, char fan_type)
{
    switch (fan_type)
    {
    case 'A':
    {
        pthread_t away_thread;
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            perror("Error : ");
            return -1;
        }
        ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;

        pthread_create(&(away_thread), NULL, zone_away_thread, (void *)(&(sid)));
        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        int rc = pthread_cond_timedwait(&match_specs[sid]->get_seat, &match_specs[sid]->spec_mutex, &ts);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);
        break;
    }
    case 'H':
    {
        pthread_t neutral_thread, home_thread;
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            perror("Error : ");
            return -1;
        }
        ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;

        pthread_create(&(home_thread), NULL, zone_home_thread, (void *)(&(sid)));
        pthread_create(&(neutral_thread), NULL, zone_neutral_thread, (void *)(&(sid)));

        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        pthread_cond_timedwait(&match_specs[sid]->get_seat, &match_specs[sid]->spec_mutex, &ts);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);

        break;
    }

    case 'N':
    {

        pthread_t away_thread, neutral_thread, home_thread;

        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            perror("Error : ");
            return -1;
        }
        ts.tv_sec = ts.tv_sec + match_specs[sid]->patience_time;

        pthread_create(&(neutral_thread), NULL, zone_neutral_thread, (void *)(&(sid)));
        pthread_create(&(home_thread), NULL, zone_home_thread, (void *)(&(sid)));
        pthread_create(&(away_thread), NULL, zone_away_thread, (void *)(&(sid)));

        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        pthread_cond_timedwait(&match_specs[sid]->get_seat, &match_specs[sid]->spec_mutex, &ts);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);
        break;
    }
    default:
        printf("Invalid fan type for %s\n", match_specs[sid]->name);
        return -1;
    }
    return -1;
}

int watch_match(int sid)
{
    //printf(RED "A = %d H = %d\n", goals_away, goals_home);
    char fan = match_specs[sid]->fan_type;

    //time_t start_watch;
    //start_watch = time(NULL);

    //printf(MAG "%s started watching the match at %ld\n", match_specs[sid]->name, start_watch);

    switch (fan)
    {
    case 'H':
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            perror("Error : ");
            return -1;
        }
        ts.tv_sec = ts.tv_sec + X;

        int rc;
        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        rc = pthread_cond_timedwait(&match_specs[sid]->opponent_goals, &match_specs[sid]->spec_mutex, &ts);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);

        if (goals_away >= match_specs[sid]->patience_num_goals)
            printf(BLUE "%s is leaving due to the bad performance of his team\n", match_specs[sid]->name);
        else
            printf(BLUE "%s watched the match for %d seconds and is leaving\n", match_specs[sid]->name, X);

        break;
    }
    case 'A':
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            perror("Error : ");
            return -1;
        }
        ts.tv_sec = ts.tv_sec + X;
        int rc;

        pthread_mutex_lock(&match_specs[sid]->spec_mutex);
        rc = pthread_cond_timedwait(&match_specs[sid]->opponent_goals, &match_specs[sid]->spec_mutex, &ts);
        pthread_mutex_unlock(&match_specs[sid]->spec_mutex);

        if (goals_home >= match_specs[sid]->patience_num_goals)
            printf(BLUE "%s is leaving due to the bad performance of his team\n", match_specs[sid]->name);
        else
            printf(BLUE "%s watched the match for %d seconds and is leaving\n", match_specs[sid]->name, X);
        break;
    }
    case 'N':
        sleep(X);
        printf(BLUE "%s watched the match for %d seconds and is leaving\n", match_specs[sid]->name, X);
        break;

    default:
        break;
    }
}

void *spectator_thread(void *arg)
{
    int sid = *(int *)(arg);

    match_specs[sid]->is_seated = 0;

    sleep(match_specs[sid]->entry_time); //sleep till entry time
    printf(YELLOW "%s has reached the stadium.\n", match_specs[sid]->name);

    char fan = match_specs[sid]->fan_type;
    switch (fan)
    {
    case 'H':
        if (goals_away >= match_specs[sid]->patience_num_goals)
        {

            printf(BLUE "%s is leaving due to the bad performance of his team\n", match_specs[sid]->name);
            return NULL;
        }
        break;
    case 'A':
        if (goals_home >= match_specs[sid]->patience_num_goals)
        {
            printf(BLUE "%s is leaving due to the bad performance of his team\n", match_specs[sid]->name);
            return NULL;
        }
        break;

    default:
        break;
    }

    get_seat(sid, match_specs[sid]->fan_type);

    //rc = 0 if person could not get the seat
    //rc = 1 - Home zone
    //rc = 2 - Away zone
    //rc = 3 - Neutral zone

    int rc = match_specs[sid]->is_seated;

    if (rc == 0)
    {
        printf(RED "%s couldn’t get a seat.\n", match_specs[sid]->name);
        return NULL;
    }
    else if (rc == 1)
        printf(GREEN "%s has got a seat in zone H”\n", match_specs[sid]->name);
    else if (rc == 2)
        printf(GREEN "%s has got a seat in zone A”\n", match_specs[sid]->name);
    else if (rc == 3)
        printf(GREEN "%s has got a seat in zone N”\n", match_specs[sid]->name);

    watch_match(sid);

    if (rc == 1)
        sem_post(&zoneH_sem);
    else if (rc == 2)
        sem_post(&zoneA_sem);
    else if (rc == 3)
        sem_post(&zoneN_sem);

    match_specs[sid]->is_seated = -1;

    return NULL;
}

void *goal_chance_thread(void *arg)
{
    int goal_id = *(int *)(arg);
    sleep(G[goal_id].time_elapsed);

    float x = G[goal_id].prob * 100.0;
    int is_goal = (rand() % 101) < x;

    //printf(RED "Prob * 100 = %f, is_goal = %d\n", x, is_goal);

    if (is_goal == 1 && G[goal_id].team == 'A')
    {
        pthread_mutex_lock(&goals_mutex);
        goals_away++;
        printf(CYAN "Team A has scored their %d goal\n", goals_away);
        pthread_mutex_unlock(&goals_mutex);
    }

    else if (is_goal == 0 && G[goal_id].team == 'A')
    {
        printf(CYAN "Team A missed the chance to score their %d goal\n", goals_away + 1);
    }

    else if (is_goal == 1 && G[goal_id].team == 'H')
    {
        pthread_mutex_lock(&goals_mutex);
        goals_home++;
        printf(CYAN "Team H has scored their %d goal\n", goals_home);
        pthread_mutex_unlock(&goals_mutex);
    }
    else
    {
        printf(CYAN "Team H missed the chance to score their %d goal\n", goals_home + 1);
    }
}

void get_input()
{
    //Capacity of each zone
    scanf("%d %d %d", &zone_H.capacity, &zone_A.capacity, &zone_N.capacity);
    // no one wants to watch the match for more than ‘X’ units (SPECTATING TIME).
    //This value is fixed across all people.

    scanf("%d", &X);
    scanf("%d", &num_groups);
    specs_per_group = (int *)malloc(num_groups * sizeof(int));

    match_specs = (spec **)malloc(MAX * sizeof(spec *));

    for (int i = 0; i < MAX; i++)
        match_specs[i] = NULL;

    for (int i = 0; i < num_groups; i++)
    {
        int x = 0;
        scanf("%d", &x);
        specs_per_group[i] = x;

        for (int j = 0; j < x; j++)
        {
            int itr = num_people + j;
            match_specs[itr] = (spec *)malloc(sizeof(spec));
            match_specs[itr]->group_num = i + 1;
            scanf("%s %c %d %d %d", match_specs[itr]->name, &match_specs[itr]->fan_type, &match_specs[itr]->entry_time, &match_specs[itr]->patience_time, &match_specs[itr]->patience_num_goals);
            match_specs[itr]->id = itr;
        }
        num_people = num_people + x;
    }

    scanf("%d", &total_goal_chances);

    G = (goal_chance *)malloc(total_goal_chances * sizeof(goal_chance));

    for (int i = 0; i < total_goal_chances; i++)
    {
        G[i].id = i;
        scanf("%s %d %f", G[i].team_str, &G[i].time_elapsed, &G[i].prob);

        if (strcmp(G[i].team_str, "H") == 0)
            G[i].team = 'H';
        else if (strcmp(G[i].team_str, "A") == 0)
            G[i].team = 'A';
        else if (strcmp(G[i].team_str, "N") == 0)
            G[i].team = 'N';
    }
}

int main()
{
    srand(time(0));
    num_people = 0;
    goals_home = 0;
    goals_away = 0;

    get_input();

    // printf("Number of goal chances = %d\n", total_goal_chances);

    // for (int i = 0; i < total_goal_chances; i++)
    //     printf("%c %d %f\n", G[i].team, G[i].time_elapsed, G[i].prob);

    for (int i = 0; i < num_people; i++)
    {
        pthread_cond_init(&match_specs[i]->get_seat, NULL);
        pthread_cond_init(&match_specs[i]->opponent_goals, NULL);
        pthread_mutex_init(&match_specs[i]->spec_mutex, NULL);
    }

    pthread_mutex_init(&goals_mutex, NULL);

    sem_init(&zoneA_sem, 0, zone_A.capacity);
    sem_init(&zoneH_sem, 0, zone_H.capacity);
    sem_init(&zoneN_sem, 0, zone_N.capacity);

    simulation = 0;

    printf("Begin Simulation\n");
    simulation = 1;

    pthread_create(&goal_signal, NULL, goals_signal_thread, NULL);

    for (int i = 0; i < total_goal_chances; i++)
        pthread_create(&(G[i].thread), NULL, goal_chance_thread, (void *)(&(G[i].id)));

    for (int i = 0; i < num_people; i++)
        pthread_create(&(match_specs[i]->thread), NULL, spectator_thread, (void *)(&(match_specs[i]->id)));

    for (int i = 0; i < num_people; i++)
        pthread_join(match_specs[i]->thread, NULL);

    for (int i = 0; i < total_goal_chances; i++)
        pthread_join(G[i].thread, NULL);

    simulation = 0;
    sleep(1);
    printf(CLEAR "End Simulation\n");
}