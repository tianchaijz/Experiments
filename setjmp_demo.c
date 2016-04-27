#include <stdio.h>
#include <setjmp.h>

static int false(void);
static int true(void);
static void second(void);
static void first(void);


static jmp_buf buf;


int main(int argc, char *argv[]) {
  if ((!setjmp(buf) || false()) && true())
    first();             /* when executed, setjmp returns 0 */
  else                   /* when longjmp jumps back, setjmp returns 1 */
    printf("main\n");    /* prints */

  return 0;
}


static int false(void) {
  printf("hit false\n");
  return 0;
}

static int true(void) {
  printf("hit true\n");
  return 1;
}

static void second(void) {
  printf("second\n");
  longjmp(buf, 1);       /* jumps back to where setjmp was called - making setjmp now return 1 */
}

static void first(void) {
  second();
  printf("first\n");     /* does not print */
}

/* vi:set ft=c ts=2 sw=2 et fdm=marker: */
