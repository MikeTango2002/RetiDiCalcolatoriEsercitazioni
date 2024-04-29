#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#define handle_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)

#define PORT 8080


//My in-ram database is a linked list of "entry" structs

typedef struct Entry{

    char* key;
    char* value;
    time_t expire_time;

} Entry;


typedef struct Node{

    Entry * entry;
    
    struct Node * next;

} Node;

typedef struct List{

    Node * head;

} List;

List * list;

List * create_list(){

    List * ret = malloc(sizeof(List));

    ret -> head = NULL;
    
    return ret;
}

//Create a new entry
Entry * create_entry(char * key, char * value){


    int size_key = strlen(key) + 1;

    int size_value = strlen(value) + 1;

    Entry * entry = malloc(sizeof(Entry));

    entry -> key = malloc(size_key);

    entry -> value = malloc(size_value);

    strcpy(entry -> key, key);
    strcpy(entry -> value, value);

    entry -> expire_time = -1; //default value: no expire time

    return entry;

}


//Create a new node in the list "list" with entry "entry" as value
/********************************************/
Node * create_node(Entry * entry){


    Node * res = malloc(sizeof(Node));

    res -> entry = entry;

    res -> next = NULL;

    return res;

}

char * set(List * list, Entry * entry){


    Node * tmp = list -> head;

    if(tmp == NULL){

        list -> head = create_node(entry);

        return "+OK\r\n";

    }

    while(tmp){

        if(strcmp((tmp -> entry -> key), entry -> key) == 0){


            strcpy((tmp -> entry -> value), (entry -> value));

            return "+OK\r\n";
        }

        else{

            tmp = tmp -> next;

        }

    }


    //There isn't key in the list: create the new node and add it to the head of the list

    tmp = list -> head;

    list -> head = create_node(entry);

    list -> head -> next = tmp;

    return "+OK\r\n";
}

/************************************************************/

//Get the key "key", if exists from the list "list"
char * get(List * list, char * key){

    Node * tmp = list -> head;

    while(tmp){


        if(strcmp((tmp -> entry -> key), key) == 0){


            char * value = tmp -> entry -> value;

            int length = strlen(value);

            char * formatted_message = (char*)malloc(sizeof(char) * 25);

            //Preparing the message to send to the client, using the redis protocol rules correctly 
    
            sprintf(formatted_message, "$%d\r\n", length);
            memcpy(formatted_message + strlen(formatted_message), value, length);
            strcat(formatted_message, "\r\n");
            
            return formatted_message;

        }

        else{
    
            tmp = tmp -> next;

        }

    }

    return "$-1\r\n";

    
}

//Remove, if exists, the key "key" from the list "list"
void remove_key(List * list, char * key){


    if(strcmp((list -> head -> entry -> key), key) == 0){

            list -> head = list -> head -> next;
            
            return;
    }


    Node * tmp = list -> head;

    while(tmp -> next){

        if(strcmp((tmp -> next -> entry -> key), key) == 0){

            tmp -> next = tmp -> next -> next;

            return;
        }

        else{

            tmp = tmp -> next;

        }

    }

    return;


}

void set_expire_time(Entry * entry, int ttl){

    entry -> expire_time = time(NULL) + ttl;
}

void check_expire_time(List * list){

    time_t current_time = time(NULL);

    if(list -> head == NULL) return;

    //Handle the case in which the head of the list needs to be removed
    while(1){
        if(list -> head == NULL) return;
        if(list -> head -> entry -> expire_time != -1 && list -> head -> entry -> expire_time <= current_time){

                remove_key(list, list -> head -> entry -> key);
                
        }
        else{ break; }
    }

    Node * tmp = list -> head;

    if(tmp == NULL) return;

    while(tmp -> next){

        current_time = time(NULL);

        //printf("%ld: current_time; %ld: expire_time\n", current_time, tmp -> entry -> expire_time);

        if(tmp -> next -> entry -> expire_time != -1 && tmp -> next -> entry -> expire_time <= current_time){

            remove_key(list, tmp -> next -> entry -> key);
        }
        
        tmp = tmp -> next;

        if(tmp == NULL) return;
    
    }

}

//This function parses the message and executes the command indicated by the message
char * parse_message(char * message, List* list){

    
    int arg_count;
    const char *arg_start;
    int arg_length;

    // Check the number of arguments
    sscanf(message, "*%d", &arg_count);
    message = strchr(message, '\n') + 1; // Skip "$'length_argument'\r\n"
    printf("Number of arguments: %d\n", arg_count);


    char * command_parsed[arg_count];

    for(int i = 0; i < arg_count; i++){

        command_parsed[i] = calloc(20, sizeof(char));
    }

    // Message parsing
    for (int i = 0; i < arg_count; i++) {
        sscanf(message, "$%d\r\n", &arg_length);
        message = strchr(message, '\n') + 1;  // Skip "$n\r\n"
        arg_start = message;

        //Extract next argument from the command message
        char arg_value[arg_length + 1];
        strncpy(arg_value, arg_start, arg_length);
        arg_value[arg_length] = '\0';

        //Copy the argument in command_parsed
        strcpy(command_parsed[i], arg_value);

        printf("Argument %d: %s\n", i + 1, command_parsed[i]);

        message = strchr(message, '\n') + 1;  // Moving the pointer forward
    }

    check_expire_time(list);


    if(strcmp(command_parsed[0], "GET") == 0){

        return get(list, command_parsed[1]);

    }

    if(strcmp(command_parsed[0], "SET") == 0){

        Entry * entry = create_entry(command_parsed[1], command_parsed[2]);

        if(arg_count > 3){
            //There is also the expire command
            set_expire_time(entry, atol(command_parsed[4]));
        }
        return set(list, entry);
    }
}



//Handle the connection with a client via socket: get message from the client and send back a response message
void handle_connection(int socket){

    fd_set readfd;
    int ret;
    char buffer[100];
    

    //Client sends to the server two messeages indicating lib-name and lib-version.
    //The server receives those messages and sends back to the client the OK message.
    /*******************************/
    memset(buffer, 0, sizeof(buffer));
    ret = recv(socket, buffer, sizeof(buffer)-1, 0);
    if (ret == -1) handle_error("Cannot read from the socket");
    if (ret == 0) handle_error("Invalid command");    
    buffer[ret] = '\0';

    printf("%s\n", buffer);

    ret = send(socket, "+OK\r\n", 5, 0); 

    memset(buffer, 0, sizeof(buffer));
    ret = recv(socket, buffer, sizeof(buffer)-1, 0);
    if (ret == -1) handle_error("Cannot read from the socket");
    if (ret == 0) handle_error("Invalid command");    
    buffer[ret] = '\0';

    printf("%s\n", buffer);

    ret = send(socket, "+OK\r\n", 5, 0);
    /**********************************/

    //waiting client commands
    while(1){

        memset(buffer, 0, sizeof(buffer));
        FD_ZERO(&readfd);
        FD_SET(socket, &readfd);

        ret = select(socket + 1, &readfd, NULL, NULL, NULL); 

        if (ret > 0){

            if (FD_ISSET(socket, &readfd)){

            //Client sent me something... 
            ret = recv(socket, buffer, sizeof(buffer)-1, 0);
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0){

                printf("The client closed the connection.\n");
                ret = close(socket);
                if (ret < 0) handle_error("close socket error");
                return ;

            }    
            buffer[ret] = '\0';

            char * message = parse_message(buffer, list);

            ret = send(socket, message, strlen(message), 0);
            }
        }

        else{

            ret = close(socket);
            if (ret < 0) handle_error("close socket error");
            return ;
        }
    }

 
}



int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    int pid;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections...\n");

    if (listen(server_fd, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    list = create_list();

    while(1){

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Accepted a new connection!\n");
        handle_connection(new_socket);

        // closing the connected socket
        close(new_socket);
        
    }

    // closing the listening socket
    close(server_fd);
    
    return 0;
}