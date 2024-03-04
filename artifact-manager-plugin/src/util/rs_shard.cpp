#include "util/rs_shard.hpp"

#include <errno.h>
#include "util/hex.hpp"
#include "util/logger.hpp"
#include <string>

/* Helper function for shard_fd that reads in <filename> and generates <num_shares>
 * reed solomon shards where you need <threshold> to reconstruct. The shards are named
 * in the form <shard_prefix><shard number>
 */
int rs_shard_file(const char *filename, const char *shard_prefix, int threshold, int num_shares)
{
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::rs_shard_file", "Begin");

    struct file_descriptor *shards[num_shares];
    size_t max_len_of_filename = strlen(shard_prefix) + 8;
    for (size_t i = 0; i < num_shares; i++)
    {
        shards[i] = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
        shards[i]->filename = (char *)calloc(max_len_of_filename, sizeof(char));
        snprintf(shards[i]->filename, max_len_of_filename, "%s%lu", shard_prefix, i);
        // shards[i]->fd = open(shards[i]->filename, O_RDONLY, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    }

    struct file_descriptor *input_fd = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
    input_fd->filename = (char *)filename;
    input_fd->fd = open(input_fd->filename, O_CREAT | O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    int ret = rs_shard_fd(input_fd, shards, threshold, num_shares);

    close(input_fd->fd);
    free(input_fd);

    for (size_t i = 0; i < num_shares; i++)
    {
        free(shards[i]->filename);
        // close(shards[i]->fd);
        free(shards[i]);
    }
    return ret;
}

/* Helper function for unshard_fd that reads in shards and reconstructs using <threshold>
 * out of <num_shares> of the reed solomon shards. The shards are named  in the form
 * <shard_prefix><shard number>
 */
int rs_unshard_file(const char *shard_prefix, const char *outfile, int threshold, int num_shares){
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::rs_unshard_file", "Begin");

    struct file_descriptor *shards[num_shares];
    size_t max_len_of_filename = strlen(shard_prefix) + 8;
    size_t num_files = 0;
    for (size_t i = 0; i < num_shares; i++)
    {
        shards[i] = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
        shards[i]->filename = (char *)calloc(max_len_of_filename, sizeof(char));
        snprintf(shards[i]->filename, max_len_of_filename, "%s%lu", shard_prefix, i);
        shards[i]->fd = open(shards[i]->filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
        if (shards[i]->fd != -1)
            num_files++;
    }

    struct file_descriptor *output_fd = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
    output_fd->filename = (char *)outfile;
    output_fd->fd = open(output_fd->filename, O_CREAT | O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    int ret = rs_unshard_fd(shards, num_files, output_fd, threshold, num_shares);

    close(output_fd->fd);
    free(output_fd);

    return ret;
}

/* Shards the metadata using hardcoded constants so that it can be reconstructed
 * without having any metadata of its own
 */
int rs_shard_meta(struct file_descriptor *fds[], char *buf, int len, int k, int m, int w)
{
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::rs_shard_meta", "Begin");
    long int file_size;
    int bytes_read;
    FILE *file;

    FILE *fp, *fp2; // file pointers
    int libec_desc;
    uint64_t fragment_len;
    int ret = 0;
    char *block; // padding file
    int size, newsize;
    int n, readins;

    int buffersize;
    int i;
    int blocksize;
    int total;
    int extra;

    char *file_data;
    /* Jerasure Arguments */
    char **data;
    char **coding;
    int *matrix;
    int *bitmatrix;

    /* Creation of file name variables */
    char temp[5];
    char *s1, *s2, *extension;
    char *fname;
    int md;
    char *curdir;
    int file_pointer;

    file_size = len;

    file_data = buf;
    bytes_read = len;

    /* Determine size of k+m files */
    blocksize = bytes_read / k;
    newsize = bytes_read;
    size = newsize;
    buffersize = 0;

    /* Allow for buffersize and determine number of read-ins */
    if (size > buffersize && buffersize != 0)
    {
        if (newsize % buffersize != 0)
        {
            readins = newsize / buffersize;
        }
        else
        {
            readins = newsize / buffersize;
        }
        block = (char *)malloc(sizeof(char) * buffersize);
        blocksize = buffersize / k;
    }
    else
    {
        readins = 1;
        buffersize = size;
        block = (char *)malloc(sizeof(char) * newsize);
    }

    /* Allocate data and coding */
    data = (char **)malloc(sizeof(char *) * k);
    coding = (char **)malloc(sizeof(char *) * m);
    for (i = 0; i < m; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * blocksize);
        if (coding[i] == NULL)
        {
            return -1;
        }
    }

    matrix = reed_sol_vandermonde_coding_matrix(k, m, w);

    /* Read in data until finished */
    n = 1;
    total = 0;
    file_pointer = 0;

    while (n <= readins)
    {
        // Check if padding is needed, if so, add appropriate
        // number of zeros
        if (total < size && total + buffersize <= size)
        {
            // read buffersize chars into block from fp
            // read from file_data
            memcpy(block, file_data + file_pointer, buffersize);
            total += buffersize;
            file_pointer += buffersize;
            // total += fread(block, sizeof(char), buffersize, fp);
        }
        else if (total < size && total + buffersize > size)
        {
            // read rest of file_data into block
            extra = size - total;
            memcpy(block, file_data + file_pointer, extra);
            // extra = fread(block, sizeof(char), buffersize, fp);
            for (i = extra; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }
        else if (total == size)
        {
            for (i = 0; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }

        /* Set pointers to point to file data */
        for (i = 0; i < k; i++)
        {
            data[i] = block + (i * blocksize);
        }

        jerasure_matrix_encode(k, m, w, matrix, data, coding, blocksize);

        for (i = 1; i <= m; i++)
        {
            if (fp == NULL)
            {
                memset(data[i - 1], 0, blocksize);
            }
            else
            {
                // fname = fds[i]->filename;
                if (n == 1)
                {
                    fp2 = fopen(fds[i - 1]->filename, "wb");
                    // write 16-bit index to front of share
                    fputc((char)(i >> 8) & 0xff, fp2);
                    fputc((char)i & 0xff, fp2);
                }
                else
                {
                    fp2 = fopen(fds[i - 1]->filename, "ab");
                }
                fwrite(coding[i - 1], sizeof(char), blocksize, fp2);
                fclose(fp2);
            }
        }
        n++;
    }

    for (i = 0; i < m; i++)
    {
        free(coding[i]);
    }
    free(coding);
    free(data);
    free(block);
    return 1;
}

/* Uses reed solomon encoding to break filename into <num_shares> number of shards where
 * <threshold> are required for recovery.
 * We throw out all of the plaintext blocks so that no blocks of contiguous file can be recovered.
 * Generates the metadata in memory and writes it as a header followed by encoded data to the
 * filenames that are part of fds[].
 */
int rs_shard_fd(const struct file_descriptor *output_fd, struct file_descriptor *fds[], int threshold, int num_shares)
{
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::rs_shard_fd", "Begin");

    long int file_size;
    int bytes_read;
    FILE *file;

    FILE *fp, *fp2; // file pointers
    int libec_desc;
    uint64_t fragment_len;
    int ret = 0;
    char *block; // padding file
    int size, newsize;
    int n, readins;

    int k, m, w;
    int buffersize;
    int i;
    int blocksize;
    int total;
    int extra;

    char *file_data;
    /* Jerasure Arguments */
    char **data;
    char **coding;
    int *matrix;
    int *bitmatrix;

    /* Creation of file name variables */
    char temp[5];
    //char *s1, *s2, *extension;
    char *fname;
    int md;
    char *curdir;
    int padding_size;

    k = threshold;  // num data files
    m = num_shares; // num parity files

    w = RS_WORD_SIZE;

    // read size of file
    file = fdopen(dup(output_fd->fd), "rb");
    if (file == NULL)
    {
        logger->log(LogLevel::ERR, "RSShard::rs_shard_fd", "Unable to open file (" + std::string(output_fd->filename) + ")");
        return -1;
    }
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    fclose(file);

    logger->log(LogLevel::DBG, "RSShard::rs_shard_fd", "File size is " + std::to_string(file_size));

    /* Determine size of k+m files */
    bytes_read = file_size;
    blocksize = bytes_read / k;
    newsize = bytes_read;
    padding_size = 0;

    // add padding
    if (newsize % 128 == 0)
    {
        padding_size = 0;
    }
    else
    {
        padding_size = 128 - newsize % 128;
        newsize += padding_size;
    }
    blocksize = newsize / k;

    size = newsize;
    buffersize = 0;

    /* Allow for buffersize and determine number of read-ins */
    if (size > buffersize && buffersize != 0)
    {
        if (newsize % buffersize != 0)
        {
            readins = newsize / buffersize;
        }
        else
        {
            readins = newsize / buffersize;
        }
        block = (char *)malloc(sizeof(char) * buffersize);
        blocksize = buffersize / k;
    }
    else
    {
        readins = 1;
        buffersize = size;
        block = (char *)malloc(sizeof(char) * newsize);
    }

    /* Allocate data and coding */
    data = (char **)malloc(sizeof(char *) * k);
    coding = (char **)malloc(sizeof(char *) * m);
    for (i = 0; i < m; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * blocksize);
        if (coding[i] == NULL)
        {
            //perror("malloc");
            return -1;
        }
    }

    /* Create coding matrix */
    matrix = reed_sol_vandermonde_coding_matrix(k, m, w);

    /* Read in data until finished */
    n = 1;
    total = 0;
    fp = fdopen(dup(output_fd->fd), "rb");
    fseek(fp, 0L, SEEK_SET);
    if (fp == NULL)
    {
        logger->log(LogLevel::ERR, "RSShard::rs_shard_fd", "Unable to open file (" + std::string(output_fd->filename) + ")");
        return -1;
    }

    // create metadata file in memory
    FILE *stream;
    char *buf;
    size_t len;

    stream = open_memstream(&buf, &len);
    fprintf(stream, "%d %d %d\n", size, blocksize, size - padding_size);
    fprintf(stream, "%d %d %d %d\n", k, m, w, buffersize);
    fprintf(stream, "%d\n", readins);
    fflush(stream);

    while (len != 1024)
    {
        fprintf(stream, "%c", '\0');
        fflush(stream);
    }

    rs_shard_meta(fds, buf, len, k, m, w);

    unsigned char *hashes[num_shares];
    unsigned char all_hashes[num_shares * MD5_DIGEST_LENGTH];
    MD5_CTX shard_context[num_shares];
    for (i = 0; i < num_shares; i++)
    {
        hashes[i] = (unsigned char *)calloc(MD5_DIGEST_LENGTH, sizeof(char));
        MD5_Init(&shard_context[i]);
        fds[i]->fd = open(fds[i]->filename, O_RDWR | O_APPEND, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

    while (n <= readins)
    {
        /* Check if padding is needed, if so, add appropriate
           number of zeros */
        if (total < size && total + buffersize <= size)
        {
            total += fread(block, sizeof(char), buffersize, fp);
        }
        else if (total < size && total + buffersize > size)
        {
            extra = fread(block, sizeof(char), buffersize, fp);
            for (i = extra; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }
        else if (total == size)
        {
            for (i = 0; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }

        /* Set pointers to point to file data */
        for (i = 0; i < k; i++)
        {
            data[i] = block + (i * blocksize);
        }

        jerasure_matrix_encode(k, m, w, matrix, data, coding, blocksize);

        for (i = 1; i <= m; i++)
        {
            if (fp == NULL)
            {
                memset(data[i - 1], 0, blocksize);
            }
            else
            {
                // if (n == 1) {
                //     fp2 = fopen(fds[i-1]->filename, "ab");
                // }
                // else {
                //     fp2 = fopen(fds[i-1]->filename, "ab");
                // }

                // fwrite(coding[i-1], sizeof(char), blocksize, fp2);
                write(fds[i - 1]->fd, coding[i - 1], sizeof(char) * blocksize);
                MD5_Update(&shard_context[i - 1], coding[i - 1], blocksize);
                // fclose(fp2);
            }
        }
        n++;
    }

    for (i = 0; i < num_shares; i++)
    {
        MD5_Final(hashes[i], &shard_context[i]);
        memcpy(&all_hashes[i * MD5_DIGEST_LENGTH], hashes[i], MD5_DIGEST_LENGTH);
    }

    for (i = 0; i < m; i++)
    {
        // fp2 = fopen(fds[i]->filename, "ab");
        // fwrite(all_hashes, num_shares, MD5_DIGEST_LENGTH, fp2);
        // fclose(fp2);
        
        write(fds[i]->fd, all_hashes, num_shares * MD5_DIGEST_LENGTH);
        close(fds[i]->fd);  
       
        free(coding[i]);
        
    }

    free(coding);
    free(data);
    free(block);
    return 1;
}

/* Validates shards using hashes
 * 1) Make sure all shards agree on hash values
 * 2) Read and hash the body of each shard
 * 3) Assert shard hash == known hash
 * 4) Throw out shards that show corruption
 */
// void validate_shards(struct file_descriptor* fds[], int num_fds, int threshold, int num_shares, int * new_num_fds) {
int validate_shards(struct file_descriptor *fds[], int num_fds, int threshold, int num_shares){
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::validate_shards", "Begin");

    int i, j, d, x, b;
    int bytesRead = 0;
    int hash_len = MD5_DIGEST_LENGTH * num_shares;
    unsigned char cur_file_hash[(MD5_DIGEST_LENGTH)];
    char cur_hash_val[(hash_len)];
    char true_hash_val[(hash_len)];
    char all_file_hashes[(hash_len)*num_fds];
    char test_hashes;
    char *shard_buffer;
    FILE *fp;
    FILE **fps; // File pointer
    long file_size;
    char meta_buffer[RS_META_BUFFERSIZE];
    unsigned char hi, low;
    item *items;
    int num_items = 0;
    int var = 0;
    int blocksize = 0; // size of individual files
    int origsize;      // size of file before padding
    int k, m, w, n, buffersize, readins, pre_padded, shard_index;
    MD5_CTX shard_context;
    char incorrect_files[(hash_len)*num_fds];
    int num_bad_files = 0;
    int num_good_files = 0;
    file_info *bad_files;

    struct file_descriptor *good_fds[num_fds];

    fps = (FILE **)calloc(sizeof(FILE *), num_fds);
    items = (item *)calloc(sizeof(item), num_fds);
    bad_files = (file_info *)calloc(sizeof(file_info), num_fds);
    // good_files = (file_info *) calloc (sizeof(file_info), num_fds);

    for (i = 0; i < num_fds; i++)
    {
        if (fds[i] == NULL)
        {
            // #ifdef DEBUG
            // fprintf(stderr, "validate_shards: fds[%d] == NULL\n", i);
            // #endif
            continue;
        }

        fp = fdopen(dup(fds[i]->fd), "rb");
        if (fp == NULL)
        {
            // #ifdef DEBUG
            // fprintf(stderr, "validate_shards: Could not open input file (%s)\n", fds[i]->filename);
            // #endif
            continue;
        }

        fseek(fp, 0L, SEEK_END);
        file_size = ftell(fp);

        if (file_size < hash_len)
        {
            // #ifdef DEBUG
            // fprintf(stderr, "validate_shards: File (%s) too small\n", fds[i]->filename);
            // #endif
            // fclose(fp);
        }
        else
        {
            fseek(fp, -hash_len, SEEK_END);
            bytesRead = fread(cur_hash_val, 1, hash_len, fp);
            var = 0;
            while (var < num_items)
            {
                if (items[var].key == cur_hash_val)
                {
                    items[var].value++;
                    break;
                }
                var++;
            }

            if (var == num_items)
            {
                items[var].key = cur_hash_val;
                items[var].value = 1;
                num_items++;
            }
        }
        fclose(fp);
    }

    item winner = items[0];

    for (var = 1; var < num_items; var++)
    {
        if (items[var].value > winner.value)
        {
            winner = items[var];
        }
    }

    if (winner.key != NULL)
    {
        // print_hex(winner.key, hash_len);
        memcpy(&true_hash_val, winner.key, hash_len);
        // print_hex(true_hash_val, hash_len);
    }

    // *************************************************

    for (j = 0; j < num_fds; j++)
    {
        if (fds[j] == NULL)
        {
            bad_files[num_bad_files].key = j;
            bad_files[num_bad_files].file = fds[j];
            num_bad_files++;
            logger->log(LogLevel::ERR, "RSShard::validate_shards", "fds[" + std::to_string(j) + "] == NULL");
            continue;
        }

        fps[j] = fdopen(dup(fds[j]->fd), "rb");
        if (fps[j] == NULL)
        {
            bad_files[num_bad_files].key = j;
            bad_files[num_bad_files].file = fds[j];
            num_bad_files++;
            logger->log(LogLevel::ERR, "RSShard::validate_shards", "Could not open input file: " + std::string(fds[j]->filename));
            continue;
        }

        fseek(fps[j], 0L, SEEK_END);
        file_size = ftell(fps[j]);
        fseek(fps[j], 0L, SEEK_SET);

        if (file_size < hash_len)
        {
            bad_files[num_bad_files].key = j;
            bad_files[num_bad_files].file = fds[j];
            num_bad_files++;
            logger->log(LogLevel::ERR, "RSShard::validate_shards", "File (" + std::string(fds[j]->filename) + ") is too small");
            continue;
        }

        low = fgetc(fps[j]);
        hi = fgetc(fps[j]);

        shard_index = (low << 8) | hi;
        fread(&meta_buffer, 1, RS_META_BUFFERSIZE, fps[j]);

        sscanf(meta_buffer, "%d %d %d\n%d %d %d %d\n%d", &origsize, &blocksize, &pre_padded, &k, &m, &w, &buffersize, &readins);

        // Skip past shard number in header
        fseek(fps[j], (long int)(2 + 128), SEEK_SET);
        shard_buffer = (char *)calloc(sizeof(char), blocksize);

        n = 0;
        MD5_Init(&shard_context);

        while (n < readins)
        {
            int fread_len = fread(shard_buffer, sizeof(char), blocksize, fps[j]);
            // int fread_len = read(fds[j]->fd, shard_buffer, sizeof(char)*blocksize);
            if (fread_len != blocksize)
            {
                logger->log(
                    LogLevel::ERR, 
                    "RSShard::validate_shards", 
                    "fread_len (" + std::to_string(fread_len) + ") != blocksize (" + std::to_string(blocksize) + ")"
                );
            }
            // print_hex(shard_buffer, blocksize);

            MD5_Update(&shard_context, shard_buffer, blocksize);
            n++;
        }

        MD5_Final((unsigned char *)&cur_file_hash, &shard_context);

        if (memcmp(&cur_file_hash, &true_hash_val[MD5_DIGEST_LENGTH * (shard_index - 1)], MD5_DIGEST_LENGTH) == 0)
        {
            // print_hex(cur_file_hash, MD5_DIGEST_LENGTH);
            good_fds[num_good_files] = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
            good_fds[num_good_files]->filename = fds[j]->filename;
            good_fds[num_good_files]->fd = fds[j]->fd;
            num_good_files++;
        }
        else
        {
            bad_files[num_bad_files].key = j;
            bad_files[num_bad_files].file = fds[j];
            num_bad_files++;
            // print_hex(cur_file_hash, MD5_DIGEST_LENGTH);
            if (j == (num_fds - 1))
            {
                good_fds[num_good_files] = (struct file_descriptor *)calloc(1, sizeof(struct file_descriptor));
                good_fds[num_good_files]->filename = fds[j]->filename;
                good_fds[num_good_files]->fd = fds[j]->fd;
            }
        }

        fclose(fps[j]);
    }

    if (num_bad_files > 0)
    {
        for (x = 0; x < num_bad_files; x++)
        {
            // free bad files
            if (bad_files[x].file != NULL)
            {
                remove(bad_files[x].file->filename); // remove file to leave no trace
                free(bad_files[x].file->filename);
                close(bad_files[x].file->fd);
                free(bad_files[x].file);
            }
        }
        for (x = 0; x < num_fds; x++)
        {
            if (x < num_good_files)
            {
                memcpy(&fds[x], &good_fds[x], sizeof(good_fds[x]));
            }
            else
            {
                memset(&fds[x], 0, sizeof(fds[x]));
            }
        }
    }

    free(fps);
    free(items);
    free(bad_files);

    logger->log(
        LogLevel::DBG, 
        "RSShard::validate_shards", 
        std::to_string(num_good_files) + " valid " + std::to_string(num_bad_files) +" invalid files."
    );
    return num_good_files;
}

/* Uses reed solomon encoding to reconstruct filename using <threshold> number of shards out of
 * the total <num_shares> number of shards. The filenames are contained in the fds[] struct and set up
 * in the helper function unshard_file. The metadata is contained in the header followed by encoded data.
 */
int rs_unshard_fd(struct file_descriptor *fds[], int num_fds, const struct file_descriptor *output_fd, int threshold, int num_shares)
{
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "RSShard::rs_unshard_fd", "Begin");

    int meta_k = threshold;
    int meta_m = num_shares;
    int meta_w = RS_WORD_SIZE;

    FILE **fps; // File pointer
    FILE *fp;
    /* Jerasure arguments */
    char **data;
    char **coding;
    int *erasures;
    int *erased;
    int *matrix;
    int *bitmatrix;
    int n, readins;
    char *meta_buffer;
    int meta_blocksize;

    /* Parameters */
    int k, m, w, buffersize;
    int tech;
    char *c_tech;

    int i, j; // loop control variable, s
    int ret;
    int blocksize = 0;  // size of individual files
    int origsize;       // size of file before padding
    int total;          // used to write data, not padding to file
    struct stat status; // used to find size of individual files
    int numerased;      // number of erased files

    /* Used to recreate file names */
    char *temp;
    char *cs1, *cs2, *extension;
    char *fname;
    int md;
    char *curdir;
    //    int num_fds;
    unsigned char low, hi;
    int shard_index;
    int index_found;
    int index;
    int fd;
    int pre_padded;
    int updated_num_fds;
    int fread_len;

    matrix = NULL;
    bitmatrix = NULL;

    k = meta_k;
    m = meta_m;
    w = meta_w;
    origsize = RS_META_SIZE;
    buffersize = RS_META_BUFFERSIZE;
    readins = RS_META_READINS;
    blocksize = RS_META_SIZE / k; // meta blocksize (filesize) is 1024 / k
    meta_blocksize = blocksize;

    // check length of fds by counting until NULL
    //    i = 0;
    // while (1) {
    //  if (fds[i] != NULL)
    // printf("string len: %d\n", strlen(fds[i]->filename));
    //      i++;
    //  else
    //      break;
    //}

    //    num_fds = i;
    // fps = (FILE **) malloc (sizeof(FILE *) * num_fds);
    fps = (FILE **)calloc(sizeof(FILE *), num_fds);

    /* Allocate memory */
    // erased = (int *)calloc(sizeof(int),(k+m));
    // for (i = 0; i < k+m; i++)
    //     erased[i] = 0;
    erasures = (int *)calloc(sizeof(int), (k + m));
    data = (char **)calloc(sizeof(char *), k);
    coding = (char **)calloc(sizeof(char *), m);
    if (buffersize != origsize)
    {
        for (i = 0; i < k; i++)
        {
            data[i] = (char *)calloc(sizeof(char), (buffersize / k));
        }
        for (i = 0; i < m; i++)
        {
            coding[i] = (char *)calloc(sizeof(char), (buffersize / k));
        }
        blocksize = buffersize / k;
    }

    matrix = reed_sol_vandermonde_coding_matrix(k, m, w);

    /* Begin decoding process */
    // validate_shards(fds, num_fds, threshold, num_shares, &updated_num_fds);
    int num_good_shards;

    num_good_shards = validate_shards(fds, num_fds, threshold, num_shares);

    if (num_good_shards < threshold)
    {
        logger->log(
            LogLevel::ERR, 
            "RSShard::rs_unshard_fd", 
            "Only validated " + std::to_string(num_good_shards) + " shards. " + std::to_string(threshold) + " are required. Exiting."
        );
        for (i = 0; i < num_good_shards; i++)
        {
            // free bad files
            if (fds[i] != NULL)
            {
                remove(fds[i]->filename); // remove file to leave no trace
                free(fds[i]->filename);
                close(fds[i]->fd);
                free(fds[i]);
            }
        }
        return -1;
    }
    else
    {
        logger->log(
            LogLevel::DBG, 
            "RSShard::rs_unshard_fd", 
            "Success! Validated " + std::to_string(num_good_shards) + " shards. only " + std::to_string(threshold) + " are required."
        );
    }

    // struct file_descriptor* uncorrup_fds[updated_num_fds];

    // num_fds = updated_num_fds;
    // for (int up_file = 0; up_file < updated_num_fds; up_file++) {
    //     // print_hex(cur_file_hash, MD5_DIGEST_LENGTH);
    //     uncorrup_fds[up_file] = (struct file_descriptor*)calloc(1, sizeof(struct file_descriptor));
    //     uncorrup_fds[up_file]->filename = fds[up_file]->filename;
    //     uncorrup_fds[up_file]->fd = fds[up_file]->fd;
    //     printf("\n\tAFTER CALL OF validate_shards()\n\t uncorrup_fds[%d]: \t%s  ~~~~~~~~~~\n\n\n", up_file, uncorrup_fds[up_file]->filename);
    // }

    // m = updated_num_fds;

    total = 0;
    n = 1;
    while (n <= readins)
    {
        numerased = 0;

        for (i = 1; i <= k; i++)
        {
            // erased[i-1] = 1;
            erasures[numerased] = i - 1;
            numerased++;
        }
        for (i = 1; i <= m; i++)
        {
            index_found = -1;
            // for (j = 0; j < updated_num_fds; j++) {
            for (j = 0; j < num_good_shards; j++)
            {
                if (fds[j] == NULL)
                {
                    logger->log(
                        LogLevel::ERR, 
                        "RSShard::rs_unshard_fd", 
                        "A fd==NULL (most likely failed validation)"
                    );
                    continue;
                }
                fps[j] = fdopen(dup(fds[j]->fd), "rb");
                fseek(fps[j], 0L, SEEK_SET); // seek to beginning of file since fd is duplicated
                if (fps[j] == NULL)
                {
                    logger->log(
                        LogLevel::WARN, 
                        "RSShard::rs_unshard_fd", 
                        "Could not open input file; Continueing"
                    );
                    continue;
                }
                // fps[j] = fopen(uncorrup_fds[j]->filename, "rb");
                low = fgetc(fps[j]);
                hi = fgetc(fps[j]);
                shard_index = (low << 8) | hi;
                if (i == shard_index)
                {
                    index_found = shard_index;
                    if (buffersize == origsize)
                    {
                        coding[i - 1] = (char *)calloc(sizeof(char), blocksize);
                        fread_len = fread(coding[i - 1], sizeof(char), blocksize, fps[j]);

                        if (fread_len != blocksize)
                        {
                            logger->log(
                                LogLevel::ERR, 
                                "RSShard::rs_unshard_fd", 
                                "fread_len != blocksize"
                            );
                        }
                    }
                    else
                    {
                        fseek(fps[j], blocksize * (n - 1), SEEK_SET);
                        fread_len = fread(coding[i - 1], sizeof(char), blocksize, fps[j]);
                        if (fread_len != blocksize)
                        {
                            logger->log(
                                LogLevel::ERR, 
                                "RSShard::rs_unshard_fd", 
                                "fread_len != blocksize"
                            );                        
                        }
                    }
                    fclose(fps[j]);
                    break;
                }
                else
                {
                    fclose(fps[j]);
                }
            }

            // if index not found
            if (index_found == -1)
            {
                // erased[k+(i-1)] = 1;
                erasures[numerased] = k + i - 1;
                numerased++;
            }
        }

        /* Finish allocating data/coding if needed */
        if (n == 1)
        {
            for (i = 0; i < numerased; i++)
            {
                if (erasures[i] < k)
                {
                    data[erasures[i]] = (char *)calloc(sizeof(char), blocksize);
                }
                else
                {
                    coding[erasures[i] - k] = (char *)calloc(sizeof(char), blocksize);
                }
            }
        }

        erasures[numerased] = -1;

        /* Choose proper decoding method */
        ret = jerasure_matrix_decode(k, m, w, matrix, 1, erasures, data, coding, blocksize);
        /* Exit if decoding was unsuccessful */
        if (ret == -1)
        {
            logger->log(
                LogLevel::ERR, 
                "RSShard::rs_unshard_fd", 
                "jerasure_matrix_decode #1: unsuccessful!"
            );
            return -1;
        }

        /* Create decoded file */
        meta_buffer = (char *)calloc(sizeof(char), RS_META_SIZE);

        index = 0;
        for (i = 0; i < k; i++)
        {
            if (total + blocksize <= origsize)
            {
                memcpy(meta_buffer + index, data[i], blocksize);
                index += blocksize;
                total += blocksize;
            }
            else
            {
                for (j = 0; j < blocksize; j++)
                {
                    if (total < origsize)
                    {
                        meta_buffer[index] = data[i][j];
                        index++;
                        total++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        n++;
    }

    sscanf(meta_buffer, "%d %d %d\n%d %d %d %d\n%d", &origsize, &blocksize, &pre_padded, &k, &m, &w, &buffersize, &readins);
    // printf("origsize: %d\nblocksize: %d\npre_padded: %d\nk: %d\nm: %d\nw: %d\nbuffersize: %d\nreadins: %d\n", origsize, blocksize, pre_padded, k, m, w, buffersize, readins);

    free(data);
    free(coding);
    free(erasures);
    // free(erased);

    // now unshard file

    /* Allocate memory */
    // erased = (int *)calloc(sizeof(int),(k+m));
    // for (i = 0; i < k+m; i++)
    //     erased[i] = 0;
    erasures = (int *)calloc(sizeof(int), (k + m));

    data = (char **)malloc(sizeof(char *) * k);
    coding = (char **)malloc(sizeof(char *) * m);
    if (buffersize != origsize)
    {
        for (i = 0; i < k; i++)
        {
            data[i] = (char *)calloc(sizeof(char), (buffersize / k));
        }
        for (i = 0; i < m; i++)
        {
            coding[i] = (char *)calloc(sizeof(char), (buffersize / k));
        }
        blocksize = buffersize / k;
    }

    matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
    // printf(matrix);

    /* Begin decoding process */
    logger->log(
        LogLevel::DBG, 
        "RSShard::rs_unshard_fd", 
        "Begin decoding"
    );
    total = 0;
    n = 1;
    while (n <= readins)
    {
        numerased = 0;

        for (i = 1; i <= k; i++)
        {
            // erased[i-1] = 1;
            erasures[numerased] = i - 1;
            numerased++;
        }

        for (i = 1; i <= m; i++)
        {
            index_found = -1;
            // for (j = 0; j < updated_num_fds; j++) {
            for (j = 0; j < num_good_shards; j++)
            {
                if (fds[j] == NULL)
                {
                    logger->log(
                        LogLevel::ERR, 
                        "RSShard::rs_unshard_fd", 
                        "A fd failed download or validation"
                    );
                    continue;
                }
                fps[j] = fdopen(dup(fds[j]->fd), "rb");
                fseek(fps[j], 0, SEEK_SET); // seek to beginning of file since fd is duplicated
                if (fps[j] == NULL)
                {
                    logger->log(
                        LogLevel::WARN, 
                        "RSShard::rs_unshard_fd", 
                        "Could not open input file, skipping"
                    );
                    continue;
                }
                // fps[j] = fopen(uncorrup_fds[j]->filename, "rb");
                low = fgetc(fps[j]);
                hi = fgetc(fps[j]);
                shard_index = (low << 8) | hi;
                if (i == shard_index)
                {
                    index_found = shard_index;
                    // printf("shard_index: %d\n", shard_index);
                    fseek(fps[j], meta_blocksize + 2, SEEK_SET);
                    if (buffersize == origsize)
                    {
                        coding[i - 1] = (char *)calloc(sizeof(char), blocksize);
                        fread_len = fread(coding[i - 1], sizeof(char), blocksize, fps[j]);
                        if (fread_len != blocksize)
                        {
                            logger->log(
                                LogLevel::ERR, 
                                "RSShard::rs_unshard_fd", 
                                "fread_len != blocksize"
                            ); 
                        }
                    }
                    else
                    {
                        fseek(fps[j], meta_blocksize + 2 + blocksize * (n - 1), SEEK_SET);
                        fread_len = fread(coding[i - 1], sizeof(char), blocksize, fps[j]);
                        if (fread_len != blocksize)
                        {
                            logger->log(
                                LogLevel::ERR, 
                                "RSShard::rs_unshard_fd", 
                                "fread_len != blocksize"
                            ); 
                        }
                    }
                    fclose(fps[j]);
                    break;
                }
                else
                    fclose(fps[j]);
            }

            // if index not found
            if (index_found == -1)
            {
                // erased[k+(i-1)] = 1;
                erasures[numerased] = k + i - 1;
                numerased++;
            }
        }

        // Remove file_descriptor, file, and memory
        for (j = 0; j < num_good_shards; j++)
        {
            remove(fds[j]->filename); // remove file to leave no trace
            free(fds[j]->filename);
            close(fds[j]->fd);
            free(fds[j]);
        }

        /* Finish allocating data/coding if needed */
        if (n == 1)
        {
            for (i = 0; i < numerased; i++)
            {
                if (erasures[i] < k)
                {
                    data[erasures[i]] = (char *)calloc(sizeof(char), blocksize);
                }
                else
                {
                    coding[erasures[i] - k] = (char *)calloc(sizeof(char), blocksize);
                }
            }
        }

        erasures[numerased] = -1;

        /* Choose proper decoding method */
        // printf("matrix: %s\n", matrix);
        // printf("erasures: ");
        // for (i=0; i < m+k; i++)
        //     printf("%i ", erasures[i]);
        // printf("\ncoding: ");
        // for (i=0; i < k; i++)
        // {
        //     for (j=0; j < blocksize; j++)
        //         printf("%i ", coding[i][j]);
        //     printf("\n");
        // }
        // printf("\nblocksize: %i\n", blocksize);
        logger->log(LogLevel::DBG, "RSShard::rs_unshard_fd", "Begin jerasure matrix decoding");
        ret = jerasure_matrix_decode(k, m, w, matrix, 1, erasures, data, coding, blocksize);

        logger->log(LogLevel::DBG, "RSShard::rs_unshard_fd", "Finished jerasure matrix decoding");


        // printf("\ndata: ");
        // for (i=0; i < k; i++)
        // {
        //     for (j=0; j < blocksize; j++)
        //         printf("%i ", data[i][j]);
        //     printf("\n");
        // }

        /* Exit if decoding was unsuccessful */
        if (ret == -1)
        {
            logger->log(
                LogLevel::ERR, 
                "RSShard::rs_unshard_fd", 
                "jerasure_matrix_decode #2: unsuccessful!"
            );            
            return -1;
        }


        /* Create decoded file */
        if (n == 1)
        {
            fp = fdopen(dup(output_fd->fd), "wb");
        }
        else
        {
            fp = fdopen(dup(output_fd->fd), "ab");
        }

        for (i = 0; i < k; i++)
        {
            if (total + blocksize <= origsize)
            {
                fwrite(data[i], sizeof(char), blocksize, fp);
                total += blocksize;
            }
            else
            {
                for (j = 0; j < blocksize; j++)
                {
                    if (total < origsize)
                    {
                        fprintf(fp, "%c", data[i][j]);
                        total++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        n++;
        fclose(fp);
    }



    // now resize fp to pre_padded
    fp = fdopen(dup(output_fd->fd), "r+");
    if (ftruncate(fileno(fp), pre_padded) != 0)
    {
        logger->log(
                LogLevel::ERR, 
                "RSShard::rs_unshard_fd", 
                "ftruncate error"
        );
        return -1;
    }
    fclose(fp);

    free(data);
    free(coding);
    free(erasures);
    free(fps);
    free(meta_buffer);
    return 1;
}

int randomize_header(const struct file_descriptor * fd){
  Logger* logger = Logger::getInstance();
  logger->log(LogLevel::DBG, "RSShard::randomize_header", std::string(fd->filename));

  unsigned char * data;
  unsigned char * hash_ext;
  unsigned char * rand_ext;
  unsigned char * hash_buf;

  int file_size;
  unsigned int pre_file_size;
  int chunk32s;
  int extraBytes;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  unsigned char buffer[SHA256_DIGEST_LENGTH];
  unsigned char pre_hash[SHA256_DIGEST_LENGTH];
  unsigned char pre_hash_ext[SHA256_DIGEST_LENGTH + sizeof(unsigned int)];
  unsigned char * file_buffer;
  unsigned char * rand_header;

  int bytes_written;
  int bytes_read;


  FILE * fp = fopen(fd->filename, "rb");
  fseek(fp, 0, SEEK_SET);
  int rc;
  if (fp == NULL)
    {
        logger->log(LogLevel::ERR, "RSShard::randomize_header", "Unable to open file");
        return -1;
    }
  // get file size
  fseek(fp, 0L, SEEK_END);
  pre_file_size = ftell(fp);
  fclose(fp);
  
  chunk32s = pre_file_size / 32;
  chunk32s = chunk32s * 32;

  extraBytes = pre_file_size - chunk32s; 

  data = (unsigned char *) malloc (pre_file_size);
  hash_ext = (unsigned char *) malloc (pre_file_size);
  rand_ext = (unsigned char *) malloc (pre_file_size);
  hash_buf = (unsigned char *) malloc (pre_file_size);


  // read first 64-bits of data to randomize

  // does rand_header need file_size now?
  // to unrandomize, we need sha256 pre-image + file_size
  // but we have file_size from length of the data
  // so we only need length of the random data (including header) and we can infer filesize
  rand_header = (unsigned char * ) malloc (SHA256_DIGEST_LENGTH + pre_file_size);


  fp = fopen(fd->filename, "r+");

  fseek(fp, 0, SEEK_SET);
  fread(data, 1, pre_file_size, fp);
  fclose(fp);

  // print_hex(data, sizeof(uint64_t));

  // generate random_header 256-bit + 64-bits
  
  // generate 256-bit pre-image
  rc = RAND_bytes(buffer, SHA256_DIGEST_LENGTH);

    if (rc != 1)
    {
        /* RAND_bytes failed */
        logger->log(LogLevel::ERR, "RSShard::randomize_header", "RAND bytes");
        return -1;
    }

  // print_hex(buffer, SHA256_DIGEST_LENGTH);


  // now buffer holds random 256-bits of data
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  // hash will store hash of random buffer data
  SHA256_Update(&sha256, buffer, SHA256_DIGEST_LENGTH);
  SHA256_Final(hash, &sha256);
  
  // copy H(Random Buffer Data) to pre_hash --> hashing once
  memcpy(pre_hash, hash, SHA256_DIGEST_LENGTH);


  // hash 100 - (number of additional hashes needed + 1 (from above))
  for (int i = 1 ; i < NUM_HASHES - (chunk32s / 32) + 1; i++) {
    SHA256_Update(&sha256, pre_hash, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash, &sha256);
    
    //copy final data to pre_hash;
    memcpy(pre_hash, hash, SHA256_DIGEST_LENGTH);
  }


  // copy 100x hash of random data into hash_buf size pre_file_size
  memcpy(hash_buf, hash, SHA256_DIGEST_LENGTH);

  // copy 100x hashed data to pre_hash_ext
  memcpy(pre_hash_ext, pre_hash, SHA256_DIGEST_LENGTH);


  // from i = 32 to i < size of the 32 divisible data i+=32
  unsigned int counter = 0;
  unsigned int count_arr[1];
  for (int i = SHA256_DIGEST_LENGTH; i < chunk32s; i += SHA256_DIGEST_LENGTH) {
    
    // increment counter each 32
    counter++;

    // pre_hash_ext is size SHA256 + sizeof(unsigned int)
    // copy counter to end of pre_hash_ext which containes 100x hashed data
    count_arr[0] = counter;
    memcpy(pre_hash_ext + SHA256_DIGEST_LENGTH, count_arr, sizeof(unsigned int));
//    pre_hash_ext[SHA256_DIGEST_LENGTH] = ++counter;

    // now sha256 of pre_hash_ext which contains single 100x hash and counter
    SHA256_Update(&sha256, pre_hash_ext, SHA256_DIGEST_LENGTH + sizeof(unsigned int));
    SHA256_Final(hash, &sha256);

    // do this chunk32 times into hash_buf of size pre_file_size
    // hash_buf contains 100x hash of random data in first slot
    // now it contains 100x hash + counter in additional slots
    memcpy(hash_buf + i, hash, SHA256_DIGEST_LENGTH);
  }

  // now add extra bytes

  // first update counter to 100x hash pre-image
  counter++;
  count_arr[0] = counter;
  memcpy(pre_hash_ext + SHA256_DIGEST_LENGTH, count_arr, sizeof(unsigned int));

  // hash to hash_buf (size pre_file_size)
  SHA256_Update(&sha256, pre_hash_ext, SHA256_DIGEST_LENGTH + sizeof(unsigned int));
  SHA256_Final(hash, &sha256);

  // only copy over extraBytes
  memcpy(hash_buf + chunk32s, hash, extraBytes);

    // now take bytes of hash and xor with data
  memcpy(hash_ext, hash_buf, pre_file_size);

  for (int i = 0 ; i < pre_file_size; i++)
    rand_ext[i] = data[i] ^ hash_ext[i];

  // now set rand_header
  // buffer holds initial random data
  memcpy(rand_header, buffer, SHA256_DIGEST_LENGTH);

  // add data[i] xor hash_ext random data to rand_header (now whole file size)
  memcpy(rand_header + SHA256_DIGEST_LENGTH, rand_ext, pre_file_size);

  // allocate buffer same size as the file
 
  // read size of file
  fp = fopen(fd->filename, "rb");
  fseek(fp, 0, SEEK_SET);
    if (fp == NULL)
    {
        logger->log(LogLevel::ERR, "RSShard::randomize_header", "Unable to open file");
        return -1;
    }
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  fclose(fp);

  // open the file, and read it into the buffer
  // Reading data to array of unsigned chars
  fp = fopen(fd->filename, "r+");
  fseek(fp, 0, SEEK_SET);
  file_buffer = (unsigned char *) malloc(file_size);
  bytes_read = fread(file_buffer, sizeof(unsigned char), file_size, fp);
  fclose(fp);


  // print_hex(rand_header, (SHA256_DIGEST_LENGTH + sizeof(uint64_t)) * sizeof(unsigned char));


  // write the header
  fp = fopen(fd->filename, "w");
  fseek(fp, 0, SEEK_SET);
  bytes_written = fwrite(rand_header, sizeof(unsigned char), SHA256_DIGEST_LENGTH + pre_file_size, fp);
  ftruncate(fileno(fp), file_size + SHA256_DIGEST_LENGTH);
  fclose(fp);
  free(file_buffer);
  free(data);
  free(hash_ext);
  free(rand_ext);
  free(hash_buf);
  free(rand_header);

  return 1;
}

int unrandomize_header(const struct file_descriptor * fd){

  Logger* logger = Logger::getInstance();
  logger->log(LogLevel::DBG, "RSShard::unrandomize_header", std::string(fd->filename));

  unsigned char * rand_header;
  unsigned char * hash_buf;
  unsigned char * hash_ext;
  unsigned char * rand_ext;
  unsigned char * data;

  unsigned char pre_image[SHA256_DIGEST_LENGTH];
  unsigned char pre_hash[SHA256_DIGEST_LENGTH];
  unsigned char pre_hash_ext[SHA256_DIGEST_LENGTH + sizeof(unsigned int)];
  unsigned char hash[SHA256_DIGEST_LENGTH];

  int file_size;
  unsigned char * file_buffer;
  int bytes_read;
  int bytes_written;
  int pre_file_size;
  int chunk32s;
  int extraBytes;

   FILE * fp = fdopen(dup(fd->fd), "rb");

  fseek(fp, 0, SEEK_SET);
  int rc;

  // get file size
  if (fp == NULL)
    {
        logger->log(LogLevel::ERR, "RSShard::unrandomize_header", "Unable to open file");
        return -1;
    }
  fseek(fp, 0L, SEEK_END);
  pre_file_size = ftell(fp) - SHA256_DIGEST_LENGTH;
  fclose(fp);

  chunk32s = pre_file_size / 32;
  chunk32s = chunk32s * 32;

  extraBytes = pre_file_size - chunk32s; 


  data = (unsigned char *) malloc (pre_file_size);
  hash_ext = (unsigned char *) malloc (pre_file_size);
  rand_ext = (unsigned char *) malloc (pre_file_size);
  hash_buf = (unsigned char *) malloc (pre_file_size);
  rand_header = (unsigned char * ) malloc (SHA256_DIGEST_LENGTH + pre_file_size);

  // read first 256-bit + RANDOMIZE_BYTES bytes for header
 // FILE * fp;
  fp = fdopen(dup(fd->fd), "r+");
  fseek(fp, 0, SEEK_SET);
  fread(rand_header,sizeof(unsigned char), SHA256_DIGEST_LENGTH + pre_file_size, fp);
  fclose(fp);

  // print_hex(rand_header, (SHA256_DIGEST_LENGTH + RANDOMIZE_BYTES) * sizeof(unsigned char));

  // extract data from header and write to file
  // copy first 256-bits and use as pre-image to hash
  // copy last pre_file_size worth to use as rand64 xor value
  memcpy(pre_image, rand_header, SHA256_DIGEST_LENGTH);
  memcpy(rand_ext, rand_header + SHA256_DIGEST_LENGTH, pre_file_size);

  // hash the pre-image
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  // once hash to start
  SHA256_Update(&sha256, pre_image, SHA256_DIGEST_LENGTH);
  SHA256_Final(hash, &sha256);
  memcpy(pre_hash, hash, SHA256_DIGEST_LENGTH);

  for (int i = 1 ; i < NUM_HASHES - (chunk32s / 32) + 1; i++) {
    SHA256_Update(&sha256, pre_hash, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash, &sha256);
    memcpy(pre_hash, hash, SHA256_DIGEST_LENGTH);
  }
  

  memcpy(hash_buf, hash, SHA256_DIGEST_LENGTH);
  memcpy(pre_hash_ext, pre_hash, SHA256_DIGEST_LENGTH);
  unsigned int counter = 0;
  unsigned int count_arr[1];
  for (int i = SHA256_DIGEST_LENGTH; i < chunk32s; i += SHA256_DIGEST_LENGTH) {

    counter++;
    count_arr[0] = counter;
    memcpy(pre_hash_ext + SHA256_DIGEST_LENGTH, count_arr, sizeof(unsigned int));
    SHA256_Update(&sha256, pre_hash_ext, SHA256_DIGEST_LENGTH + sizeof(unsigned int));
    SHA256_Final(hash, &sha256);
    memcpy(hash_buf + i, hash, SHA256_DIGEST_LENGTH);
  }

  counter++;
  count_arr[0] = counter;
  memcpy(pre_hash_ext + SHA256_DIGEST_LENGTH, count_arr, sizeof(unsigned int));
  SHA256_Update(&sha256, pre_hash_ext, SHA256_DIGEST_LENGTH + sizeof(unsigned int));
  SHA256_Final(hash, &sha256);
  memcpy(hash_buf + chunk32s, hash, extraBytes);

  memcpy(hash_ext, hash_buf, pre_file_size);
  for (int i = 0 ; i < pre_file_size; i++)
    data[i] = rand_ext[i] ^ hash_ext[i];

  // copy data to front of file
  // truncate file length

  // read size of file
  fp = fdopen(dup(fd->fd), "rb");
  fseek(fp, 0, SEEK_SET);
  if (fp == NULL) {
    fprintf(stderr,  "Unable to open file (%s).\n", fd->filename);
    return -1;
  }
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  fclose(fp);

  // open the file, and read it into the buffer
  // Reading data to array of unsigned chars
  fp = fdopen(dup(fd->fd), "r+");
  fseek(fp, 0, SEEK_SET);
  file_buffer = (unsigned char *) malloc(file_size);
  bytes_read = fread(file_buffer, sizeof(unsigned char), file_size, fp);
  fclose(fp);

  // print_hex(data, RANDOMIZE_BYTES_DEBUG);

  // write the header and buffer minus the original header
  fp = fdopen(dup(fd->fd), "w");
  fseek(fp, 0, SEEK_SET);
  ftruncate(fileno(fp), 0);
  bytes_written = fwrite(data, 1, pre_file_size, fp);
  bytes_written += fwrite(file_buffer + SHA256_DIGEST_LENGTH + pre_file_size, 1, file_size - SHA256_DIGEST_LENGTH - pre_file_size, fp);

  fclose(fp);
  free(file_buffer);
  free(data);
  free(hash_ext);
  free(rand_ext);
  free(hash_buf);
  free(rand_header);

  return 1;
}
