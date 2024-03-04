#include "slothy.hpp"
#include "util/logger.hpp"

// info on ifdefs for different architectures: https://sourceforge.net/p/predef/wiki/Architectures/
#ifdef __android__

int get_android_api_version(void) {
    FILE* file = popen("getprop ro.build.version.sdk", "r");
    if (file == NULL) {
        // if this doesnt work, it is prior to Lollipop (API 22/21, 20 is skipped)
        #ifdef DEBUG
        printf("get_android_api_version() returning `19`\n");
        #endif
        return 19;
    }
    // read the property value from file
    int i = 0;
    fscanf(file, "%d", &i);
    pclose(file);
    #ifdef DEBUG
    printf("get_android_api_version() returning: `%d`\n", i);
    #endif
    return i;
}

const char* get_package_name(void) {
    pid_t pid = getpid();
    static char buffer[255];
    FILE* f;
    char path[255];

    // Format the string "/proc/pid/cmdline"
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    // Read the contents of the file "/proc/pid/cmdline", get the command line parameters of the process
    f = fopen(path, "r");
    if (f == NULL) {
        return NULL;
    }

    // Read the first line string content of the file "/ proc / pid / cmdline" - the name of the process
    int result = fgets(buffer, sizeof(buffer), f);
    if (result == NULL) {
        fclose(f);
        return NULL;
    }
    // Turn the file
    fclose(f);
    #ifdef DEBUG
    printf("get_package_name() returning: `%s`\n", buffer);
    #endif
    return buffer;
}

struct file_descriptor* get_fd_from_template(char* file_template) {
    #ifdef DEBUG
    printf("called get_fd_from_template(%s)\n", file_template);
    #endif
    struct file_descriptor *f = (struct file_descriptor*) malloc(sizeof(struct file_descriptor));
    f->filename = NULL;

    f->fd = mkstemp(file_template);
    f->filename = file_template;

    // close and remove to create a file with execute access
    close(f->fd);
    remove(f->filename);
    f->fd = open(f->filename, O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

    if (f->fd <= 0) { //Something went wrong :(
        fprintf(stderr, "[-] 'open' failed to get file descriptor to %s with error: %s\n", f->filename, strerror(errno));
        exit(-1);
    }

    #ifdef DEBUG
    printf("get_fd_from_template(file_template) allocated file at: `%s`\n", f->filename);
    #endif

    return f;
}

// Returns a file descriptor where we can write our shared object
struct file_descriptor* get_fd_data_local_tmp(void) {
    char* file_template = calloc((16+12+1), 1);
    // file_template isn't freed because it is given to f->filename

    strncpy(file_template, "/data/local/tmp/", 16);
    strncpy(file_template + 16, "deleteXXXXXX", 12);

    struct file_descriptor* f = get_fd_from_template(file_template);

    return f;
}

// Returns a file descriptor where we can write our shared object
struct file_descriptor* get_fd_app_dir(void) {
    char* package_name = get_package_name();
    // char* package_name = "com.twosix.race";
    int package_name_len = strlen(package_name);

    char* file_template = calloc(package_name_len + 31, 1);
    // file_template isn't freed because it is given to f->filename

    // "/data/data/" = 11 characters
    strncpy(file_template, "/data/data/", 11);
    strncpy(file_template + 11, package_name, package_name_len);
    // "/files/deleteXXXXXX" = 19 characters
    strncpy(file_template + 11 + package_name_len, "/files/deleteXXXXXX", 19);

    struct file_descriptor* f = get_fd_from_template(file_template);

    return f;
}

// Returns a file descriptor where we can write our shared object
struct file_descriptor* get_fd_archdep(void) {
    struct file_descriptor *f = (struct file_descriptor*) malloc(sizeof(struct file_descriptor));
    if (f == NULL) {
        fprintf(stderr, "[-] 'malloc' failed to allocate file_descriptor\n");
        exit(-1);
    }
    f->filename = NULL;

    // If we have a kernel < 3.17, __NR_memfd_create syscall isn't supported in linux kernel
    // If we have android API < 30, __NR_memfd_create syscall isn't supported in android
    if (kernel_version() < 3.17 || get_android_api_version() < 30) {
        // f = get_fd_data_local_tmp();
        f = get_fd_app_dir();
    }
    // If we have a kernel >= 3.17
    // We can use the funky style
    else {
        #define FD_ARCHDEP_INMEMORY // defining this will make it so the file descriptors are not closed, since closing the file descriptor will lose the allocated memory.
        f->fd = syscall(__NR_memfd_create, SHM_NAME, 1);
        f->filename = (char*) malloc(sizeof(char)*MAX_FILEPATH_LENGTH); // TODO: fix hardcoded max path length
        snprintf(f->filename, MAX_FILEPATH_LENGTH, "/proc/%d/fd/%d", getpid(), f->fd);
        if (f->fd <= 0) { //Something went wrong :(
            fprintf(stderr, "[-] 'memfd_create' failed to get file descriptor to %s\n", f->filename);
            exit(-1);
        }
    }

    #ifdef DEBUG
    printf("get_fd_archdep(void) allocated file at: `%s`\n", f->filename);
    #endif

    return f;
}

#endif

#ifdef __mips__
#endif



// *******************************************
// *********PLATFORM INDEPENDENT CODE*********
// *******************************************


Slothy::Slothy(std::string registry_address) :
    registry_address(registry_address) 
{

    

}
Slothy::~Slothy(){

}

uint8_t* Slothy::file_to_buffer(struct file_descriptor* f) {
    Logger* logger = Logger::getInstance();

    // f->fd = open(f->filename, O_RDONLY, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    int file_size = lseek(f->fd, 0L, SEEK_END);
    lseek(f->fd, 0L, SEEK_SET);

   
    uint8_t* data = (uint8_t*)calloc(file_size+1, sizeof(char));

    // read json into memory for parsing
    if (read(f->fd, data, file_size*1) < 0) {
        logger->log(LogLevel::ERR, "Slothy::file_to_buffer", "Failed to read bytes");
    }

    close(f->fd);
    remove(f->filename); // remove file to leave no trace
    free(f->filename);
    free(f);
    return data;
    // TODO could write over memory before closing for extra security. here and in sss_reassemble, and when removing so files (use memset)
}

std::string Slothy::get_registry_address() {
    return registry_address;
}

void Slothy::set_registry_address(std::string new_reg_addr) {
    registry_address = new_reg_addr;
}



bool Slothy::download_artifact(std::string artifact_name, std::string download_location) {
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "SlothyRace::download_artifact", "Begin");    

    Profiler* p = Profiler::getInstance();

    
    p->beginProfilerSection("get_info_from_race_registry");

    logger->log(LogLevel::DBG, "SlothyRace::download_artifact", "calling get_artifact_registry_info");    
    nlohmann::json s = this->get_artifact_registry_info(artifact_name);
   
    p->endProfilerSection("get_info_from_race_registry");

    logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact info received from registry");    

    nlohmann::json meta;
    if(s.at("json_sharded") == true){
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Metadata is sharded");    

        std::vector<std::string> urls;
        s["json"].get_to(urls);

        int fd = Downloader::download_and_unshard(urls)->fd;
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Metadata retrieved and de-sharded");    

        // Quick length check
        int fileSize = lseek(fd, 0L, SEEK_END);
        char* contents = (char*) mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
        
        
        meta = nlohmann::json::parse(std::string(contents));
        munmap(contents, fileSize);
        close(fd);
    }else{
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Metadata is not sharded");    
    }

    std::string key_b64;
    meta["key"].get_to(key_b64);
    std::string nonce_b64;
    meta["nonce"].get_to(nonce_b64);


    std::vector<BYTE> key = base64_decode(key_b64);
    std::vector<BYTE> nonce = base64_decode(nonce_b64);

    // With the metadata in hand, we can now go acquire the actual artifact
    // using the secret and the nonce from the metadata to decrypt it
    if(s.at("so_sharded")){
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact is sharded");    
    }else{
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact is not sharded");    

        // SO is not sharded, can simply direct download the encypted blob
        std::vector<std::string> url;
        s["so"].get_to(url);
        
        p->beginProfilerSection("download_artifact_blob");
        struct file_descriptor* f_enc = Downloader::download(url[0]);
        p->endProfilerSection("download_artifact_blob");

        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact encrypted blob retrieved");    

        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "unrandomizing artifact");    
        unrandomize_header(f_enc);
        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "unrandomizing success");    

        // Proceed to decrypt the blob using key/nonce
        struct file_descriptor *f_dec;
        f_dec = get_fd_archdep();

        p->beginProfilerSection("decrypt_artifact_blob");
        // sss_decrypt_file(f->filename, f->filename, (unsigned char*)so_key->key, (unsigned char*)so_key->nonce);
        crypto_secretbox_decrypt_fd(f_enc, f_dec, key.data(), nonce.data());
        int fileSize = lseek(f_dec->fd, 0L, SEEK_END);
        char* contents = (char*) mmap(NULL, fileSize, PROT_READ, MAP_SHARED, f_dec->fd, 0);
        p->endProfilerSection("decrypt_artifact_blob");

        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact blob decrypted");    


        remove(f_enc->filename);
        free(f_enc->filename);
        close(f_enc->fd);
        free(f_enc);
        
        // Copy our decrypted file to a location
        p->beginProfilerSection("write_artifact_to_disk");

        int dst_fd = open(download_location.c_str(), O_CREAT | O_WRONLY);
        int err = write(dst_fd, contents, fileSize);
        if (err == -1) {
            logger->log(LogLevel::ERR, "SlothyRace::download_artifact", "Error writing blob to permanent file");    
        }
        
        close(dst_fd);
        p->endProfilerSection("write_artifact_to_disk");
        munmap(contents, fileSize);
        remove(f_dec->filename);
        free(f_dec->filename);
        close(f_dec->fd);
        free(f_dec);

        logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Artifact retrieved");    
        return true;
    }
    
    logger->log(LogLevel::ERR, "SlothyRace::download_artifact", "Artifact retrieval failed");    
    return false;
}

