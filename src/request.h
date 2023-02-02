#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "segel.h"


typedef struct request_t {
    int connfd;
    int id;
    struct timeval arrival_time;
    struct timeval dispatch_interval;
} Request;

Request *requestCreate(int connfd);

void requestHandle(int fd, int thread_id);

typedef struct stat_thread {
    int id;
    int thread_count;
    int thread_static;
    int thread_dynamic;
} statThread;

statThread *statThreadCreate(int id);

statThread **stat_thread_array;
Request **job_data_array;

#endif
