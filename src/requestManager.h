//
// Created by Alis Daniel.
//

#ifndef REQUESTMANEGAR_H
#define REQUESTMANEGAR_H


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "request.h"


pthread_t *threads_array;


typedef struct node_t {
    Request *data;
    struct node_t *prev;
    struct node_t *next;
} Node;

Node *nodeCreate(int connfd);

typedef struct Queue {
    int size;
    Node *head;
    Node *tail;
    char *policy;
    int max_workers;
    int max_request;
    int workers;
    pthread_mutex_t global_lock;
    pthread_cond_t work_allowed_not_full;
    pthread_cond_t work_allowed_not_empty;

} Queue;

Queue *queueCreate(char *policy, int max_request, int max_workers,
                   pthread_mutex_t global_lock, pthread_cond_t work_allowed_not_full,
                   pthread_cond_t work_allowed_not_empty);

void queueDestroy(Queue *queue);

bool enqueue(Queue *queue, Node *node_insert);

Node *dequeue(Queue *queue, bool to_lock);

bool remove_by_fd(Queue *queue, int fd);

bool remove_by_ind(Queue *queue, int index);

bool isEmpty(Queue *queue);


typedef struct request_manager {
    Queue *wait_queue;
    int workers;
    int max_workers;
    int max_request;
    char *policy;
    pthread_mutex_t global_lock;
    pthread_cond_t work_allowed_not_full;
    pthread_cond_t work_allowed_not_empty;
} requestManager;

requestManager *requestManagerCreate(char *policy, int max_request, int max_workers,
                                     pthread_mutex_t global_lock, pthread_cond_t work_allowed_not_full,
                                     pthread_cond_t work_allowed_not_empty);

void addRequest(requestManager *manager, int connfd);

void activeRequest(requestManager *manager, int id);



#endif //REQUESTMANEGAR_H


