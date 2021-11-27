#include "client.h"

int get_socket_fd(struct sockaddr_in *ptr)
{
    struct sockaddr_in server_obj = *ptr;

    // socket() creates an endpoint for communication and returns a file
    //descriptor that refers to that endpoint.  The file descriptor
    //returned by a successful call will be the lowest-numbered file
    //descriptor not currently open for the process.

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        perror("Error in socket creation for CLIENT");
        return -1;
    }

    int port_num = SERVER_PORT;

    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num); //convert to big-endian order
    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
    {
        perror("Problem in connecting to the server");
        return -1;
    }
    return socket_fd;
}

void *client_thread(void *arg)
{
    int cid = *(int *)arg;
    // cout << "Thread started for" << req_list[cid].client_thread_id << endl;
    struct sockaddr_in server_obj;
    req_list[cid].client_socket_fd = get_socket_fd(&server_obj); //create a socket and connect it to the server and return the fd

    sleep(req_list[cid].t);
    //write your request to the socket - to be sent to the server
    //accquire the lock so that only the client can access the socket for now

    pthread_mutex_lock(&req_list[cid].client_mutex);
    int rc = write(req_list[cid].client_socket_fd, req_list[cid].command, strlen(req_list[cid].command));
    if (rc < 0)
    {
        cerr << "Failed to SEND DATA on socket.\n";
        pthread_mutex_unlock(&req_list[cid].client_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&req_list[cid].client_mutex);

    //read the server's response from the socket
    // cout << "Sent command to the server successfully " << req_list[cid].client_thread_id << endl;

    pthread_mutex_lock(&req_list[cid].client_mutex);
    char buf[MAX];
    rc = read(req_list[cid].client_socket_fd, buf, MAX); //read returns the number of bytes read;
    if (rc < 0)
    {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        pthread_mutex_unlock(&req_list[cid].client_mutex);
        return NULL;
    }
    buf[rc] = '\0';

    pthread_mutex_unlock(&req_list[cid].client_mutex);
    // print the result on the terminal - one client at a time

    pthread_mutex_lock(&output);
    cout << req_list[cid].id << " : " << buf << endl;
    pthread_mutex_unlock(&output);
    return NULL;
}

int main(int argc, char *argv[])
{
    int m; //total number of user requests made throughout the simulation
    scanf("%d", &m);

    req_list = (client_req *)malloc(m * sizeof(client_req));

    int rc = pthread_mutex_init(&output, NULL); //initialise the lock
    assert(rc == 0);

    for (int i = 0; i < m; i++)
    {
        //get input
        req_list[i].id = i;
        scanf("%d ", &req_list[i].t);
        cin.getline(req_list[i].command, CMAX);
        rc = pthread_mutex_init(&req_list[i].client_mutex, NULL); //initialise the lock
        assert(rc == 0);
    }

    for (int i = 0; i < m; i++)
    {
        pthread_create(&req_list[i].client_thread_id, NULL, client_thread, (void *)&req_list[i].id);
    }

    for (int i = 0; i < m; i++)
    {
        pthread_join(req_list[i].client_thread_id, NULL);
    }

    //begin_process();
    return 1; //successful simulation
}