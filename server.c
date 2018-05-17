/* This program run on Mac OS*/
/* Unlike Windows system, it use unix file descriptor to create socket*/
/* which is an int value*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <signal.h>
#include <pthread.h>

// max connections amount
#define MAX_CONN 3

// client msg format, send between clients
typedef struct {
    char name[10];
    char msg[1024];
} Msg;

// server maintain client's connections table
typedef struct {
    int client_sock;
    char ip[16];
    int port;
} Connection;

// init connection table
Connection * con_table[MAX_CONN];

// server socker descriptor
int server_sock;

// total alive connections
int con_count = 0;

// handle SIGINT keyboard interrupt
void sig_handler(int signo)
{
  if (signo == SIGINT) {
      printf("- Close socket -");
      close(server_sock);
      exit(1);
  }
}

// hash client sock for table index
int hash(int client_sock){
    return client_sock % MAX_CONN;
}

// set connection table's elements to NULL
void init_table(void){
    for(int i = 0; i < MAX_CONN; i++){
        con_table[i] = NULL;
    }
}

// insert table
void insert(int client_sock, char *ip, int port){
    int hash_index = hash(client_sock);

    Connection *this = (Connection *) malloc(sizeof(Connection));

    this -> client_sock = client_sock;
    strcpy(this->ip, ip);
    this -> port = port;

    while(con_table[hash_index] != NULL){
        hash_index++;
        hash_index %= MAX_CONN;
    }

    con_table[hash_index] = this;
}

// search element in table
Connection * search(int client_sock){
    int search_count = 0, hash_index = hash(client_sock);

    while(con_table[hash_index] != NULL){
        if(con_table[hash_index] -> client_sock == client_sock){
            return con_table[hash_index];
        }

        hash_index++;
        hash_index %= MAX_CONN;

        search_count++;
        if(search_count >= MAX_CONN){
            return NULL;
        }
    }

    return NULL;
}

// set disconnect connection to NULL
void delete(int client_sock){
    int hash_index = hash(client_sock);
    int search_count = 0;

    while(search_count < MAX_CONN){
        if(con_table[hash_index] != NULL && con_table[hash_index] -> client_sock == client_sock){
            printf("-----index: %d\n-----", hash_index);
            free(con_table[hash_index]);
            con_table[hash_index] = NULL;
            break;
        }

        search_count++;

        hash_index++;
        hash_index %= MAX_CONN;
    }
}

// show all connections in table
void show(void){
    puts("-----Connections Pool-----");
    for(int i = 0; i < MAX_CONN; i++){
        if(con_table[i] != NULL){
            printf("%s:%d\n", con_table[i] -> ip, con_table[i] -> port);
        } else {
            puts("NULL");
        }
    }
}


// serve every connection
void accept_client(void *arg){
    //accept connection from an incoming client
    int client_sock, this_sock, read_size, c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;
    pthread_t new_thread, myid = pthread_self();
    Msg msg;

    //*********connections limited***********//
    while(1){
        // printf("-----con_count: %d-----\n", con_count);
        if(con_count < MAX_CONN) break;
        sleep(1);
    }
    //***************end*********************//

    //*************accept*********************//
    client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0){
        perror("accept");
        pthread_exit(NULL);
    }
    //**************end***********************//

    // create thread to accept if alive connections is less than MAX_CONN
    if(con_count < MAX_CONN - 1){
        pthread_create(&new_thread, NULL, accept_client, (void *) "*");
    }

    //*********client info*********************//
    char client_ip[16];
    int client_port;

    strcpy(client_ip, inet_ntoa(client.sin_addr));
    client_port = ntohs(client.sin_port);

    printf("- Accept connection from %s:%d -\n", client_ip, client_port);
    //********end******************************//

    //****store client info to client_table****//
    insert(client_sock, client_ip, client_port);
    show();
    con_count++;
    //******end********************************//

    //****Receive a message from client********//
    while((read_size = recv(client_sock, &msg, sizeof(Msg), 0)) > 0){
        // send to every one in client_table
        for(int i = 0; i < MAX_CONN; i++){
            if(con_table[i] != NULL && (con_table[i] -> client_sock) != client_sock){
                write(con_table[i] -> client_sock, &msg, sizeof(msg));
            }
        }
    }

    //*****Handle Client Disconnect*********//
    if(read_size == 0){
        printf("- %s:%d disconnect -\n", client_ip, client_port);

        // clear this connection
        delete(client_sock);
        show();

        // time to server another connection when decrease from MAX_CONN
        if(con_count == MAX_CONN){
            pthread_create(&new_thread, NULL, accept_client, (void *) "*");
        }
        con_count--;

        // terminate thread
        pthread_exit(NULL);
    }
    //**************end*********************//

    //********recv error********************//
    else if(read_size == -1){
        perror("recv");

        // clear this connection
        delete(client_sock);
        show();
        con_count--;

        // terminate thread
        pthread_exit(NULL);
    }
    //**********end****************************//

    return;
}

int main(int argc , char *argv[])
{
    struct sockaddr_in server;
    pthread_t new_thread;

    // init connection table
    init_table();

    //Create socket
    server_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sock == -1)
    {
        perror("socker");
        exit(1);
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5000);

    //Bind
    if(bind(server_sock,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }

    //Listen
    listen(server_sock , 3);

    // catch signal
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("\ncan't catch SIGINT\n");
    }

    //Accept incoming connection
    puts("- Waiting for incoming connections... -");

    //accept connection from an incoming client
    pthread_create(&new_thread, NULL, accept_client, (void *) "*");

    while(1){
        // sleep 200 milesecond
        usleep(200*1000);
    }

    return 0;
}
