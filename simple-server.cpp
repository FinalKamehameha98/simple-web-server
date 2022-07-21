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
#include <stdexcept>

using std::cout;
using std::endl;

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
    return 0;
}
