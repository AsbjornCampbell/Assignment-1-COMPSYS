// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include <pthread.h>

#include "job_queue.h"


pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void safe_print(char* string){
  pthread_mutex_lock(&print_mutex);
  printf("%s", string);
  pthread_mutex_unlock(&print_mutex);
}

int fauxgrep_file(char const *needle, char const *path) {
  FILE *f = fopen(path, "r");

  if (f == NULL) {
    warn("failed to open %s", path);
    return -1;
  }

  char *line = NULL;
  size_t linelen = 0;
  int lineno = 1;

  while (getline(&line, &linelen, f) != -1) {
    if (strstr(line, needle) != NULL) {
      int size = snprintf(NULL, 0, "%s:%d: %s", path, lineno,line) + 1;
      char *buffer = malloc(size);
      snprintf(buffer, size, "%s:%d: %s", path, lineno,line);
      safe_print(buffer);
      free(buffer);
    }

    lineno++;
  }

  free(line);
  fclose(f);

  return 0;
}


struct worker_args {
    struct job_queue *queue;
    const char *needle;
};

void *worker(void *arg) {
  struct worker_args *args = arg;
  char* file;
  while (1){
    int code = job_queue_pop(args->queue, (void **)&file);

    if (code == -1){
      return NULL;
    }

    fauxgrep_file(args->needle, file);
    free(file);
  }
}


int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: [-n INT] STRING paths...");
    exit(1);
  }

  int num_threads = 1;
  char const *needle = argv[1];
  char * const *paths = &argv[2];

  struct job_queue queue;

    if (job_queue_init(&queue, 10) != 0) {
      err(1, "failed to initialize job queue");
    }

  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
    // Since atoi() simply returns zero on syntax errors, we cannot
    // distinguish between the user entering a zero, or some
    // non-numeric garbage.  In fact, we cannot even tell whether the
    // given option is suffixed by garbage, i.e. '123foo' returns
    // '123'.  A more robust solution would use strtol(), but its
    // interface is more complicated, so here we are.
    
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
        err(1, "invalid thread count: %s", argv[2]);
    }
    needle = argv[3];
    paths = &argv[4];

  } 
  else {
    needle = argv[1];
    paths = &argv[2];
  }
  struct worker_args args;
  args.queue = &queue;
  args.needle = needle;

  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, worker, &args);
  }

  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;

  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F: {
      char *file = strdup(p->fts_path);
      job_queue_push(&queue, file);
      break;
    }
    default:
      break;
    }
  }

  fts_close(ftsp);
  job_queue_destroy(&queue);

  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }
  pthread_cond_destroy(&queue.not_empty);
  pthread_cond_destroy(&queue.not_full);
  pthread_mutex_destroy(&queue.mutex);
  pthread_mutex_destroy(&print_mutex);
  return 0;
}

