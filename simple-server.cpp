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

using std::cout;
using std::endl;

static const int BACKLOG = 10;


int get_socket_and_listen(const char *port_number);

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

    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int accept_sockfd;
    int listen_sockfd = get_socket_and_listen(argv[1]);
    if((accept_sockfd = accept(listen_sockfd, (struct sockaddr *)&their_addr, &addr_size)) < 0){
        perror("Failed to accept incoming connection");
        close(listen_sockfd);
        exit(1);
    }
    close(listen_sockfd);
    close(accept_sockfd);
    return 0;

}

/**
 *
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
