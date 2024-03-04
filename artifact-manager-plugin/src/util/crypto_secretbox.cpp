#include "util/crypto_secretbox.hpp"
#include "util/logger.hpp"

extern "C"{
    /*
    * Encrypts infile using AES-128 with key/nonce
    * Writes encrypted result out to outfile
    */
    void crypto_secretbox_encrypt_file(const char * infile, const char * outfile,
                        const unsigned char * key, const unsigned char * nonce)
    {
        Logger* logger = Logger::getInstance();
        logger->log(LogLevel::DBG, "crypto_secretbox_encrypt_file", "Begin");

        FILE *fpin;
        fpin = fopen(infile, "rb");
        fseek(fpin, 0, SEEK_END);
        size_t message_len = ftell(fpin);
        rewind(fpin);

        size_t mlen = message_len + crypto_secretbox_ZEROBYTES;
        unsigned char* m = (unsigned char*)calloc(mlen, sizeof(unsigned char));
        unsigned char* c = (unsigned char*)calloc(mlen, sizeof(unsigned char));

        if (fread(&m[crypto_secretbox_ZEROBYTES], 1, message_len, fpin) != message_len) {
            logger->log(LogLevel::ERR, "crypto_secretbox_encrypt_file", "fread failed to read bytes");
        }
        fclose(fpin);

        int tmp = crypto_secretbox(c, m, mlen, nonce, key);
        if(tmp != 0){
            logger->log(LogLevel::ERR, "crypto_secretbox_encrypt_file", "crypto_secretbox failed");
            exit(-1);
        }

        FILE *fpout;
        fpout = fopen(outfile, "w");
        size_t len = 0;
        do {
            len += fwrite(&c[len], 1, mlen - len, fpout);
        } while (len < mlen);
        fclose(fpout);
        free(c);
        free(m);
    }

    /*
    * Decrypts infile using AES-128 with key/nonce
    * Writes decrypted result out to outfile
    */
    void crypto_secretbox_decrypt_file(const char * infile, const char * outfile,
                        const unsigned char * key, const unsigned char * nonce)
    {
        Logger* logger = Logger::getInstance();
        logger->log(LogLevel::DBG, "crypto_secretbox_decrypt_file", "Begin");

        FILE *fpin;
        fpin = fopen(infile, "rb");
        fseek(fpin, 0, SEEK_END);
        size_t mlen = ftell(fpin);
        rewind(fpin);

        size_t message_len = mlen - crypto_secretbox_ZEROBYTES;
        unsigned char* m = (unsigned char*)calloc(mlen, sizeof(unsigned char));
        unsigned char* c = (unsigned char*)calloc(mlen, sizeof(unsigned char));

        if (fread(c, 1, mlen, fpin) != mlen) {
            logger->log(LogLevel::ERR, "crypto_secretbox_decrypt_file", "fread failed to read bytes");
        }
        fclose(fpin);

        int tmp = crypto_secretbox_open(m, c, mlen, nonce, key);
        if (tmp != 0) {
            logger->log(LogLevel::ERR, "crypto_secretbox_decrypt_file", "crypto_secretbox failed");
            exit(-1);
        }

        FILE *fpout;
        fpout = fopen(outfile, "wb");
        size_t len = 0;
        do {
            len += fwrite(&m[crypto_secretbox_ZEROBYTES + len], 1, message_len - len, fpout);
        } while (len < message_len);
        fclose(fpout);
        free(m);
        free(c);
    }

    void crypto_secretbox_decrypt_fd(const struct file_descriptor* infile, const struct file_descriptor* outfile,
                        const unsigned char * key, const unsigned char * nonce)
    {
        Logger* logger = Logger::getInstance();
        logger->log(LogLevel::DBG, "crypto_secretbox_decrypt_fd", "Begin");

        FILE *fpin;
        fpin = fdopen(dup(infile->fd), "rb");
        fseek(fpin, 0, SEEK_END);
        size_t mlen = ftell(fpin);
        rewind(fpin);

        size_t message_len = mlen - crypto_secretbox_ZEROBYTES;
        unsigned char* m = (unsigned char*)calloc(mlen, sizeof(unsigned char));
        unsigned char* c = (unsigned char*)calloc(mlen, sizeof(unsigned char));

        if (fread(c, 1, mlen, fpin) != mlen) {
            logger->log(LogLevel::ERR, "crypto_secretbox_decrypt_fd", "fread failed to read bytes");
        }
        fclose(fpin);

        int tmp = crypto_secretbox_open(m, c, mlen, nonce, key);
        if (tmp != 0) {
            logger->log(LogLevel::ERR, "crypto_secretbox_decrypt_fd", "crypto_secretbox failed");
            exit(-1);
        }

        FILE *fpout;
        fpout = fdopen(dup(outfile->fd), "wb");
        size_t len = 0;
        do {
            len += fwrite(&m[crypto_secretbox_ZEROBYTES + len], 1, message_len - len, fpout);
        } while (len < message_len);
        fclose(fpout);
        free(m);
        free(c);
    }
} // extern C
