#ifndef __REGISTRY_SSL_H__
#define __REGISTRY_SSL_H__

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void InitializeSSL();
void DestroySSL();
void ShutdownSSL(SSL* ssl_socket);


#endif