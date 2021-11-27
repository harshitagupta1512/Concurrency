#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>
#include <tuple>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <queue>
using namespace std;

#define MAX 50

#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

#define MAX_CLIENTS 100
#define PORT_ARG 8001

const int initial_msg_len = 256;

const LL buff_sz = 1048576;

typedef struct worker
{
    int id;
    pthread_t worker_thread_id;
    //pthread_mutex_t w_mutex;

} worker;

worker *worker_list;
pthread_cond_t service_client;
//sem_t service_client_sem;

pthread_mutex_t queue_lock;
queue<int> client_q;

int n;

typedef struct dict_entry
{
    int key;
    char value[MAX];
    pthread_mutex_t dict_entry_mutex;
    int is_present;
} dict_entry;

dict_entry dict[MAX];
