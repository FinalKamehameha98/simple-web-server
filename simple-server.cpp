/**
 * Simple Web Server
 * Author: Andres Rivera
 * 
 * A simple web server that serves files from a given directory through a
 * given port number, receiving and validating HTTP request messages from
 * other 
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
#include <filesystem>

namespace fs = std::filesystem;

// using declarations
using std::cout;
using std::endl;
using std::string;

// Constants
static const int BACKLOG = 10;


// Forward declarations
int get_socket_and_listen(const char *port_number);
void handle_client(int accept_sockfd);
int accept_connection(int listen_sockfd);
bool is_valid_get_request(string request_msg);
string get_400_response();
string get_404_response();
string get_path(string request_msg);

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
    handle_client(accept_sockfd);
    close(listen_sockfd);
    close(accept_sockfd);

    return 0;

}

/**
 * 
 *
 * @param port_number Port number where files will be served
 *
 * @return Socket file descriptor listening for incoming connections
 */
int get_socket_and_listen(const char *port_number){
    int status;
    struct addrinfo hints;
    struct addrinfo *result;
    int listen_sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP connections expected
    hints.ai_flags = AI_PASSIVE; // Any IP address

    // Fills up 
    if((status = getaddrinfo(NULL, port_number, &hints, &result)) != 0){
        perror("Failed to get address information");
        exit(1);   
    }
    
    if((listen_sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0){
        perror("Failed to create listening socket");
        freeaddrinfo(result);
        exit(1);
    }
    
    // Set listening socket to reuse same IP address
    int reuse_yes = 1;
    if((status = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_yes, sizeof(reuse_yes))) < 0){
        perror("Failed to set socket options");
        exit(1);
    }
    
    if((status = bind(listen_sockfd, result->ai_addr, result->ai_addrlen)) < 0){
        perror("Failed to bind socket to IP address");
        freeaddrinfo(result);
        exit(1);
    }

    freeaddrinfo(result);
    if((status = listen(listen_sockfd, BACKLOG)) < 0){
        perror("Failed to listen to incoming connections");
        exit(1);
    }
    
    cout << "Listening at port " << port_number << endl;
    return listen_sockfd;
}

/**
 * Waits and listens until first incoming connection is accepted.
 *
 *
 * @param listen_sockfd Listening socket to wait on 
 * @return Socket of the accepted connection between
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
 * Receives HTTP GET request message and sends out appropriate HTTP response.
 *
 * Appropriate HTTP responses:
 * Invalid HTTP Request --> 400 Response
 * File Not Found --------> 404 Response
 * File Found ------------> 200 Response
 *  
 * @param accept_sockfd Represents accepted connection with client 
 */
void handle_client(int accept_sockfd){
    char request_buf[2048]; // 
    int bytes_recv;

    if((bytes_recv = recv(accept_sockfd, request_buf, sizeof(request_buf), 0)) < 0){
        perror("Failed to receive data");
        close(accept_sockfd);
        exit(1);
    }
    

    // Convert request message frm char array to C++ string
    string request_msg(request_buf, bytes_recv);
    //cout << request_msg << "\n";
    
    string response_msg;
    if(!is_valid_get_request(request_msg)){
        //cout << "HTTP request NOT valid" << endl;
        response_msg = get_400_response();
    }
    else{
        //cout << "HTTP request IS valid" << endl;
        response_msg = get_400_response();
        string path = get_path(request_msg);
        cout << path << endl;
        if(!fs::exists(path)){
            response_msg = get_404_response();
        }
        //else{
        //    if(is_file()){

        //    }
        //    else{

        //    }
        //}
    }
    

    int bytes_sent = 0;
    int msg_size = response_msg.length();
    const char *response_msg_buf = response_msg.c_str();

    while(msg_size > 0){
        if((bytes_sent = send(accept_sockfd, response_msg_buf, msg_size, 0)) < 0){
            perror("Failed to send message");
            close(accept_sockfd);
            exit(1);
        }
        response_msg_buf += bytes_sent;
        msg_size -= bytes_sent;
        
    }
}

/**
 * Checks if the HTTP request message from the client is a valid GET request or not.
 * 
 * A valid HTTP GET request message is defined as follows:
 * 
 * GET | space(s) | URI | space(s) | HTTP/#.# | cr | lf |
 * Header name | : | space(s) | header value | cr | lf |
 * ...
 * Header name | : | space(s) | header value | cr | lf |
 * cr | lf |
 * 
 * --> space(s) can 1 or more whitespaces (\t or " ")
 * --> URI is not case sensitive; can have dashes (-), dots (.), and
 *  underscores (_); 
 * --> # can any digit from 0-9 (e.g. HTTP/1.1)
 * --> cr = carriage return (\r)
 * --> lf = line feed (\n)
 *
 * @param request_str The HTTP GET request message that needs to be verified 
 * @return If HTTP GET request message is valid, true.  Otherwise, false.
 */
bool is_valid_get_request(string request_str){
    std::smatch request_match;
    std::regex request_regex("GET\\s+/([a-zA-Z0-9_\\-\\.]+/?)*\\s+HTTP/[0-9]\\.[0-9]\r\n([a-zA-Z0-9\\-]+:(\\s)+.+\r\n)*\r\n",
            std::regex_constants::ECMAScript);

    return std::regex_match(request_str, request_match, request_regex);
}

/**
 *
 */
string get_path(string request_msg){
    int root_index = request_msg.find("/");
    int first_space_index = request_msg.find(" ", root_index);
    return request_msg.substr(root_index, first_space_index-root_index);
}

/**
 *
 *
 */
string get_400_response(){
    return "HTTP/1.0 400 BAD REQUEST\r\n\r\n"; 
}

string get_404_response(){
    string status_line = "HTTP/1.0 404 Not Found\r\n";
    string html_page = "<html><head><title>Oh no! Page not found</title></head>"
       "<body><p>404 Page Not Found!!!!!</p>"
       "<p>:^( :^( :^(</p></body></html>";
    string header_lines = "Content-Length: " + std::to_string(html_page.length())
        + "\r\nContent-Type: text/html\r\n\r\n";

    return status_line + header_lines + html_page;
}
