/* $Id: recursion.c,v 1.2 2005/04/04 11:34:58 csg Exp $ */

/* Generate an example of recursive code, to see  *
 * how it can be modeled in the scope graph.      */

/* self-recursion  */
/* main computes fib(10); with the two base cases (i==0, i==1) that makes
 * C(10)=177 total invocations of fib (C(i)=1+C(i-1)+C(i-2), C(0)=C(1)=1).
 * recursion_bound caps total invocations so the recursive call edges are
 * bounded like loop backedges and a finite WCET is produced. */
#pragma recursion_bound(177)
int fib(int i) {
  if (i == 0)
    return 1;
  if (i == 1)
    return 1;
  return fib(i - 1) + fib(i - 2);
}

/* mutual recursion */
int anka(int);

int kalle(int i) {
  if (i <= 0)
    return 0;
  else
    return anka(i - 1);
}

int anka(int i) {
  if (i <= 0)
    return 1;
  else
    return kalle(i - 1);
}

/* Provide a definition (was an undefined extern: link failed on a freestanding
 * MSP430 target with no external `In` symbol). */
volatile int In;

void main(void) { In = fib(10); }
