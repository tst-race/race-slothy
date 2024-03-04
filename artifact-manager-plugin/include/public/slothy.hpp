#ifndef __SLOTHY_H__
#define __SLOTHY_H__

#include <pthread.h>
#include <assert.h>
#include <curl/curl.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <errno.h>
#include <json-c/json.h>
#include <nlohmann/json.hpp>
#include "download.hpp"
#include "util/file_descriptor.hpp"
#include "util/concat.hpp"

#include "util/base64.hpp"
#include "util/logger.hpp"
#include "util/profiler.hpp"

#include "util/crypto_secretbox.hpp"


class Slothy {
    public:
        Slothy(std::string registry_address);
        ~Slothy();

        std::string get_registry_address();
        void set_registry_address(std::string);

        // Main entry point, download artifact by name to specified location
        bool download_artifact(std::string artifact_name, std::string download_location);

        uint8_t* file_to_buffer(struct file_descriptor* f);
    protected:
        // Overriden in derivative classes to control metadata acquisition
        virtual nlohmann::json get_artifact_registry_info(std::string artifact_name){return nlohmann::json();};

        // Overriden in derivative classes to select a file downloading interface for downloading shards
        // and encrypted blobs
        virtual Downloader get_downloader(){return Downloader();};
        std::string registry_address;
};

#endif