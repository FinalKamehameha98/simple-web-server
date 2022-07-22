/**
 * Simple Web Server
 * Author: Andres Rivera
 * 
 *
 *
 *
 */

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <regex>

using std::cout;
using std::endl;

static const int BACKLOG = 10;


int get_socket_and_listen(const char *port_number);
void handle_client(int accept_sockfd);
int accept_connection(int listen_sockfd);


/**
 * 
 *
 *
 * @param argc Number of arguments on command line
 * @param argv Array of arguments from command line
 */
int main(int argc, char *argv[]){
    if(argc != 3){
        cout << "Usage: " << argv[0] << " PORT_NUMBER DIRECTORY" << endl;
        exit(1);
    }

    int listen_sockfd = get_socket_and_listen(argv[1]);
    int accept_sockfd = accept_connection(listen_sockfd);
    close(listen_sockfd);
    close(accept_sockfd);
    //handle_client(accept_sockfd)
    return 0;

}

/**
 *
 *
 * @param port_number Port number where files will be served
 * @return 
 */
int get_socket_and_listen(const char *port_number){
    int status;
    struct addrinfo hints;
    struct addrinfo *result;
    int listen_sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port_number, &hints, &result)) != 0){
        perror("Failed to get address information");
        exit(1);   
    }
    
    if((listen_sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0){
        perror("Failed to create listening socket");
        freeaddrinfo(result);
        exit(1);
    }
    
    if((status = bind(listen_sockfd, result->ai_addr, result->ai_addrlen)) < 0){
        perror("Failed to bind socket to IP address");
        freeaddrinfo(result);
        exit(1);
    }

    int reuse_yes = 1;
    freeaddrinfo(result);
    if((status = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_yes, sizeof(reuse_yes))) < 0){
        perror("Failed to set socket options");
        exit(1);
    }
    
    if((status = listen(listen_sockfd, BACKLOG)) < 0){
        perror("Failed to listen to incoming connections");
        exit(1);
    }
    
    cout << "Listening at port " << port_number << endl;
    return listen_sockfd;
}

/**
 *
 */
int accept_connection(int listen_sockfd){
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int accept_sockfd;

    if((accept_sockfd = accept(listen_sockfd, (struct sockaddr *)&their_addr, &addr_size)) < 0){
        perror("Failed to accept incoming connection");
        close(listen_sockfd);
        exit(1);
    }
    return accept_sockfd;
}



/**
 *  @param accept_sockfd Represents accepted connection with client 
 */
//void handle_client(int accept_sockfd){
//    char [2048];
//    int bytes_recv;
//
//    if((bytes_recv = recv(accept_sockfd, http_request, sizeof(buffer), 0)) < 0){
//        perror("Failed to receive data");
//        close(accept_sockfd);
//        exit(1);
//    }
//
//
//}
//
///**
// *
// */
//void send_http_400_response(int accept_sockfd){
//
//}
//
