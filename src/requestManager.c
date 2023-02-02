//
// Created by Alis Daniel.
//

#include "requestManager.h"


Node *nodeCreate(int connfd) {
    Node *node = (Node *) malloc(sizeof(Node));
    if (node == NULL) {
        return NULL;
    }
    node->data = requestCreate(connfd);
    node->prev = NULL;
    node->next = NULL;
    return node;
}

Queue *queueCreate(char *policy, int max_request, int max_workers,
                   pthread_mutex_t global_lock, pthread_cond_t work_allowed_not_full,
                   pthread_cond_t work_allowed_not_empty) {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->workers = 0;
    queue->policy = malloc(strlen(policy) * sizeof(char) + 1);
    strcpy(queue->policy, policy);
    queue->max_request = max_request;
    queue->max_workers = max_workers;
    queue->global_lock = global_lock;
    queue->work_allowed_not_full = work_allowed_not_full;
    queue->work_allowed_not_empty = work_allowed_not_empty;

    return queue;
}

void queueDestroy(Queue *queue) {
    Node *temp;
    while (!isEmpty(queue)) {
        temp = queue->head;
        queue->head = queue->head->next;
        queue->size--;
        free(temp);
    }
    free(queue->policy);
    free(queue);
}

bool enqueue(Queue *queue, Node *node_insert) {
    if ((queue == NULL) || (node_insert == NULL)) {
        return false;
    }
    pthread_mutex_lock(&queue->global_lock);
    while (queue->size == queue->max_request || queue->size + queue->workers == queue->max_request) {
        if (strcmp(queue->policy, "block") == 0) {
            pthread_cond_wait(&queue->work_allowed_not_full, &queue->global_lock);
        } else if ((strcmp(queue->policy, "dt") == 0) ||
                   (isEmpty(queue) && (strcmp(queue->policy, "dh") == 0 || strcmp(queue->policy, "random") == 0))) {
            Close(node_insert->data->connfd);
            free(node_insert->data);
            free(node_insert);
            pthread_mutex_unlock(&queue->global_lock);
            return false;
        } else if (strcmp(queue->policy, "random") == 0) {
            int num_to_remove = (50 * queue->size) / 100;
            if ((50 * queue->size) % 100 != 0) {
                num_to_remove++;
            }
            for (int i = 0; i < num_to_remove; i++) {
                remove_by_ind(queue, rand() % queue->size);
            }
        } else if (strcmp(queue->policy, "dh") == 0) {
            Node *to_remove = dequeue(queue, false);
            Close(to_remove->data->connfd);
            free(to_remove->data);
            free(to_remove);
        }else{
            exit(1);
        }
    }
    if (queue->size == 0) {
        queue->head = node_insert;
        queue->tail = node_insert;
    } else {
        Node *old_tail = queue->tail;
        queue->tail = node_insert;
        (queue->tail)->prev = old_tail;
        old_tail->next = node_insert;
    }
    queue->size++;
    pthread_cond_signal(&queue->work_allowed_not_empty);
    pthread_mutex_unlock(&queue->global_lock);
    return true;
}

Node *dequeue(Queue *queue, bool to_lock) {
    if (to_lock) {
        pthread_mutex_lock(&queue->global_lock);
    }
    Node *to_remove;
    while (isEmpty(queue)) {
        pthread_cond_wait(&queue->work_allowed_not_empty, &queue->global_lock);
    }
    to_remove = queue->head;
    if (queue->size == 1) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = (queue->head)->next;
        queue->head->prev = NULL;
    }
    queue->size--;
    pthread_cond_signal(&queue->work_allowed_not_full);
    if (to_lock) {
        pthread_mutex_unlock(&queue->global_lock);
    }
    return to_remove;
}

bool remove_by_fd(Queue *queue, int fd) {
    Node *to_remove = NULL;
    if (isEmpty(queue))
        return false;
    Node *iter = queue->head;
    while (iter != NULL) {
        if (iter->data->connfd == fd) {
            to_remove = iter;
            break;
        } else {
            iter = iter->next;
        }
    }
    if (to_remove == NULL) {
        return false;
    } else {
        if (queue->size == 1) {
            queue->head = NULL;
            queue->tail = NULL;
            queue->size--;
            return true;
        } else if (to_remove->prev == NULL) {
            queue->head = to_remove->next;
            queue->head->prev = NULL;
        } else {
            Node *prev = to_remove->prev;
            prev->next = to_remove->next;
        }
        if (to_remove->next == NULL) {
            queue->tail = to_remove->prev;
            queue->tail->next = NULL;
        } else {
            Node *next = to_remove->next;
            next->prev = to_remove->prev;
        }
        queue->size--;
        return true;
    }
}

bool remove_by_ind(Queue *queue, int index) {
    if (isEmpty(queue))
        return false;
    Node *to_remove = queue->head;
    for (int i = 0; i < index; i++) {
        to_remove = to_remove->next;
    }
    if (to_remove == NULL) {
        return false;
    } else {
        if (queue->size == 1) {
            queue->head = NULL;
            queue->tail = NULL;
        }
        else {

            if (to_remove->prev == NULL) {
                queue->head = to_remove->next;
            } else {
                Node *prev = to_remove->prev;
                prev->next = to_remove->next;
            }
            if (to_remove->next == NULL) {
                queue->tail = to_remove->prev;
            } else {
                Node *next = to_remove->next;
                next->prev = to_remove->prev;
            }
        }
        queue->size--;
        Close(to_remove->data->connfd);
        free(to_remove->data);
        free(to_remove);
        return true;
    }
}

bool isEmpty(Queue *queue) {
    if (queue == NULL) {
        return false;
    }
    if (queue->size == 0) {
        return true;
    } else {
        return false;
    }
}

requestManager *requestManagerCreate(char *policy, int max_request, int max_workers,
                                     pthread_mutex_t global_lock, pthread_cond_t work_allowed_not_full,
                                     pthread_cond_t work_allowed_not_empty) {
    requestManager *new_request_manager = (requestManager *) malloc(sizeof(requestManager));
    if (new_request_manager == NULL) {
        return NULL;
    }
    new_request_manager->policy = malloc(strlen(policy) * sizeof(char) + 1);
    strcpy(new_request_manager->policy, policy);
    new_request_manager->max_request = max_request;
    new_request_manager->max_workers = max_workers;
    new_request_manager->wait_queue = queueCreate(policy, max_request, max_workers, global_lock, work_allowed_not_full,
                                                  work_allowed_not_empty);
    new_request_manager->global_lock = global_lock;
    new_request_manager->work_allowed_not_full = work_allowed_not_full;
    new_request_manager->work_allowed_not_empty = work_allowed_not_empty;
    return new_request_manager;
}

void addRequest(requestManager *manager, int connfd) {
    Node *new_request = nodeCreate(connfd);
    enqueue(manager->wait_queue, new_request);
}


void activeRequest(requestManager *manager, int id) {
    Node *to_switch = dequeue(manager->wait_queue, true);
    job_data_array[id] = to_switch->data;
    gettimeofday(&to_switch->data->dispatch_interval, NULL);
    timersub( &to_switch->data->dispatch_interval,&to_switch->data->arrival_time, &to_switch->data->dispatch_interval);
    pthread_mutex_lock(&manager->global_lock);
    manager->wait_queue->workers++;
    pthread_mutex_unlock(&manager->global_lock);
    requestHandle(to_switch->data->connfd, id);
    Close(to_switch->data->connfd);
    job_data_array[id] = NULL;
    pthread_mutex_lock(&manager->global_lock);
    manager->wait_queue->workers--;
    pthread_mutex_unlock(&manager->global_lock);
    pthread_cond_signal(&manager->wait_queue->work_allowed_not_full);
    free(to_switch->data);
    free(to_switch);
}
