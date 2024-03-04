#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <curl/curl.h>

#include <pthread.h>
#include <assert.h>

#include "shard.hpp"
#include "util/file_descriptor.hpp"
#include "util/sleep.hpp"
#include "util/rs_shard.hpp"
#include <vector>
#include <string>


class Downloader {
    public:
        static struct file_descriptor* download(std::string url);
        static struct file_descriptor* download_and_unshard(std::vector<std::string> urls);
    private:
        static struct file_descriptor* download_with_retry(const char *url, int max_retries, int ms_between_retry);
        static struct file_descriptor* download_with_retry_hardcoded(const char *url);

        static struct file_descriptor* download_redundant(char ** urls, unsigned int num_urls);
        static struct file_descriptor* download_redundant_multithreaded(char ** urls, unsigned int num_urls);

};

#endif