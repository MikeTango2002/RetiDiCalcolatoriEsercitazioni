#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <time.h>

#define handle_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)
#define PORT 9090
#define MAX_SEQUENCE_NUMBER 5

typedef struct Message{
    int source;
    int sequence;
    int payload;
} Message;

void log(int id, char * format, ...) {
    va_list args;
    va_start(args, format);
    printf("%d: ", id);
    printf(format, args);
    va_end(args);
}

int create_socket_broadcast(int port){

    int sock;
    int yes = 1;
    struct sockaddr_in my_addr;
    int addr_len;
    int ret;

    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0){
        handle_error("socket error");
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes));
    if (ret == -1){
        handle_error("setsockopt error");
    }

    //initialise sockaddr struct
    addr_len = sizeof(struct sockaddr_in);
    memset((void *)&my_addr, 0, addr_len);
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htons(INADDR_ANY);
    my_addr.sin_port = htons(port);

    ret = bind(sock, (struct sockaddr *)&my_addr, addr_len);
    if (ret < 0){
        handle_error("bind error");
    }

    return sock;
    
}

int compare(Message msg1, Message msg2){
    // returns 0 if the messages are the same, else returns 1

    if(msg1.payload == msg2.payload && msg1.source == msg2.source && msg1.sequence == msg2.sequence) return 0;
    else return 1;
}

int check_neighbors(int source, int* neighbors, int num_neighbors){

    for(int i = 0; i < num_neighbors; i++){

        if (source == neighbors[i]) return 0;
    }
    return -1;
}

int broadcast(int id, int* neighbors, int num_neighbors){
    //Receive and send messages in broadcast. 
    //Node 0 sends first packet in the network
    
    struct sockaddr_in broadcast_addr;
    int sock;
    int addr_len;
    int count;
    int ret;
    fd_set readfd;
    int i;

    struct timeval timeout = {0};
    
    Message msg = {0};
    Message last_msg_received = {0};

    //initialise broadcast address and port
    addr_len = sizeof(struct sockaddr_in);
    memset((void *)&broadcast_addr, 0, addr_len);
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(PORT);

    sock = create_socket_broadcast(PORT);

    //set timeout for the select function
    timeout.tv_sec = 10;
    timeout.tv_usec = 0; 
    

    //node leader sends first packet in the network, waiting 5 seconds so the other nodes can start correctly
    if(id == 0){

        sleep(5);

        printf("Prepare broadcast message\n");

        memset(&msg, 0, sizeof(msg));
        msg.source = id;
        msg.sequence = 0;
        msg.payload = rand() % 50; //random payload beetween 0 and 50
    
        printf("Sending %d in broadcast\n", msg.payload);

        ret = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&broadcast_addr, addr_len);
        if (ret < 0){
        handle_error("sendtoerror");
        }

    }

    //receving and sending packets
    while(1){
        FD_ZERO(&readfd);
        FD_SET(sock, &readfd);

        ret = select(sock + 1, &readfd, NULL, NULL, &timeout); 

        if (ret > 0){
            if (FD_ISSET(sock, &readfd)){

                count = recvfrom(sock, &msg, sizeof(msg), 0, NULL, NULL);

                if (msg.source != id && compare(last_msg_received, msg)){ 
                
                    ret = check_neighbors(msg.source, neighbors, num_neighbors);
                    if (ret == 0){
                        if(msg.sequence >= MAX_SEQUENCE_NUMBER) {
                            printf("Max sequence message reached. Closing...\n ");
                            ret = close(sock);
                            if (ret < 0) handle_error("close socket error");
                            return 0;
                        }
                        printf("Accepting message %d sent by %d. Sequence: %d\n", msg.payload, msg.source, msg.sequence);
                        
                        last_msg_received = msg;
                        msg.source = id;
                        msg.sequence++;

                        printf("Sending %d in broadcast\n", msg.payload);
                        ret = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&broadcast_addr, addr_len);
                    }
                }
            }
        }
        else{
            ret = close(sock);
            if (ret < 0) handle_error("close socket error");
            return 0;
        }
    }
}



int main(int argc, char *argv[]){   
    
    int ret;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 3) {
        handle_error("No neighbor for this node");
    }

    int id = atoi(argv[1]);

    int * neighbors = calloc(sizeof(int), argc - 2);
    for (int i = 0; i < argc - 2; i ++) {
        neighbors[i] = atoi(argv[i + 2]);
    }

    log(id, "begin\n");
    printf("Inizializing...\n");
    // Implementation
    int num_neighbors = argc - 2;

    ret = broadcast(id, neighbors, num_neighbors);
    if(ret == -1){
            handle_error("Error in send_broadcast");
        }
    
    free(neighbors);
    return 0;
}
