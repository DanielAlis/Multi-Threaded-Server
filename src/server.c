#include "segel.h"
#include "requestManager.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

requestManager * manager = NULL;

char *method = "";
int active_workers, total_requests;
int max_workers, max_requests;
pthread_mutex_t global_lock;
pthread_cond_t work_allowed_not_full;
pthread_cond_t work_allowed_not_empty;

void *handle_work(void *id) {
    while(1){
        activeRequest(manager,*(int *) id);
    }
}

void getargs(int *port, int argc, char *argv[])
{
    if (argc < 5) {
        exit(1);
    }
    *port = atoi(argv[1]);
    method = malloc(strlen(argv[4]) * sizeof(char) + 1);
    strcpy(method, argv[4]);
    max_workers = atoi(argv[2]);
    max_requests = atoi(argv[3]);
}


int main(int argc, char *argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    pthread_mutex_init(&global_lock, NULL);
    pthread_cond_init(&work_allowed_not_empty, NULL);
    pthread_cond_init(&work_allowed_not_full, NULL);
    threads_array = malloc(max_workers * sizeof(pthread_t));
    stat_thread_array = malloc(max_workers * sizeof(stat_thread_array));
    job_data_array = malloc(max_workers * sizeof(Request *));
    int *index_arr = malloc(max_workers * sizeof(int));

    manager = requestManagerCreate(method, max_requests, max_workers, global_lock, work_allowed_not_full,
                                   work_allowed_not_empty);

    for (int i = 0; i < max_workers; i++) {
        index_arr[i] = i;
        if (pthread_create(&threads_array[i], NULL, (void *) handle_work, (void *) &index_arr[i]) != 0) {
            exit(1);
        }
        stat_thread_array[i] = statThreadCreate(i);
    }

        listenfd = Open_listenfd(port);
        while (1) {
            clientlen = sizeof(clientaddr);
            connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
            addRequest(manager, connfd);
        }
}


    


 
