#include <assert.h>
#include <pthread.h>
#include <stdio.h>

int niterations = 10000;
int nobjects = 100000;
int nthreads = 4;
int size = 8;

class Foo {
public:
  Foo(void) : x(14), y(29) {}
  int x;
  int y;
};

void *thread_job(void *para) {
  int i, j;
  Foo **a;
  a = new Foo *[nobjects / nthreads];

  for (j = 0; j < niterations; j++) {
    
    for (i = 0; i < (nobjects / nthreads); i++) {
      a[i] = new Foo[size];
      assert(a[i]);
    }

    for (i = 0; i < (nobjects / nthreads); i++) {
      delete[] a[i];
    }
  }

  delete[] a;
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t threads[nthreads];

  for (int i = 0; i < nthreads; ++i)
    pthread_create(&threads[i], NULL, thread_job, NULL);

  for (int i = 0; i < nthreads; ++i)
    pthread_join(threads[i], NULL);
}