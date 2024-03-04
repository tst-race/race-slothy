#include <iostream>

#include "shard.hpp"

extern "C"{

void randomize_file(const char * infile) {

  Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::randomize_file", "Begin");

    struct file_descriptor * fd = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
    fd->filename = (char *)infile;
    fd->fd = open(fd->filename, O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    randomize_header(fd);

    free(fd);
}

void unrandomize_file(const char * infile) {


    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::unrandomize_file", "Begin");

    struct file_descriptor * fd = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
    fd->filename = (char *)infile;
    fd->fd = open(fd->filename, O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    unrandomize_header(fd);

    free(fd);
}

void encrypt_file (const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce) {
    crypto_secretbox_encrypt_file(infile, outfile, key, nonce);
}

void decrypt_file (const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce) {
    crypto_secretbox_decrypt_file(infile, outfile, key, nonce);
}

void shard_file(const char* filename, const char* shard_prefix, int threshold, int total_num_shares){

    Logger* logger = Logger::getInstance();


    logger->log(LogLevel::DBG, "Shard::shard_fd", "entering shard_fd()");
    logger->log(LogLevel::DBG, "Shard::shard_fd", "1. key K := keygen()");

    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char iv[crypto_secretbox_NONCEBYTES];

	/* Generate a random encryption key, iv */
	randombytes(key, sizeof(key));
    randombytes(iv, sizeof(iv));

    logger->log(LogLevel::DBG, "Shard::shard_fd", "2. sss_shares := calling sss on key K");

  	uint8_t data[sss_MLEN], restored[sss_MLEN];
	sss_Share shares[total_num_shares];
	size_t idx;
	int tmp;

	// store key, iv in data to be shared
	memcpy(data, key, sizeof(key));
    memcpy(data + sizeof(key), iv, sizeof(iv));


    logger->log(LogLevel::DBG, "Shard::shard_fd", "calling sss_create_shares()");

	// Split the secret into 16 shares (with a recombination theshold of 8)
	sss_create_shares(shares, data, total_num_shares, threshold);

   logger->log(LogLevel::DBG, "Shard::shard_fd", "3. F_enc := calling encrypt on file F with key K");

    struct file_descriptor * f = get_fd_archdep(); // Give me a file descriptor to memory
    if (f == NULL) {
        logger->log(LogLevel::ERR, "Shard::", "get_fd_archdep failed in shard.");
        exit(1);
    }


    crypto_secretbox_encrypt_file(filename, f->filename, key, iv);

    logger->log(LogLevel::DBG, "Shard::shard_fd", "4. rs_shard := calling rs_shard on F_enc");

    rs_shard_file(f->filename, shard_prefix, threshold, total_num_shares);

   logger->log(LogLevel::DBG, "Shard::shard_fd", "5. concat_share := calling concat on sss_shares + rs_shares");

    // we know filenames of the rs_shard_file output
    // we need pre-pend the sss_share to each rs_shard_file output

   logger->log(LogLevel::DBG, "Shard::shard_fd", "6. random_shares := calling randomize on concat_shares");

    // we need to randomize header on each shard_file_prefix output


    size_t max_len_of_filename = strlen(shard_prefix) + 8;
    for (size_t i = 0; i < total_num_shares; i++)
    {
        struct file_descriptor * shard = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
        shard->filename = (char *)calloc(max_len_of_filename, sizeof(char));
        snprintf(shard->filename, max_len_of_filename, "%s%lu", shard_prefix, i);

        FILE * fp = fopen(shard->filename, "rb");
        unsigned char * data;
        int pre_file_size;
        fseek(fp, 0, SEEK_SET);
        int rc;
        if (fp == NULL)
        {
            logger->log(LogLevel::ERR, "Shard::shard_fd - concat/randomize", "Unable to open file");
            return;
        }
        // get file size
        fseek(fp, 0L, SEEK_END);
        pre_file_size = ftell(fp);
        fclose(fp);
        data = (unsigned char *) malloc (pre_file_size);

        fp = fopen(shard->filename, "r+");
        fseek(fp, 0, SEEK_SET);
        fread(data, 1, pre_file_size, fp);
        fclose(fp);

        fp = fopen(shard->filename, "w");
        fseek(fp, 0, SEEK_SET);
        int bytes_written = fwrite(shares[i], sizeof(unsigned char), sizeof(shares[i]), fp);
        bytes_written += fwrite(data, sizeof(unsigned char), pre_file_size, fp);
        ftruncate(fileno(fp), sizeof(shares[i]) + pre_file_size);
        fclose(fp);                

        randomize_header(shard);

        free(data);
        free(shard->filename);
        free(shard);
    }

    return; 
}



void unshard_fd(struct file_descriptor* shards[], unsigned int num_shards, struct file_descriptor* output_fd, unsigned int threshold, unsigned int num_shards_total){

  	uint8_t restored[sss_MLEN];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char iv[crypto_secretbox_NONCEBYTES];

    Logger* logger = Logger::getInstance();

    logger->log(LogLevel::DBG, "Shard::unshard_fd", "entering unshard_fd()");

    sss_Share sss_shares[num_shards];

   logger->log(LogLevel::DBG, "Shard::unshard_fd", "1. unrandomize()");
   logger->log(LogLevel::DBG, "Shard::unshard_fd", "2. split shares into sss shares and encrypted rs shares");
   
    for (size_t i = 0; i < num_shards; i++)
    {
        // unrandomize
        unrandomize_header(shards[i]);
        logger->log(LogLevel::DBG, "Shard::unshard_fd", "splitting shares");

        // remove first 64 bytes of shard[i]->filename and store in sss_shares[i]
        
        // get filename file length
        FILE * fp = fopen(shards[i]->filename, "rb");
        unsigned char * data;
        int pre_file_size;
        fseek(fp, 0, SEEK_SET);
        int rc;
        if (fp == NULL)
        {
            logger->log(LogLevel::ERR, "Shard::unshard_fd - split/unrandomize", "Unable to open file");
            return;
        }
        // get file size
        fseek(fp, 0L, SEEK_END);
        pre_file_size = ftell(fp);
        fclose(fp);
        data = (unsigned char *) malloc (pre_file_size);

        fp = fopen(shards[i]->filename, "r+");
        fseek(fp, 0, SEEK_SET);
        fread(data, 1, pre_file_size, fp);
        fclose(fp);

        memcpy(sss_shares[i], data, sizeof(sss_shares[i]));

        fp = fopen(shards[i]->filename, "w");
        fseek(fp, 0, SEEK_SET);
        int bytes_written = fwrite(data + sizeof(sss_shares[i]), sizeof(unsigned char), pre_file_size - sizeof(sss_shares[i]), fp);
        ftruncate(fileno(fp), pre_file_size - sizeof(sss_shares[i]));
        fclose(fp);                

        free(data);
        
    }

    logger->log(LogLevel::DBG, "Shard::unshard_fd", "3. unshard encrypted rs shares to get F_enc");

    struct file_descriptor * f = get_fd_archdep(); // Give me a file descriptor to memory
    if (f == NULL) {
        logger->log(LogLevel::ERR, "UnShard::", "get_fd_archdep failed in shard.");
        exit(1);
    }

    rs_unshard_fd(shards, num_shards, f, threshold, num_shards_total);

    logger->log(LogLevel::DBG, "Shard::unshard_fd", "4. combine sss to get key K and IV");

	// Combine some of the shares to restore the original secret
	int tmp = sss_combine_shares(restored, sss_shares, num_shards);
	assert(tmp == 0);

    logger->log(LogLevel::DBG, "Shard::unshard_fd", "5. decrypt F_enc with K, IV to reveal F");
  
    memcpy(key, restored, crypto_secretbox_KEYBYTES);
    memcpy(iv, restored + sizeof(key), crypto_secretbox_NONCEBYTES);
    
    crypto_secretbox_decrypt_file(f->filename, output_fd->filename, key, iv);

    logger->log(LogLevel::DBG, "Shard::unshard_fd", "decrypt_file success");

    return;
}

void unshard_file(const char * shard_prefix, const char * outfile, int threshold, int num_shares_total) {
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::unshard_file", "Begin");

    struct file_descriptor *shards[num_shares_total];
    struct file_descriptor ** shards_final;

    size_t max_len_of_filename = strlen(shard_prefix) + 8;
    int num_files = 0;

    for (size_t i = 0; i < num_shares_total; i++)
    {
        shards[i] = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
        shards[i]->filename = (char *)calloc(max_len_of_filename, sizeof(char));
        snprintf(shards[i]->filename, max_len_of_filename, "%s%lu", shard_prefix, i);
        shards[i]->fd = open(shards[i]->filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
        if (shards[i]->fd != -1)
            num_files++;
    }

    shards_final = (struct file_descriptor **) malloc (sizeof(struct file_descriptor *)* num_files);

    int s_counter = 0;
    for (size_t i = 0 ; i < num_shares_total; i++) {
        if (shards[i]->fd != -1) {
            shards_final[s_counter] = shards[i];
            s_counter++;
        }
    }

    struct file_descriptor *output_fd = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
    output_fd->filename = (char *)outfile;
    output_fd->fd = open(output_fd->filename, O_CREAT | O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    unshard_fd(shards_final, num_files, output_fd, threshold, num_shares_total);

    close(output_fd->fd);
    free(output_fd);

    // cleanup here

}

}