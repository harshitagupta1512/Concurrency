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

#define CYAN "\x1B[36m"
#define CLEAR "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAG "\x1B[35m"
#define MAX 100
typedef long long int ll;

typedef struct zone
{
    int capacity;
} zone;
zone zone_A, zone_H, zone_N; //home, away and neutral

int num_groups;
int num_people;

typedef struct spectator
{
    int id; //index in the match_specs array
    char name[MAX];
    int group_num;
    char fan_type; // H/A/N
    int entry_time;
    int patience_time;
    int patience_num_goals;
    pthread_t thread;
    int is_seated;
    pthread_mutex_t spec_mutex;
    pthread_cond_t get_seat;
    pthread_cond_t opponent_goals;

} spec;

int X; //Spectating time, same accross all pepole

typedef struct goal_chance
{
    int id;
    char team_str[2];
    char team;
    int time_elapsed;
    float prob;
    pthread_t thread;

} goal_chance;

int total_goal_chances;

pthread_mutex_t goals_mutex;
int goals_home;
int goals_away;

spec **match_specs;
int *specs_per_group;
goal_chance *G;

int simulation;

sem_t zoneA_sem;
sem_t zoneH_sem;
sem_t zoneN_sem;

pthread_t goal_signal;

#endif