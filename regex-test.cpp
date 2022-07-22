
#include <iostream>
#include <regex>
#include <string>

using std::cout;
using std::endl;
using std::string;

int main(){
    string http_string = "GET /somedir/some2/.git/phalanx HTTP/1.0\r\nConnection: active\r\n\r\n";

    std::regex http_request_regex("GET\\s+/([a-zA-Z0-9_\\-\\.]+/?)*\\s+HTTP/[0-9]\\.[0-9]\r\n([a-zA-Z0-9\\-]+:(\\s)+.+\r\n)*\r\n", 
            std::regex_constants::ECMAScript);
    std::smatch request_match;
    
    if(std::regex_match(http_string, request_match, http_request_regex)){
        cout << "Request was a match.\n";
        cout << "URI: " << request_match[1] << endl;
    }
    else{
        cout << "Request was NOT a match" << endl;
    }

    return 0;

}
