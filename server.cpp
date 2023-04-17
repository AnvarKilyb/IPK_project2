#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
// #include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <regex>
#include <csignal>

#define BUFFERSIZE 512

#define INIT_STATE 0
#define ESTABLISHED_STATE 1

// Occuring during SOLVE command
#define TEXT_OK 0
#define TEXT_WRONG_COMMAND -1
#define TEXT_WRONG_FORMAT -2

using namespace std;
char buffer[BUFFERSIZE];
char *HOST, *MODE;
int PORT = 0;
int sockfd, newsockfd, pid;
short connections = 0;

// Receiveng messages from client, then sending results, only for TCP
void event_handler(int);
// Helping function to write messages to client back
void send_message(int, string);
// Function to check command "SOLVE" and compute lisp expression
double text_solve(string, short*);
// Basic function to print ERRORs
void print_err(string);
// Function to compute lisp expression
int expr(const smatch&);
// Function to parse lisp expression
int lisp(string, bool*);

regex LISP_RE(R"(\(([+\-*/]) (\d+) (\d+)\))");

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
                print_err("Usage: ./ipkcpd -h host -p port -m mode\n");
        }
    }
    if(HOST == NULL || PORT == 0 || MODE == NULL)
        print_err("Usage: ./ipkcpd -h host -p port -m mode\n");
    
    if (PORT < 1024)
        print_err("Invalid port number");

    if (strcmp(MODE, "tcp") != 0 && strcmp(MODE, "udp") != 0)
        print_err("Invalid mode. Enter either tcp or udp");
}

//Signal handler for SIGINT
void signalHandler(int signum){
    // Only parent message needs to print message
    if(pid > 0) cout << "Interrupt signal (" << signum << ") received\n";

    //check if buffer is empty
    if (strcmp(buffer, "") != 0) {
        memset(buffer, 0, BUFFERSIZE);
    }

    //Pressing ctrl+c and using TCP sends BYE to client
    if (strcmp(MODE, "tcp") == 0) {
        // Before interrupting sends BYE message to client
        if(pid == 0){
            close(sockfd);
            send_message(newsockfd, "BYE\n");
        }else{
            close(newsockfd);
            
        }
    }

    //close sockets
    if(newsockfd != 0) close(newsockfd);
    if(sockfd != 0) close(sockfd);
    fflush(stdout);

    exit(signum);

}

// Run server if TCP connection
void run_tcp(){
    cout << "TCP running..." << endl;
    struct sockaddr_in serv_addr, cli_addr;
    // Creating socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) print_err("Error opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    // Filling server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    serv_addr.sin_port = htons(PORT);
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
    cout << "UDP running..." << endl;
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) print_err("ERROR opening socket");

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    // Filling server information
    servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(HOST);
    servaddr.sin_port = htons(PORT);
    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, 
            sizeof(servaddr)) < 0 )
    {
        print_err("ERROR on binding");
    }
    while(true){
        memset(buffer, 0, BUFFERSIZE);
        socklen_t len;
        int n;
        bool ok_flag = true;

        len = sizeof(cliaddr);
        n = recvfrom(sockfd, (char *)buffer, BUFFERSIZE, 
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);
        buffer[n] = '\0';
        cout << "Received datagram from client: " << buffer + 2;

        // Result data that will be sent back to client
        char result_data[BUFFERSIZE];
        bzero(result_data, BUFFERSIZE);

        int expr_result = lisp(buffer + 2, &ok_flag);
        char prep_data[BUFFERSIZE];
        bzero(prep_data, BUFFERSIZE);
        result_data[0] = 1;
        if(ok_flag){
            strncpy(prep_data, to_string(expr_result).c_str(), sizeof(prep_data));
            result_data[1] = '\0'; //OK status
            result_data[2] = static_cast<char>(strlen(prep_data));
            memcpy(&result_data[3], prep_data, strlen(prep_data));
            cout << "\nResult: " << result_data + 3 << endl;
            sendto(sockfd, result_data, strlen(prep_data) + 3, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
            cout << "Result data sent" << endl << endl;
        }
        else{
            result_data[1] = 1; //Error status
            result_data[2] = strlen(prep_data);
            strcat(prep_data, "Invalid argument\n");
            memcpy(result_data + 3, prep_data, strlen(prep_data));
            cout << "\nInvalid lisp expression, sending to client error code" << endl << endl;
            sendto(sockfd, result_data, strlen(prep_data) + 3, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        }
    }
}

int main(int argc, char *argv[]){
    //Register signal handler
    signal(SIGINT, signalHandler);

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
                }

                char prep_message[BUFFERSIZE];
                bzero(prep_message, BUFFERSIZE);
                strcat(prep_message, "RESULT ");
                strcat(prep_message, to_string(result).c_str());
                strcat(prep_message, "\n");
                cout << connections << ":"  << "Sending result message to client" << endl << endl;
                send_message(sock, prep_message);
            }else{
                cout << connections << ":"  << "Received wrong command from client, interrupting connection..." << endl << endl;
                send_message(sock, "BYE\n");
                break;
            }
        }
        else{
            cout << connections << ":"  << "Received wrong command from client, interrupting connection..." << endl << endl;
            send_message(sock, "BYE\n");
            break;
        }
    }

}
// Function to write messages to client back
void send_message(int sock, string msg){
    if(write(sock,msg.c_str(), msg.length()) < 0)
        print_err("ERROR writing to socket");
}

double text_solve(string input, short* err_code){
    int result = 0;
    size_t pos = input.find(" ");
    string command = input.substr(0, pos);
    string expr = input.substr(pos+1);
    if(command == "SOLVE")
        *err_code = TEXT_OK;
    else
        *err_code = TEXT_WRONG_COMMAND;
    bool ok_flag = true;
    result = lisp(expr, &ok_flag);
    if(!ok_flag)
        *err_code = TEXT_WRONG_FORMAT;
    return result;
}

// Function to compute lisp expression
int expr(const smatch& match) {
    char op = match[1].str()[0];
    int a = stoi(match[2].str());
    int b = stoi(match[3].str());
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default: 
            print_err("Invalid lisp\n");
            return -1;
    }
}

// Function to parse lisp expression
int lisp(string data, bool* ok) {
    data = regex_replace(data, std::regex(R"(\n)"), "");
    while (!regex_match(data, regex(R"(\d+)"))) {
        smatch subexpr;
        if (!regex_search(data, subexpr, LISP_RE)) {
            *ok = false;
            return 0;
        }
        int subres = expr(subexpr);
        // data = regex_replace(data, LISP_RE, to_string(subres), "");
        data = regex_replace(data, LISP_RE, to_string(subres), regex_constants::format_first_only);
    }
    *ok = true;
    return stoi(data);
}

// Basic function to print ERRORs
void print_err(string msg){
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}
