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
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <assert.h>
#include <queue>
#include <vector>
#include <tuple>

using namespace std;

#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;
const LL MOD = 1000000007;
#define part cout << "-----------------------------------" << endl;
#define pb push_back
#define debug(x) cout << #x << " : " << x << endl

#define SERVER_PORT 8001
const LL buff_sz = 1048576;

#define MAX 50
#define CMAX 100

typedef struct client_req
{
    int id;
    int t; //time in sec after which the request to connect to the server is made
    char command[CMAX];
    pthread_t client_thread_id;
    pthread_mutex_t client_mutex;
    // a mutex accquired whenever we are accessing/modifying
    //data members of this stucture that can be changed by different threads simultaneously
    int client_socket_fd;
    int req_type;
    //0 - insert
    //1 - delete
    //2 - update
    //3 - concat
    //4 - fetch
    //-1 - invalid

} client_req;

client_req *req_list;
pthread_mutex_t output; //lock accquired by the client thread who wishes to print on the terminal