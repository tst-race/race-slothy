#include "http/httpServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <errno.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <RaceLog.h>
const std::string LOG_PREFIX="SlothyHttpRegistry";

std::map<pthread_t, int> HttpServer::threadPool = std::map<pthread_t, int>();
std::mutex HttpServer::threadPoolLock = std::mutex();
int HttpServer::shouldStop = 0;
std::mutex HttpServer::shouldStopLock = std::mutex();
std::mutex HttpServer::allChildThreadsReaped = std::mutex();

const char* ERR404_header = "HTTP/1.1 404 Not Found\n";
const char* ERR404_body = "<html>\n<head>\n<title>404 - Not Found</title>\n</head>\n<body>\n<h1>404 - Not Found</h1>\n</body>\n</html>\n";

const char* OK200_header = "HTTP/1.1 200 OK\n";

const char* ERR400_header = "HTTP/1.1 400 Bad Request\n";
const char* ERR400_body = "<html>\n<head>\n<title>400 - Bad Request</title>\n</head>\n<body>\n<h1>400 - Bad Request</h1>\n</body>\n</html>\n";

const char* ERR500_header = "HTTP/1.1 500 Internal Server Error\n";
const char* ERR500_body = "<html>\n<head>\n<title>500 - Internal Server Error</title>\n</head>\n<body>\n<h1>404 - Not Found</h1>\n</body>\n</html>\n";

HttpServer::HttpServer(int port, RegistryDatabase* db){
        RaceLog::logInfo(LOG_PREFIX, "::constructor", "");
	mainSocket = (int*) malloc(sizeof(int));
	printf("[HTTP] Getting socket for port %d....", port);

	int get_socket_result = getSocket(mainSocket, port);
	if(get_socket_result == 0){
		printf("ok\n");

		HttpServer::mainThreadArgs* args = (HttpServer::mainThreadArgs*) malloc(sizeof(HttpServer::mainThreadArgs));
		args->listen_file_desc = *mainSocket;
		args->db = db;

		int threadCreateResult = pthread_create(&mainThreadHandle, NULL, run, (void*) args);
		if(threadCreateResult){
			printf("error: %d\n", threadCreateResult);
			exit(-1);
		}
	}else{
		printf("Error: %d\n", get_socket_result);
		exit(-1);
	}
};

void* HttpServer::run(void* args){
        RaceLog::logInfo(LOG_PREFIX, "::run", "");
	InitializeSSL();

	HttpServer::mainThreadArgs mta = *((HttpServer::mainThreadArgs*)args);
	free(args);
	int listen_file_desc = mta.listen_file_desc;
	RegistryDatabase* db = mta.db;

	struct sockaddr_in* clientaddr = (sockaddr_in*) malloc( sizeof( struct sockaddr_in ) );
	socklen_t* addrlen = (socklen_t*) malloc( sizeof( socklen_t ) );
	memset( clientaddr, 0, sizeof( struct sockaddr_in ) );
	memset( addrlen, 0, sizeof( socklen_t ) );

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(listen_file_desc, &read_fds);


	for(;;){
		int shouldStopTmp;
		shouldStopLock.lock();
		shouldStopTmp = shouldStop;
		shouldStopLock.unlock();
		if(shouldStopTmp){
			printf("[HTTP] Stopping, waiting for existing connections to close...");
			break;
		}else{
			int client_file_desc = accept(listen_file_desc, (struct sockaddr*) clientaddr, addrlen);

			clientThreadArgs* client_args = (clientThreadArgs*) malloc(sizeof(clientThreadArgs));
			client_args->client_file_desc = client_file_desc;
			client_args->db = db;		

			//printf("Client file desc is %d (errno: %d)\n", client_file_desc, errno);

			if(client_file_desc != -1){
				pthread_t threadHandle;
				int threadCreateResult = pthread_create(&threadHandle, NULL, handleClient, (void*) client_args);
				if(threadCreateResult != 0){
					printf( "[HTTP] Failed to create a child thread for connection!\n");
				}

				threadPoolLock.lock();
				threadPool[threadHandle] = 1;
				threadPoolLock.unlock();
			}else{
				//printf("File desc was %d\n", client_file_desc);
			}
		}

	}

	// Shut down
	close(listen_file_desc);
	free(addrlen);
	free(clientaddr);

	threadPoolLock.lock();
	std::vector<int> keys;
	for (const auto& [k, v] : threadPool) {
		keys.push_back(k);
	}
	for(pthread_t t : keys){
		printf(".");
		pthread_join(t, NULL);
		threadPool.erase(t);
	};
	printf("done\n");

	if(threadPool.size() != 0){
		printf("\n[HTTP] WARN: Failed to reap all child threads of HTTP server");
	}
	threadPoolLock.unlock();
	allChildThreadsReaped.unlock();		
	
	DestroySSL();
	printf("[HTTP] Web server has exited\n");
	return NULL;
}

HttpServer::~HttpServer(){
}

void HttpServer::kill(){

	// Set lock for reaping children thread here. We will attempt to lock this again further on.
	// The assumption is that by setting shouldStop, the listen thread will begin shutdown and reap
	// all children, after which it will unlock the mutex and we can procede with the rest of the shutdown
	allChildThreadsReaped.lock();
	shouldStopLock.lock();
	shouldStop = 1;
	shouldStopLock.unlock();

	// Shutdown the main listen socket, forcing a blocking accept() call to immediately return
	// with an error code.
	shutdown(*mainSocket, SHUT_RDWR);
	close(*mainSocket);
	free(mainSocket);

	// Continue once all children have been reaped
	allChildThreadsReaped.lock();
	pthread_join(mainThreadHandle, NULL);
}

int HttpServer::getSocket(int* listen_file_desc, int port){
    if(listen_file_desc == NULL){
        listen_file_desc = (int*) malloc(sizeof(int));
    }
	
    //Configure hints
    struct addrinfo* hints = (addrinfo*) malloc(sizeof(struct addrinfo));
        memset (hints, 0, sizeof( struct addrinfo));
        hints->ai_family = AF_INET;
        hints->ai_socktype = SOCK_STREAM;
	
    *listen_file_desc = socket(hints->ai_family, hints->ai_socktype, 0);
    free(hints);
    if(*listen_file_desc == -1){
        return -1;
    }

    int bind_status = -1;
    struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

	int iSetOption = 1;
	setsockopt(*listen_file_desc, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
        sizeof(iSetOption));

    bind_status = bind(*listen_file_desc, (struct sockaddr*) &address, sizeof(address));
    if(bind_status != 0){
        // Bind failed
        printf("\n[HTTP] Bind failed (%d)! Is something else running on that port?\n", bind_status);
        return -1;	
    }

	// Start listening with our file descriptor
    int listen_status = listen(*listen_file_desc, 100);
    if(listen_status != 0){
        return -1;
    }
	
    return 0;
}


void* HttpServer::handleClient(void *args){
        RaceLog::logInfo(LOG_PREFIX, "::handleClient", "");
	HttpServer::clientThreadArgs client_args = *((HttpServer::clientThreadArgs*)args);
	int client_file_desc = client_args.client_file_desc;
	RegistryDatabase* db = client_args.db;
	free(args);

	SSL_CTX *sslctx;
	sslctx = SSL_CTX_new(TLS_server_method());
	SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
	int use_cert = SSL_CTX_use_certificate_file(sslctx, "./server.pem" , SSL_FILETYPE_PEM);
	if(use_cert != 1){
		printf("[HTTP] Could not load cert file\n");
		return NULL;
	}
	int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "./server.key", SSL_FILETYPE_PEM);
	if(use_prv != 1){
		printf("[HTTP] Could not load private key file\n");
		return NULL;
	}

	SSL* ssl_socket = SSL_new(sslctx);
	SSL_set_fd(ssl_socket, client_file_desc);

	int ssl_err = SSL_accept(ssl_socket);
	if(ssl_err <= 0){
		//Error occurred, log and close down ssl
		ShutdownSSL(ssl_socket);
		return NULL;
	}else{
		HTTPRequest request = HTTPRequest::readRequest(ssl_socket);
	
		// Respond to the client	
		if( strcmp(request.getCommand(), "POST") == 0 ){
			// Stow the shard data to the registry and return 200 OK
			if(strcmp(request.getValue((char*) "Content-Type"), (char*) "application/json") == 0){
				auto body_json = json::parse(request.getBody());
				if(body_json.contains("name")){
					std::string name = body_json["name"];
					printf("Stored resource with name \"%s\"\n", name.c_str());
					printf("JSON is:\n----------------\n%s\n----------------\n", request.getBody());				
					db->storeShardEntry(name.c_str(), request.getBody());
					HttpServer::httpRespond(ssl_socket, OK200_header, NULL);	
				}else{
					HttpServer::httpRespond(ssl_socket, ERR404_header, ERR404_body);
				}
			}else{
				HttpServer::httpRespond(ssl_socket, ERR404_header, ERR404_body);
			}
		}else{
			// If we receive a GET request, possibly the slothy AMP plugin trying to get
			// metadata, we should check
			printf("Requested resqource: %s\n", request.getResource());
			std::string shard_name = (request.getResource()) + 1; // Slide the pointer over to drop leading slash
			int shard_exists = db->shardExists(shard_name.c_str());
			if(shard_exists == 1){
				printf("Shard exists\n");
				try{
					std::string shard_data = db->getShardData(shard_name.c_str());
					printf("Data is:\n%s\n", shard_data.c_str());

					HttpServer::httpRespond(ssl_socket, OK200_header, shard_data.c_str());
				}catch(std::exception &e){
					HttpServer::httpRespond(ssl_socket, ERR404_header, ERR404_body);
				}
			}else{
				HttpServer::httpRespond(ssl_socket, ERR404_header, ERR404_body);
			}
		}

		// Terminate SSL
		ShutdownSSL(ssl_socket);
	}
	
	// Deallocate the thread slot and close the file descriptor for the socket
	HttpServer::closeServerThread( pthread_self( ) );
	shutdown(client_file_desc, 2);
	close(client_file_desc);

	// Terminate the thread
	pthread_exit( NULL );
}



int HttpServer::httpRespond(SSL* ssl_socket, const char* header, const char* body){
        RaceLog::logInfo(LOG_PREFIX, "::httpRespond", "");
	const char* connection = "Connection: close\n";
	const char* content_type = "Content-Type: text/html\n";
	

	// Send everything on its way
	SSL_write(ssl_socket, header, strlen(header)); 
	SSL_write(ssl_socket, connection, strlen(connection)); 
	SSL_write(ssl_socket, content_type, strlen(content_type)); 


	if(body != NULL){
		// Get number of digits in the length of the body
		int chars_in_body_length = int(log10(strlen(body)) + 1);
		char* content_length = (char*) malloc(chars_in_body_length);
		// 16 chars in 'Content-Length: ' + number of digits in body length + null terminator
		snprintf(content_length, 17 + chars_in_body_length, "Content-Length: %d", strlen(body));	
		SSL_write(ssl_socket, content_length, strlen(content_length)); 
		SSL_write(ssl_socket, "\r\n\r\n", 4); 
		SSL_write(ssl_socket, body, strlen(body));	
		free(content_length);		
	}else{
		SSL_write(ssl_socket, "\r\n\r\n", 4); 
	}
	return 0;
}



int HttpServer::closeServerThread(pthread_t t){
    threadPoolLock.lock();
    threadPool.erase(t);
    threadPoolLock.unlock();

    return 0;
}
