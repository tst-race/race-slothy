#include "http/request.h"
#include "util/strlib.h"

#include <tuple>
#include <regex>
#include <algorithm>

HTTPRequest::HTTPRequest(
    std::string _resource, 
    std::string _command, 
    std::map<std::string, std::string> _headers,
    std::string _body

) : 
    resource(_resource),
    command(_command),
    headers(_headers),
    body(_body)
{}

const char* HTTPRequest::getValue(char* key){
    if(headers.count(key)){
        return headers[key].c_str();
    }else{
        return NULL;
    }
}

const char* HTTPRequest::getCommand(){
    return command.c_str();
}

const char* HTTPRequest::getResource(){
    return resource.c_str();
}
const char* HTTPRequest::getBody(){
    return body.c_str();
}

HTTPRequest HTTPRequest::readRequest(SSL* ssl_socket){
    size_t pos = 0;
    int buffer_size = BUFSIZ;

    // Read until \r\n\r\n delimiter between header and body
	char* sock_buff_raw = (char*) malloc(buffer_size);
	memset(sock_buff_raw, (int) '\0', buffer_size);
    std::string header = "";
    std::string body = "";
	int total_bytes_received = 0;
	// Receive until we have at least the entire header
	for(;;){
		// Read _one_at_a_time_, because we don't want to overshoot
		int bytes_received = SSL_read(ssl_socket, sock_buff_raw, buffer_size);
		total_bytes_received += bytes_received;
        std::string sock_buff_string = std::string(sock_buff_raw);

		if( !(bytes_received > 0) ){
			break;
		}
        header.append(sock_buff_string.substr(0,std::min(buffer_size,bytes_received)));	
        pos = header.find("\r\n\r\n");
        if(pos != -1){
            body.append(header.substr(pos+4));
            header = header.substr(0,pos);
			break;
		}
	}

    // Parse headers
    std::istringstream raw_stream(header);
    std::string line;
    std::string::size_type index;
    std::map< std::string,  std::string> headers;

    std::getline(raw_stream, line);
    std::string token;
    pos = line.find(" ");
    std::string command = line.substr(0, pos);
    line.erase(0, pos + 1);
    pos = line.find(" ");
    std::string resource = line.substr(0, pos);

    while (std::getline(raw_stream, line) && line != "\r") {
        index = line.find(':', 0);
        if(index != std::string::npos) {
            // Use regex to trim leading and trailing spaces 
            std::string key = line.substr(0, index);
            std::string value = line.substr(index + 1);
            

            headers.insert(std::make_pair(trim(key),trim(value)));
        }
    }

    // If there is a body, read it    
    if(headers.count("Content-Length")){
        int content_length = stoi(headers["Content-Length"]);
        content_length -= body.length();
        if(content_length > 0){
            total_bytes_received = 0;
            for(;;){
                // Read _one_at_a_time_, because we don't want to overshoot
                int bytes_received = SSL_read(ssl_socket, sock_buff_raw, buffer_size);
                total_bytes_received += bytes_received;
                std::string sock_buff_string = std::string(sock_buff_raw);

                if( !(bytes_received > 0) ){
                    break;
                }
                body.append(sock_buff_string.substr(0,std::min(buffer_size,bytes_received)));	

                if(total_bytes_received == content_length){
                    break;
                }
            }
        }
    }

    free(sock_buff_raw);

    return HTTPRequest(resource, command, headers, body);
}

