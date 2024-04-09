#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/time.h>
#include <signal.h>


#include "traffic_generator.h"

#define handle_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)

#define PORT 9090

#define TIME_FRAME_MSEC 10

#define NUM_NODES 10

//#define DEBUG 1


//-------------------------
// Utility
// ------------------------

//global variables

int nodes_pkt_last_sequence_num[NUM_NODES];

TrafficAnalyzer * analyzer;

int nodes_pkt_received[NUM_NODES];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t mutex_list_analyzer = PTHREAD_MUTEX_INITIALIZER;

//////

/**
 * Sleep a given amount of milliseconds
 */
int msleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0){
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do{
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}


//Create socket broadcast and bind it with port
int create_socket_broadcast(){

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
    my_addr.sin_port = htons(PORT);

    ret = bind(sock, (struct sockaddr *)&my_addr, addr_len);

    if (ret < 0){

        handle_error("bind error");
    }

    return sock;
    
}

struct sockaddr_in * initialise_broadcast_addr(){

    int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in * broadcast_addr = malloc(addr_len);

    if (broadcast_addr == NULL){
        
        handle_error("Malloc errror");
    }

    memset(broadcast_addr, 0, addr_len);
    broadcast_addr -> sin_family = AF_INET;
    broadcast_addr -> sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr -> sin_port = htons(PORT);

    return broadcast_addr;

}

//Function to create a new network node

Node * create_node(int id){

    Node * node = malloc(sizeof(Node));
    
    if (node == NULL){
        
        handle_error("Malloc errror");
    }

    node -> id = id;
    node -> sock = create_socket_broadcast();
    node -> broadcast_addr = initialise_broadcast_addr();
    return node;
}


//Function to create a new message with random int payload

Message * prepare_message(int source, int sequence_num){

    Message * msg = malloc(sizeof(Message));

    if(msg == NULL){

        handle_error("Error during malloc");
    }

    int value = rand() % 100;

    Payload payload = {0};

    payload.value = value;

    msg -> source = source;
    msg -> sequence_num = sequence_num;
    msg -> payload_lenght = sizeof(payload);
    msg -> payload = payload;

    return msg;

}



/**
 * Bind the given socket to all interfaces (one by one)
 * and send the message
 */
void send_to_all_interfaces(Node * node, Message * msg) {

    struct ifaddrs *addrs, *tmp;
    int ret;

    getifaddrs(&addrs);
    tmp = addrs;

        while (tmp){

            ret = setsockopt(node->sock, SOL_SOCKET, SO_BINDTODEVICE, tmp->ifa_name, sizeof(tmp->ifa_name));
            if(ret < 0){

                handle_error("Recv socket bind interface");

            }
            
            pthread_mutex_lock(&mutex);
            ret = sendto(node->sock, msg, sizeof(Message), 0, (struct sockaddr *) node->broadcast_addr, sizeof(struct sockaddr_in));
            if(ret < 0){

                handle_error("Recv socket bind interface");

            }
            pthread_mutex_unlock(&mutex);

            tmp = tmp->ifa_next;
        }

    freeifaddrs(addrs);

    // Listen from all interfaces
    ret = setsockopt(node->sock, SOL_SOCKET, SO_BINDTODEVICE, NULL, 0);
    if (ret < 0){

        handle_error("Recv socket bind interface");
    }
    

}




//---------------
// Traffic Generator
//---------------


// Some sistem that triggers handler callback every x milliseconds
void * traffic_generator(void * node_ptr){

    Node * node = (Node *) node_ptr;

    Message * msg;
    Payload payload = {0};
    int value;
    srand( (unsigned)time( NULL ) );

    int sequence_num = 0;
    
    while(1){

        msg = prepare_message(node->id, sequence_num);

        msleep(TIME_FRAME_MSEC);

        send_to_all_interfaces(node, msg);

        sequence_num++;
    }
}



//---------------
// Broadcaster
//---------------

void send_broadcaster(Node * node, Message * msg) {
    // Send UDP packet and update local sequence
    
    //msg -> hop++;

    send_to_all_interfaces(node, msg);
}

void process_broadcaster(Node * node) {
    // In case of packet reception, notify the handler
    // Discard already seen packets
    

    fd_set readfd;

    Message * msg = malloc(sizeof(Message));
    if (msg == NULL){

        handle_error("Malloc error");
    }




#ifdef DEBUG
    printf("%d: inizializing Node...\n", node->id);
#endif

    int count;

    while (1)
    {
        FD_ZERO(&readfd);
        FD_SET(node->sock, &readfd);

        struct timeval tv = {5, 0};
        int ret = select(node->sock + 1, &readfd, NULL, NULL, &tv);
        if (ret > 0){

            if (FD_ISSET(node->sock, &readfd)){

                count = recvfrom(node->sock, msg, sizeof(Message), 0, NULL, 0);
                if(ret < 0){

                handle_error("Recvfrom error");

                }
            
                //if msg -> hop <= nodes_pkt_last_sequence_num[msg -> source] : discard packet (already received)
                if (msg -> sequence_num > nodes_pkt_last_sequence_num[msg -> source]){
                    nodes_pkt_last_sequence_num[msg -> source] = msg -> sequence_num;

                    pthread_mutex_lock(&mutex_list_analyzer);
                    nodes_pkt_received[msg -> source] += 1;
                    pthread_mutex_unlock(&mutex_list_analyzer);

                    //add packet to the list of packets received
                    received_pkt(analyzer, msg);

                    #ifdef DEBUG
                    if(node -> id == 0)
                    printf("%d: recv %d from %d:msg hop %d\n", node->id, msg->payload.value, msg->source, msg->sequence_num);
                    #endif
                }
                send_broadcaster(node, msg);
                if (ret < 0)
                {
                    handle_error("Error sending broadcast");
                }

                    
            }
                

        }
    }


}

void * broadcaster(void * node_ptr){

    Node * node = (Node *) node_ptr;

    process_broadcaster(node);

}



//---------------
// Traffic Analyzer
//---------------

TrafficAnalyzer * create_traffic_analyzer(){

    TrafficAnalyzer * traffic_analyzer_list = malloc(sizeof(TrafficAnalyzer));

    if (traffic_analyzer_list == NULL){

        handle_error("Malloc error");
    }

    traffic_analyzer_list -> head = NULL;
    traffic_analyzer_list -> tail = NULL;

    return traffic_analyzer_list;
}

void received_pkt(TrafficAnalyzer * analyzer, Message *msg) {
    // Record the packet sent and datetime of it
    struct timeval datetime;

    gettimeofday(&datetime, NULL);

    pthread_mutex_lock(&mutex_list_analyzer);

    append_node_traffic_analyzer(analyzer, msg, datetime);

    pthread_mutex_unlock(&mutex_list_analyzer);
}

void append_node_traffic_analyzer(TrafficAnalyzer * traffic_analyzer, Message * msg, struct timeval datetime){

    NodeTrafficAnalyzer * node = malloc(sizeof(NodeTrafficAnalyzer));

    if(node == NULL){

        handle_error("Malloc error");
    }

    node -> msg = msg;
    node -> datetime = datetime;
    
    if (traffic_analyzer -> tail == NULL){

        traffic_analyzer -> head = node;
        traffic_analyzer -> tail = node;

        node -> next = NULL;
        node -> prev = NULL;

        return;

    }

    else{
        
         node -> next = NULL;
         node -> prev = traffic_analyzer -> tail;

         traffic_analyzer -> tail -> next = node;
         traffic_analyzer -> tail = node;

         return;
    }
}


void * dump(void * args) {
    // Dump information about the trhoughput of all packets received every second
    //Sliding window of 2 sec
    //Remove old nodes from the list of TrafficAnalyzer object

    struct timeval time_last_pkt_received;
    NodeTrafficAnalyzer * tmp;
    double diff;

    //memset(nodes_pkt_received, 0, sizeof(nodes_pkt_received));

    while(1){


        sleep(1);

/*
        if(analyzer -> tail == NULL){

            continue;
        }

        time_last_pkt_received = analyzer -> tail -> datetime;

        tmp = analyzer -> tail;

        while(tmp != NULL){

            diff = (time_last_pkt_received.tv_sec - tmp->datetime.tv_sec) * 1000.0;      // seconds in milliseconds
            diff += (time_last_pkt_received.tv_usec - tmp->datetime.tv_usec) / 1000.0;   // microseconds in milliseconds

            if (diff <= 2000.0){

                nodes_pkt_received[tmp -> msg -> source] += 1;

                tmp = tmp -> prev;
                
               
            }

            else{

                if (nodes_pkt_received[tmp -> msg -> source] > 0) nodes_pkt_received[tmp -> msg -> source] -= 1;

                tmp -> next -> prev = tmp -> prev;

                tmp = tmp -> prev;
            }


/*
            else{

                analyzer -> head = tmp;
                analyzer -> head -> prev = NULL;
                break;
            }
            
        }
*/      
        print_throughput();

    }
    
}

void print_throughput(){

    for(int i=0; i <= NUM_NODES - 2; i++){

        printf("%d: %04d, ", i, nodes_pkt_received[i]);
    }

    printf("%d: %04d\n", NUM_NODES - 1, nodes_pkt_received[NUM_NODES - 1]);
}

////

void handle_alarm(int signum){

    printf("Time expired. Terminating simulation...\n");
    exit(EXIT_SUCCESS);
}


//---------------
// Main
//---------------

int main(int argc, char * argv[]) {

    // Autoflush stdout for docker
    setvbuf(stdout, NULL, _IONBF, 0);

    if(argc != 2){
        
        handle_error("Wrong number of args");
    }
    
    signal(SIGALRM, handle_alarm);

    int ret;
    int id = atoi(argv[1]);

    Node * node = create_node(id);

    memset(nodes_pkt_last_sequence_num, -1, sizeof(nodes_pkt_last_sequence_num));
    
    analyzer = create_traffic_analyzer();

    memset(nodes_pkt_received, 0, sizeof(nodes_pkt_received));
        


    //Using threads to handle the main nodes function simultaneously
    pthread_t threads[3];

    alarm(20);
    
    // Traffic generator
    //printf("Creating thread traffic generator\n");
    ret = pthread_create(&threads[0], NULL, traffic_generator, node);
    
    if (ret < 0){

        handle_error("TrafficGenerator thread");
    }

    // Broadcaster
    //printf("Creating thread broadcaster\n");
    ret = pthread_create(&threads[1], NULL, broadcaster, node);
    
    if (ret < 0){

        handle_error("Broadcaster thread");
    }

    // Traffic analyzer
    //printf("Creating thread traffic analyzer\n");
    ret = pthread_create(&threads[2], NULL, dump, NULL);
    
    if (ret < 0){

        handle_error("Dump thread");
    }



    
    //

    ret =  pthread_join(threads[0], NULL);
    ret =  pthread_join(threads[1], NULL);
    ret =  pthread_join(threads[2], NULL);

    return 0;
}
