/* This program run on Mac OS*/
/* Unlike Windows system, it use unix file descriptor to create socket*/
/* which is an int value*/

#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

// Msg format send between clients and server
typedef struct {
    char name[10];
    char msg[1024];
} Msg;

int server_sock;

void sig_handler(int signo)
{
  if (signo == SIGINT) {
      puts("-----Bye Bye-----");
      close(server_sock);
      exit(0);
  }
}

/*A thread to continuesly receive stdin
and send Msg to server*/
void get_msg(void *arg){
    Msg msg;
    int s_len;

    printf("Enter your name : ");
    scanf("%s" , msg.name);

    puts("-----Let's Talk-----");

    while(1){
        fgets(msg.msg, 1024, stdin);

        s_len = strlen(msg.msg);
        msg.msg[s_len - 1] = '\0';

        if(s_len > 1){
            send(server_sock, &msg, sizeof(Msg), 0);
        }
    }
}

int main(int argc , char *argv[]){
    struct sockaddr_in server;
    pthread_t new_thread;
    Msg rcv_msg;

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("\ncan't catch SIGINT\n");
    }

    //Create socket
    server_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sock == -1)
    {
        perror("socker");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);

    //Connect to remote server
    if (connect(server_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect");
        return 1;
    }

    puts("-----Chatroom Connected-----");

    //create thread to receive from stdin
    pthread_create(&new_thread, NULL, get_msg, (void *)"*");

    // keep receiving msg from server
    while(1){
        if(recv(server_sock , &rcv_msg , sizeof(Msg) , 0) < 0){
            perror("recv");
            break;
        }
        printf("%s: %s\n", rcv_msg.name, rcv_msg.msg);
    }

    close(server_sock);
    return 0;
}
