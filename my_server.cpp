#include "server.h"

int send_string_on_socket(int fd, const string &s, int worker_thread_id)
{
    // debug(s.length());
    string str = to_string(worker_thread_id);
    str = str + " : ";
    str = str + s;

    int bytes_sent = write(fd, str.c_str(), str.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

void printQueue(queue<int> q)
{
    //printing content of queue
    while (!q.empty())
    {
        cout << " " << q.front();
        q.pop();
    }
    cout << endl;
}

int get_arguments(char command[], char arguments[4][MAX])
{
    int itr = 0;
    int i = 0;

    while (command[i] != '\0')
    {
        int j = 0;

        while (command[i] != '\0' && command[i] != ' ')
        {
            arguments[itr][j++] = command[i++];
        }

        arguments[itr][j] = '\0';

        while (command[i] == ' ')
        {
            i++;
        }

        if (command[i] == '\0')
            break;

        itr++;
    }

    // for (int i = 0; i <= itr; i++)
    //     cout << "-" << arguments[i] << "-" << endl;

    itr++;
    return itr;
}

void handle_command(char *command, int client_socket_fd)
{
    int wid = gettid();
    char arguments[4][MAX];
    int num_args = get_arguments(command, arguments);

    if (strcmp(arguments[0], "insert") == 0)
    {
        if (num_args != 3)
        {
            cout << "Invalid Command" << endl;
            return;
        }

        int key = atoi(arguments[1]);
        char value[MAX];
        strcpy(value, arguments[2]);
        pthread_mutex_lock(&dict[key].dict_entry_mutex);

        if (dict[key].is_present == 1)
        {
            cout << "Key already exists" << endl;
            send_string_on_socket(client_socket_fd, "Key already exists", wid);
        }
        else
        {
            //insert this new key
            dict[key].is_present = 1;
            dict[key].key = key;
            strcpy(dict[key].value, value);
            cout << "Insertion successful" << endl;
            send_string_on_socket(client_socket_fd, "Insertion successful", wid);
        }
        pthread_mutex_unlock(&dict[key].dict_entry_mutex);
    }
    else if (strcmp(arguments[0], "delete") == 0)
    {
        if (num_args != 2)
        {
            cout << "Invalid Command" << endl;
            return;
        }

        int key = atoi(arguments[1]);

        pthread_mutex_lock(&dict[key].dict_entry_mutex);

        if (dict[key].is_present == 0)
        {
            cout << "No such key exists" << endl;
            send_string_on_socket(client_socket_fd, "No such key exists", wid);
        }
        else
        {
            dict[key].is_present = 0;
            strcpy(dict[key].value, "\0");
            cout << "Deletion successful" << endl;
            send_string_on_socket(client_socket_fd, "Deletion successful", wid);
        }

        pthread_mutex_unlock(&dict[key].dict_entry_mutex);
    }
    else if (strcmp(arguments[0], "update") == 0)
    {
        if (num_args != 3)
        {
            cout << "Invalid Command" << endl;
            return;
        }

        int key = atoi(arguments[1]);
        char value[MAX];
        strcpy(value, arguments[2]);

        pthread_mutex_lock(&dict[key].dict_entry_mutex);
        if (dict[key].is_present == 0)
        {
            cout << "Updation successful" << endl;
            send_string_on_socket(client_socket_fd, "Updation successful", wid);
        }
        else
        {
            dict[key].key = key;
            strcpy(dict[key].value, value);
            cout << "No such key exists" << endl;
            send_string_on_socket(client_socket_fd, "No such key exists", wid);
        }
        pthread_mutex_unlock(&dict[key].dict_entry_mutex);
    }
    else if (strcmp(arguments[0], "concat") == 0)
    {
        if (num_args != 3)
        {
            cout << "Invalid Command" << endl;
            return;
        }

        int key1 = atoi(arguments[1]);
        int key2 = atoi(arguments[2]);

        pthread_mutex_lock(&dict[key1].dict_entry_mutex);
        pthread_mutex_lock(&dict[key2].dict_entry_mutex);

        if (dict[key2].is_present == 0 || dict[key1].is_present == 0)
        {
            cout << "Concat failed as at least one of the keys does not exist" << endl;
            send_string_on_socket(client_socket_fd, "Concat failed as at least one of the keys does not exist", wid);
        }
        else
        {
            char temp[MAX];
            strcpy(temp, "");
            strcat(temp, dict[key1].value);
            strcat(dict[key1].value, dict[key2].value);
            strcat(dict[key2].value, temp);

            cout << "Concatenation successful" << endl;
            send_string_on_socket(client_socket_fd, dict[key2].value, wid);
        }

        pthread_mutex_unlock(&dict[key2].dict_entry_mutex);
        pthread_mutex_unlock(&dict[key1].dict_entry_mutex);
    }
    else if (strcmp(arguments[0], "fetch") == 0)
    {
        if (num_args != 2)
        {
            cout << "Invalid Command" << endl;
            return;
        }

        int key = atoi(arguments[1]);

        pthread_mutex_lock(&dict[key].dict_entry_mutex);
        if (dict[key].is_present == 0)
        {
            cout << "Key does not exist" << endl;
            send_string_on_socket(client_socket_fd, "Key does not exist", wid);
        }
        else
        {
            cout << "Fetch successful" << endl;
            send_string_on_socket(client_socket_fd, dict[key].value, wid);
        }
        pthread_mutex_unlock(&dict[key].dict_entry_mutex);
    }
    else
    {
        cout << "Invalid Command" << endl;
        return;
    }
}

void handle_connection(int client_socket_fd)
{
    char command[MAX];
    int cmd_size = MAX;
    int bytes_read = read(client_socket_fd, &command[0], cmd_size - 1);

    if (bytes_read <= 0)
    {
        cerr << "Failed to read data from socket. \n";
        printf("Server could not read msg sent from client\n");
        goto close_client_socket_ceremony;
    }
    command[bytes_read] = '\0';

    handle_command(command, client_socket_fd);

close_client_socket_ceremony:
    close(client_socket_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
}

void *worker_thread(void *arg)
{
    int wid = *(int *)arg;
    while (1)
    {
        pthread_mutex_lock(&queue_lock);

        //sem_wait(&service_client_sem);

        while (client_q.empty())
        {
            pthread_cond_wait(&service_client, &queue_lock);
        }

        int client_sock_fd = client_q.front();

        client_q.pop();

        // cout << "Worker thread " << worker_list[wid].worker_thread_id << "assigned for " << client_sock_fd;
        pthread_mutex_unlock(&queue_lock);

        handle_connection(client_sock_fd);
    }
}

int create_server_socket()
{
    // the server requires 2 sockets
    // one to listen to the client requests - wel_socket_fd
    // one to actually communicate with the clients - client_socket_fd
    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;
    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact 
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // initialisation
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        return -1;
    }

    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj)); //initialise the structures - no garbage values
    port_number = PORT_ARG;                               // port to which the server listen to for incoming client requests
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); //process specifies port
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */
    // bind the socket with the structure

    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        return -1;
    }

    /* listen for incoming connection requests */
    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd
        During the three-way handshake, the client process knocks on the welcoming door
        of the server process. When the server “hears” the knocking, it creates a new door—
        more precisely, a new socket that is dedicated to that particular client. 
        */
        //accept is a blocking call
        printf("Waiting for a new client to request for a connection\n");

        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        // system calledits the structure client_addr_obj

        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            return -1;
        }

        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        pthread_mutex_lock(&queue_lock);
        client_q.push(client_socket_fd);
        pthread_mutex_unlock(&queue_lock);

        pthread_cond_signal(&service_client);
        //sem_post(&service_client_sem);

        //printQueue(client_q);
    }
    close(wel_socket_fd);
    return 1;
}

int main(int argc, char *argv[])
{
    n = atoi(argv[1]);

    worker_list = (worker *)malloc(n * sizeof(worker));

    pthread_mutex_init(&queue_lock, NULL);

    pthread_cond_init(&service_client, NULL);
    //sem_init(&service_client_sem, 0, 0); //initialise to 0 as 0 client requests

    for (int i = 0; i < MAX; i++)
    {
        dict[i].key = i;
        pthread_mutex_init(&dict[i].dict_entry_mutex, NULL);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_create(&worker_list[i].worker_thread_id, NULL, worker_thread, (void *)&worker_list[i].id);
    }

    int rc = create_server_socket();

    return 1;
}