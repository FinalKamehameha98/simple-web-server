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
#include <fstream>
#include <thread>

#include "bounded_buffer.hpp"

// Declared namespaces
namespace fs = std::filesystem;

// using declarations
using std::cout;
using std::endl;
using std::string;

// Constants
static const int BACKLOG = 10;
static const int NUMBER_OF_WORKERS = 8;
static const int BUFFER_SIZE = 16;

// Forward declarations
int get_socket_and_listen(const char *port_number);
void handle_client(int accept_sockfd);
void accept_connections(int listen_sockfd);
bool is_valid_get_request(string request_msg);
string get_path(string request_msg);
void send_400_response(int accept_sockfd);
void send_404_response(int accept_sockfd);
void send_200_response(int accept_sockfd, string path);
void send_200_header(int accept_sockfd, string path);
void send_200_body(int accept_sockfd, string path);
void send_200_directory(int accept_sockfd, string path);
string generate_file_list(string path);
void send_data(int accept_sockfd, const char *data, size_t data_size);
void send_301_response(int accept_sockfd, string path);
void handle_connection(bounded_buffer &buffer);

/**
 * 
 *
 *
 * @param argc Number of arguments on command line (should be exactly two)
 * @param argv Command line arguments, specifically port number and directory
 * of where 
 */
int main(int argc, char *argv[]){
    if(argc != 3){
        cout << "Usage: " << argv[0] << " PORT_NUMBER DIRECTORY" << endl;
        exit(1);
    }

    int listen_sockfd = get_socket_and_listen(argv[1]);
    
    accept_connections(listen_sockfd);

    close(listen_sockfd);

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
    int enable_reuse = 1;
    if((status = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(enable_reuse))) < 0){
        perror("Failed to set socket options");
        freeaddrinfo(result);
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
 */
void accept_connections(int listen_sockfd){
    bounded_buffer connection_buff(BUFFER_SIZE);

    for(size_t i = 0; i < NUMBER_OF_WORKERS; i++){
        std::thread worker(handle_connection, std::ref(connection_buff));
        worker.detach();
    }

    while(true){
        struct sockaddr_storage their_addr; 
        socklen_t addr_size = sizeof(their_addr);
        int accept_sockfd;

        if((accept_sockfd = accept(listen_sockfd, (struct sockaddr *)&their_addr, &addr_size)) < 0){
            perror("Failed to accept incoming connection");
            close(listen_sockfd);
            exit(1);
        }

        connection_buff.put_item(accept_sockfd);
    }
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
    char request_buf[2048]; // 2 KB buffer 
    int bytes_recv;

    if((bytes_recv = recv(accept_sockfd, request_buf, sizeof(request_buf), 0)) < 0){
        perror("Failed to receive data");
        close(accept_sockfd);
        exit(1);
    }
    

    // Convert request message from char array to C++ string
    string request_msg(request_buf, bytes_recv);
    //cout << "Request\n" << request_msg << "\n";

    string response_msg;
    if(!is_valid_get_request(request_msg)){
        send_400_response(accept_sockfd);
    }
    else{
        string path = "WWW" + get_path(request_msg);
        if(!fs::exists(path)){
            send_404_response(accept_sockfd);
        }
        else{
            if(fs::is_regular_file(path)){
               send_200_response(accept_sockfd, path); 
            }
            else{
                if(path.back() != '/'){
                    path = path.substr(path.find("/")) + "/";
                    send_301_response(accept_sockfd, path);
                }
                else{
                    if(fs::is_regular_file(path + "index.html")){
                        send_200_response(accept_sockfd, path + "index.html");
                    }
                    else{
                        send_200_directory(accept_sockfd, path);
                    }
                }
            }
        }
    }

    close(accept_sockfd);
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
    int path_length = first_space_index - root_index;
    return request_msg.substr(root_index, path_length);
}

/**
 *
 */
void send_data(int accept_sockfd, const char *data, size_t data_size){
    int bytes_sent = 0;

    while(data_size > 0){
        if((bytes_sent = send(accept_sockfd, data, data_size, 0)) < 0){
            perror("Failed to send message");
            close(accept_sockfd);
            exit(1);
        }
        data += bytes_sent;
        data_size -= bytes_sent;
    }
}

void send_400_response(int accept_sockfd){
    string response = "HTTP/1.0 400 BAD REQUEST\r\n\r\n";
    send_data(accept_sockfd, response.c_str(), response.size());
}

void send_404_response(int accept_sockfd){
    string status_line = "HTTP/1.0 404 Not Found\r\n";
    string html_page = "<html><head><title>Oh no! Page not found!</title></head>"
       "<body><p>404 Page Not Found!!!!!</p>"
       "<p>:^( :^( :^(</p></body></html>";
    string header_lines = "Content-Length: " + std::to_string(html_page.length())
        + "\r\nContent-Type: text/html\r\n\r\n";
    string response = status_line + header_lines + html_page;
    send_data(accept_sockfd, response.c_str(), response.size());

}

void send_301_response(int accept_sockfd, string path){
    string status_line = "HTTP/1.0 301 Moved Permanently\r\n";
    string location = "Location: " + path + "\r\n\r\n";
    string response = status_line + location;

    send_data(accept_sockfd, response.c_str(), response.size());
}

void send_200_response(int accept_sockfd, string path){
    send_200_header(accept_sockfd, path);
    send_200_body(accept_sockfd, path);
}

void send_200_header(int accept_sockfd, string path){
    string status_line = "HTTP/1.0 200 OK\r\n";
    string content_length = "Content-Length: " + std::to_string(fs::file_size(path)) + "\r\n";
    
    string file_extension = path.substr(path.find("."));
    string content_type = "";
    
    if(file_extension == ".txt"){
        content_type = "text/plain";
    }
    else if(file_extension == ".pdf"){
        content_type = "application/pdf";
    }
    else if(file_extension == ".html"){
        content_type = "text/html";
    }
    else if(file_extension == ".css"){
        content_type = "text/css";
    }
    else if(file_extension == ".jpeg" || file_extension == ".jpg" || file_extension == ".jpe" 
            || file_extension == ".jfif" || file_extension == ".jif"){
        content_type = "image/jpeg";
    }
    else if(file_extension == ".png" || file_extension == ".PNG"){
        content_type = "image/png";
    }
    else if(file_extension == ".gif"){
        content_type = "image/gif";
    }
    content_type += "\r\n\r\n";
    string response = status_line + content_length + content_type;
    send_data(accept_sockfd, response.c_str(), response.size());
}

void send_200_body(int accept_sockfd, string path){
    std::ifstream file(path, std::ios::binary);
    const unsigned int buff_size = 4096; // 4 KB buffer
    char file_data[buff_size];

    while(!file.eof()){
        file.read(file_data, buff_size);
        int bytes_read = file.gcount();
        
        send_data(accept_sockfd, file_data, bytes_read);
    }
    file.close();
}

void send_200_directory(int accept_sockfd, string path){
    string status_line = "HTTP/1.0 200 OK\r\n";
    string content_type = "Content-Type: text/html\r\n";
    string html_page = generate_file_list(path);
    string response = status_line + content_type + "Content-Length: " + std::to_string(html_page.size())
        + "\r\n\r\n" + html_page;

    send_data(accept_sockfd, response.c_str(), response.size());
}

string generate_file_list(string path){
    string html_page = "<html><head><title>File Listing</title></head><body><ul>";
    
    for(auto& entry: fs::directory_iterator(path)){
        string filename = entry.path().filename();

        if(entry.is_directory()){
            html_page += "<li style=\"list-style: ':^) '\"><a href=\"" + filename  +"/\">" + filename + "/</a></li>";
        }
        else{
            html_page += "<li style=\"list-style: ':^) '\"><a href=\"" + filename  + "\">" + filename + "</a></li>";
        }
    }
    html_page += "</ul></body></html>";
    
    return html_page;
}

void handle_connection(bounded_buffer &buffer){
    while(true){
       handle_client(buffer.get_item()); 
    }
}
