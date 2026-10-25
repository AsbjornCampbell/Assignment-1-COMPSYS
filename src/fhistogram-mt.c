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

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include "histogram.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
int global_histogram[8] = { 0 };

// Merge local counts into the global histogram this often (in bytes),
// but print at most once per PRINT_INTERVAL_MS.
#define MERGE_INTERVAL_BYTES 100000
#define PRINT_INTERVAL_MS 50

static long last_print_ms = 0;  // protected by stdout_mutex

static long now_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000L + tv.tv_usec / 1000;
}


int fhistogram(char const *path) {
  // TODO: kopiér funktionen fra fhistogram.c

  // TODO: de to steder, hvor der står
  //   merge_histogram(local_histogram, global_histogram);
  //   print_histogram(global_histogram);
  // skal begge kald ligge mellem lock og unlock.
  // Hvilken mutex? Hvorfor begge kald i samme sektion?
}

void *worker(void *arg) {
  struct job_queue *jq = arg;

  // TODO: loop mens job_queue_pop(...) returnerer 0
  //   - kald fhistogram med filnavnet
  //   - free filnavnet
  // TODO: return NULL
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

  assert(0); // Initialise the job queue and some worker threads here.

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
      assert(0); // Process the file p->fts_path, somehow.
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);

  assert(0); // Shut down the job queue and the worker threads here.

  move_lines(9);

  return 0;
}
