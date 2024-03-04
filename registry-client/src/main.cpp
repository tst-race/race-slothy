#include "registry.h"
#include  <stdio.h>
#include  <signal.h>
#include  <stdlib.h>
#include <chrono>
#include <thread>


RaceRegistry* registry = new RaceRegistry();   

void  intHandler(int sig){
    printf("Interrupted\n");
    registry->isRunning = false;
}

std::int32_t main(int argc, char *argv[]) {
    // Set up interupt handler
    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);

    // Start registry
    int exitCode;
    if(argc == 2 && strcmp(argv[1], "norace") == 0){
        printf("Starting registry with RACE interface disabled\n");
        exitCode = registry->start(false);

    }else{
        exitCode = registry->start(true);
    }
    

    return exitCode;
}
