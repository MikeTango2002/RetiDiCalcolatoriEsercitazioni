#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdarg.h>

void log(int id, char * format, ...) {
    va_list args;
    va_start(args, format);
    printf("%d: ", id);
    printf(format, args);
    va_end(args);
}


int main(int argc, char * argv[]){

    //setvbuf(stdout, NULL, _IONBF, 0);
    //int id = argv[1];
    //log(id, "begin\n");
    printf("Hello world!\n");
    return 0;
}