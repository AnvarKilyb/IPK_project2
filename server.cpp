#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cpuid.h>
#include <string>
#include <sstream>

#define BUFFERSIZE 512

#define INIT_STATE 0
#define ESTABLISHED_STATE 1

// Occuring during SOLVE command
#define TEXT_OK 0
#define TEXT_WRONG_COMMAND -1
#define TEXT_WRONG_OPERAND -2
#define TEXT_WRONG_FORMAT -3

using namespace std;
char *HOST, *MODE;
int PORT = 0;
short connections = 0;

// Receiveng messages from client, then sending results
void event_handler(int);
// Helping function to write messages to client back
void send_message(int, string);
// Function to compute SOLVE command
double text_solve(const char*, short*);
// Basic function to print ERRORs
void print_err(string);

// Basic function to parse arguments
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

// Run server if TCP connection
void run_tcp(){
    printf("TCP running...\n");
    int sockfd, newsockfd, portno, pid;
    char buffer[BUFFERSIZE];
    struct sockaddr_in serv_addr, cli_addr;
    // Creating socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) print_err("Error opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) {
                print_err("ERROR on binding");
    }
    if(listen(sockfd,5) < 0) print_err("Listen failed");
    socklen_t clilen = sizeof(cli_addr);
    // Sets server on loop, in order to stop interrupting server after computing clients requests
    while(true){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        connections++;
        if(newsockfd < 0)
            print_err("ERROR on accept");
        // Forking each connection from clients
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
// Run server if UDP connection
void run_udp(){
    printf("UDP running...\n");
}


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

// -------------------------- Helping functions ---------------------------------------

// Receiveng messages from client, then sending results
void event_handler(int sock){
    char buffer[BUFFERSIZE];
    int result = 0;
    bool command_ok = false;
    // Parameter to determine server's state:
    // Init state = 0
    // Established state = 1
    // Values are defined
    short state = INIT_STATE;
    short txt_err = TEXT_OK;
    

    while(true){
        bzero(buffer, BUFFERSIZE);
        if (read(sock, buffer, BUFFERSIZE-1) < 0) print_err("ERROR reading from socket");
        cout << connections << ":" << "Received message from client: " << buffer;
        // Checks if received message is HELLO flips state to ESTABLISHED
        if (strcmp(buffer, "HELLO\n") == 0 && state == INIT_STATE){
            cout << connections << ":"  << "Received greeting from client, sets state to ESTABLISHED" << endl << endl;
            send_message(sock, "HELLO\n");
            state = ESTABLISHED_STATE;
        }
        // Checks if received message is BYE
        else if(strcmp(buffer, "BYE\n") == 0){
            cout << connections << ":"  << "Received farewell from client, interrupting connection..." << endl << endl;
            send_message(sock, "BYE\n");
            break;
        }
        else if(state == ESTABLISHED_STATE){
            result = text_solve(buffer, &txt_err);
            if(txt_err == 0){
                
                switch(txt_err){
                    case TEXT_WRONG_COMMAND:
                        cout << connections << ":"  << "Received unknown command from client, interrupting connection..." << endl << endl;
                        send_message(sock, "BYE\n");
                        break;
                    case TEXT_WRONG_FORMAT:
                        cout << connections << ":"  << "Received wrong SOLVE format from client, interrupting connection..." << endl << endl;
                        send_message(sock, "BYE\n");
                        break;
                    case TEXT_WRONG_OPERAND:
                        cout << connections << ":"  << "Received invalid operand at SOLVE from client, interrupting connection..." << endl << endl;
                        send_message(sock, "BYE\n");
                        break;
                }

                char prep_message[BUFFERSIZE];
                bzero(prep_message, BUFFERSIZE);
                strcat(prep_message, "RESULT ");
                strcat(prep_message, to_string(result).c_str());
                strcat(prep_message, "\n");
                cout << connections << ":"  << "Sending result message to client" << endl << endl;
                send_message(sock, prep_message);
            }else{
                cout << connections << ":"  << "Received wrong command from client, interrupting connection..." << buffer << endl << endl;
                send_message(sock, "BYE\n");
                break;
            }
        }
    }

}
// Function to write messages to client back
void send_message(int sock, string msg){
    if(write(sock,msg.c_str(), msg.length()) < 0)
        print_err("ERROR writing to socket");
}

// Function to compute SOLVE command
double text_solve(const char* input, short* err_code){
    stringstream ss(input);
    string command, arg1, arg2, operand;
    double a, b;
    ss >> command >> operand >> arg1 >> arg2; // SOLVE (OPERAND ARG1 ARG2)
    try
    {
        a = stod(arg1);
        b = stod(arg2.erase(arg2.size() - 1)); // Erases closing bracket at ARG2
    }
    catch(const std::exception& e)
    {
        *err_code = TEXT_WRONG_FORMAT;
        return 0;
    }
    
    if(command == "SOLVE"){
        *err_code = TEXT_OK;
        switch(input[7]){
            case '+':
                return a + b;
                break;
            case '-':
                return a - b;
                break;
            case '*':
                return a * b;
                break;
            case '/':
                return a / b;
                break;
            default:
                *err_code = TEXT_WRONG_OPERAND;
                return 0;
        }
    }else
        *err_code = TEXT_WRONG_COMMAND;
    return 0;
}

// Basic function to print ERRORs
void print_err(string msg){
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

