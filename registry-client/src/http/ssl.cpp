#include "http/ssl.h"


void InitializeSSL(){
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL(){
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL(SSL* ssl_socket){
    SSL_shutdown(ssl_socket);
    SSL_free(ssl_socket);
}
