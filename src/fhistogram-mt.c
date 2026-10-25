// Setting _DEFAULT_SOURCE is necessary to test visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "job_queue.h"

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include "histogram.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
int global_histogram[8] = { 0 };

// Merge local counts into the global histogram in bytes,
// but print once per PRINT_INTERVAL_MS.
#define MERGE_INTERVAL_BYTES 100000
#define PRINT_INTERVAL_MS 50

static long last_print_ms = 0;  // protected by stdout_mutex

static long now_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000L + tv.tv_usec / 1000;
}

//Merge a local histogram into the global one.
//Print if enough time has passed since the last print.
static void flush_local(int local[8]) {
  pthread_mutex_lock(&stdout_mutex);
  merge_histogram(local, global_histogram);  // zeroes local
  long t = now_ms();
  if (t - last_print_ms >= PRINT_INTERVAL_MS) {
    print_histogram(global_histogram);
    last_print_ms = t;
  }
  pthread_mutex_unlock(&stdout_mutex);
}
   
int fhistogram(char const *path) {
  FILE *f = fopen(path, "r");

  if (f == NULL) {
    pthread_mutex_lock(&stdout_mutex);
    fflush(stdout);
    warn("failed to open %s", path);
    pthread_mutex_unlock(&stdout_mutex);
    return -1;
  }

  int local_histogram[8] = { 0 };
  unsigned char buf[4096];
  size_t since_flush = 0;
  size_t n;

  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    for (size_t j = 0; j < n; j++) {
      update_histogram(local_histogram, buf[j]);
    }
    since_flush += n;
    if (since_flush >= MERGE_INTERVAL_BYTES) {
      flush_local(local_histogram);
      since_flush = 0;
    }
  }

  fclose(f);
  flush_local(local_histogram);
  return 0;
}

static void *worker(void *arg) {
  struct job_queue *jq = arg;
  void *job;
  while (job_queue_pop(jq, &job) == 0) {
    char *path = job;
    fhistogram(path);
    free(path);  // we own the strdup'ed string
  }
  return NULL; 
}


int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: paths...");
    exit(1);
  }

  int num_threads = 1;
  char * const *paths = &argv[1];

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

    paths = &argv[3];
  } else {
    paths = &argv[1];
  }

  struct job_queue jq;
  if (job_queue_init(&jq, 64) != 0) {
    errx(1, "job_queue_init() failed");
  }

  pthread_t *threads = calloc(num_threads, sizeof(pthread_t));
  for (int i = 0; i < num_threads; i++) {
    if (pthread_create(&threads[i], NULL, &worker, &jq) != 0) {
      err(1, "pthread_create() failed");
    }
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
    case FTS_F:
      char *copy = strdup(p->fts_path);  // fts reuses p->fts_path
      if (copy == NULL) {
        err(1, "strdup() failed");
      }
      if (job_queue_push(&jq, copy) != 0) {
        free(copy);  // queue refused it, so nobody else will free it
      }
      break;
    default:
      break;
    }
  }
  fts_close(ftsp);

  job_queue_destroy(&jq);  // waits for the queue to drain, wakes idle workers
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);  // wait for in-progress files to finish
  }
  free(threads);

  // Rate-limited printing may have skipped the last update, so show
  // the final, complete result. All threads are joined, so no lock needed.
  print_histogram(global_histogram);
  move_lines(9);

  return 0;
}
