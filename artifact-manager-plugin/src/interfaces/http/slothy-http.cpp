#include "slothy-http.hpp"
#include "stdlib.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

SlothyHTTP::SlothyHTTP(std::string registry_address) : Slothy(registry_address){}

nlohmann::json SlothyHTTP::get_artifact_registry_info(std::string artifact_name){
    Logger* logger = Logger::getInstance();
    struct file_descriptor* info_fd = Downloader::download(this->registry_address + "/" + artifact_name);  
    int file_size = lseek(info_fd->fd, 0L, SEEK_END);
    lseek(info_fd->fd, 0L, SEEK_SET); 
    char * data = (char *)calloc(file_size+1, sizeof(char));
    // read json into memory for parsing
    if (read(info_fd->fd, data, file_size*1) < 0) {
        logger->log(LogLevel::ERR, "SlothyHTTP::file_to_buffer", "Failed to read bytes");
    }
    std::string s( reinterpret_cast< char const* >(data), file_size ) ;
    nlohmann::json info = nlohmann::json::parse(s);
    return info;

}
