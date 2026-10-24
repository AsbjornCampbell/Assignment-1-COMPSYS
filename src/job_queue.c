#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

int job_queue_init(struct job_queue *job_queue, int capacity) {
  if (capacity <= 0) {
    return -1;
  }

  job_queue->buffer = malloc(capacity * sizeof(void *));
  if (job_queue->buffer == NULL) {
    return -1;
  }

  job_queue->capacity = capacity;
  job_queue->count = 0;
  job_queue->head = 0;
  job_queue->tail = 0;
  job_queue->closed = 0;

  if (pthread_mutex_init(&job_queue->mutex, NULL) != 0) {
    free(job_queue->buffer);
    return -1;
  }

  if (pthread_cond_init(&job_queue->not_empty, NULL) != 0) {
    pthread_mutex_destroy(&job_queue->mutex);
    free(job_queue->buffer);
    return -1;
  }

  if (pthread_cond_init(&job_queue->not_full, NULL) != 0) {
    pthread_cond_destroy(&job_queue->not_empty);
    pthread_mutex_destroy(&job_queue->mutex);
    free(job_queue->buffer);
    return -1;
  }

  return 0;
}
// Call after all producers have finished pushing jobs.
// Waits until the queue is empty, closes it, and frees the buffer.
// The caller must join all workers before destroying the mutex
// and condition variables.
int job_queue_destroy(struct job_queue *job_queue) {
  pthread_mutex_lock(&job_queue->mutex);

  while (job_queue->count > 0) {
    pthread_cond_wait(&job_queue->not_full, &job_queue->mutex);
  }

  job_queue->closed = 1;

  pthread_cond_broadcast(&job_queue->not_empty);
  pthread_cond_broadcast(&job_queue->not_full);

  free(job_queue->buffer);
  job_queue->buffer = NULL;

  pthread_mutex_unlock(&job_queue->mutex);
  return 0;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  pthread_mutex_lock(&job_queue->mutex);

  while (job_queue->count == job_queue->capacity
         && !job_queue->closed) {
    pthread_cond_wait(&job_queue->not_full, &job_queue->mutex);
  }

  if (job_queue->closed) {
    pthread_mutex_unlock(&job_queue->mutex);
    return -1;
  }

  job_queue->buffer[job_queue->tail] = data;
  job_queue->tail = (job_queue->tail + 1) % job_queue->capacity;
  job_queue->count++;

  pthread_cond_signal(&job_queue->not_empty);

  pthread_mutex_unlock(&job_queue->mutex);
  return 0;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  pthread_mutex_lock(&job_queue->mutex);

  while (job_queue->count == 0 && !job_queue->closed) {
    pthread_cond_wait(&job_queue->not_empty, &job_queue->mutex);
  }

  if (job_queue->count == 0 && job_queue->closed) {
    pthread_mutex_unlock(&job_queue->mutex);
    return -1;
  }

  *data = job_queue->buffer[job_queue->head];
  job_queue->head = (job_queue->head + 1) % job_queue->capacity;
  job_queue->count--;

  pthread_cond_broadcast(&job_queue->not_full);

  pthread_mutex_unlock(&job_queue->mutex);
  return 0;
}
