#include <time.h>

//////////
//Utility
/////////

// Network Node struct and function to create a new network node
typedef struct Node{

    int sock;
    int id;
    struct sockaddr_in * broadcast_addr;

} Node;

Node * create_node(int id);


//Message struct and function to create a new message with random int payload
typedef struct Payload {
    int value;
} Payload;

typedef struct Message {
    int source;
    int hop;
    int payload_lenght;
    Payload  payload;
} Message;


Message * prepare_message(int source);

void send_to_all_interfaces(Node * node, Message * msg);

//////////////////
//TrafficGenerator
//////////////////

void * traffic_generator(void * node_ptr);

/////////////
//Broadcaster
/////////////

void send_broadcaster(Node * node, Message * msg);
void process_broadcaster(Node * node);
void * broadcaster(void * node_ptr);

/////////////////
//TrafficAnalyzer
/////////////////

typedef struct NodeTrafficAnalyzer{

    Message * msg;
    struct timeval datetime;

    struct NodeTrafficAnalyzer * next;
    struct NodeTrafficAnalyzer * prev;

} NodeTrafficAnalyzer;



typedef struct TrafficAnalyzer {

    // Definition of sliding window...
    NodeTrafficAnalyzer * head;
    NodeTrafficAnalyzer * tail;

} TrafficAnalyzer;


TrafficAnalyzer * create_traffic_analyzer();
void append_node_traffic_analyzer(TrafficAnalyzer * traffic_analyzer, Message * msg, struct timeval datetime);

void received_pkt(TrafficAnalyzer * analyzer, Message *msg);
void * dump(void * args);

void print_throughput();


