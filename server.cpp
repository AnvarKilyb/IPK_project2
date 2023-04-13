#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <netinet/in.h>
#include <cpuid.h>
#include <string>

#define MAXHOST 1024
#define MAXSERV 32
#define BUFFERSIZE 1024

using namespace std;
char *HOST, *MODE;
int PORT = 0;


void check_args(int argc, char *argv[]){
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:p:m:")) != -1) {
        switch (opt) {
            case 'h':
                HOST = optarg;
                break;
            case 'p':
                PORT = atoi(optarg);
                break;
            case 'm':
                MODE = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -h host -p port -m mode\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if(HOST == NULL || PORT == 0 || MODE == NULL){
        fprintf(stderr, "Usage: %s -h host -p port -m mode\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (PORT < 1024) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(MODE, "tcp") != 0 && strcmp(MODE, "udp") != 0) {
        fprintf(stderr, "Invalid mode. Enter either tcp or udp\n");
        exit(EXIT_FAILURE);
    }



}

// serv_addr.sin_addr.s_addr = inet_addr("192.168.1.100");

int main(int argc, char *argv[]){
    check_args(argc, argv);
    printf("%s\n", HOST);
    printf("%i\n", PORT);
    printf("%s\n", MODE);

    
}


