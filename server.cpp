#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cpuid.h>
#include <string>

#define BUFFERSIZE 1024

using namespace std;
char *HOST, *MODE;
int PORT = 0;

void event_handler(int);
void print_err(string msg){
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

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
                print_err(("Usage: %s -h host -p port -m mode\n", argv[0]));
        }
    }
    if(HOST == NULL || PORT == 0 || MODE == NULL)
        print_err(("Usage: %s -h host -p port -m mode\n", argv[0]));
    
    if (PORT < 1024)
        print_err("Invalid port number");

    if (strcmp(MODE, "tcp") != 0 && strcmp(MODE, "udp") != 0)
        print_err("Invalid mode. Enter either tcp or udp");
}

void run_tcp(){
    printf("TCP running...\n");
    int sockfd, newsockfd, portno, pid;
    char buffer[BUFFERSIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        print_err("Error opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) {
                print_err("ERROR on binding");
    }
    if(listen(sockfd,5) < 0)
        print_err("Listen failed");
    socklen_t clilen = sizeof(cli_addr);
    while(true){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd < 0)
            print_err("ERROR on accept");
        pid = fork();
        if(pid < 0)
            print_err("ERROR on fork");
        if(pid == 0){
            close(sockfd);
            event_handler(newsockfd);
            exit(EXIT_SUCCESS);
            
        }
        else
            close(newsockfd);
    }

}
void run_udp(){
    printf("UDP running...\n");
}

// serv_addr.sin_addr.s_addr = inet_addr("192.168.1.100");

int main(int argc, char *argv[]){
    check_args(argc, argv);

    if(strcmp(MODE, "tcp") == 0){
        run_tcp();
    }
    else if (strcmp(MODE, "udp") ==0){
        run_udp();
    }

    return 0;
    
}


void event_handler(int sock){
    int n;
   char buffer[256];
      
   bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) print_err("ERROR reading from socket");
   printf("Here is the message: %s\n",buffer);
   n = write(sock,"I got your message",18);
   if (n < 0) print_err("ERROR writing to socket");
}


