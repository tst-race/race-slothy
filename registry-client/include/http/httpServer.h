#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <pthread.h>
#include <map>
#include <mutex> 
#include "http/ssl.h"
#include "http/request.h"
#include "db/registryDatabase.h"



class HttpServer {
public:
    HttpServer(int port, RegistryDatabase* db);
    ~HttpServer();
    void kill();

private:
    static  std::map<pthread_t, int>    threadPool; 
    static  std::mutex                  threadPoolLock;

    static  int                         shouldStop;
    static  std::mutex                  shouldStopLock;
    static  std::mutex                  allChildThreadsReaped;


    pthread_t mainThreadHandle;

    int* mainSocket = NULL;

    static int     getSocket(int* listen_file_desc, int port);
    static void*   run(void* args);
    static int     httpRespond(SSL* ssl_socket, const char* header, const char* body);
    static void*   handleClient(void* args);
    static int     closeServerThread(pthread_t tid);

    typedef struct mainThreadArgs {
        int listen_file_desc;
        RegistryDatabase* db;
    } mainThreadArgs;

    typedef struct clientThreadArgs{
        int client_file_desc;
        RegistryDatabase* db;
    } clientThreadArgs;

};
#endif