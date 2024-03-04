#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <map>
#include "http/ssl.h"


class HTTPRequest {
    public:
        HTTPRequest(
            std::string _resource, 
            std::string _command, 
            std::map<std::string,  std::string> _headers,
            std::string body
        );
        const char* getValue(char* key);
        const char* getResource();
        const char* getCommand();
        const char* getBody();

        
        static HTTPRequest readRequest(SSL* ssl_socket);

    private:
        std::map<std::string,  std::string> headers;
        std::string command;
        std::string resource;
        std::string body;

};

#endif