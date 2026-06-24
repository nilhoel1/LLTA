/* CFG shape: a loop formed by a backward goto rather than a for/while. Exercises
 * the LoopBoundPlugin's backward-goto recognition (so the pragma's JSON bound is
 * emitted) and the bound's application to the resulting machine loop. */
volatile int acc;

int main(void) {
  int i = 0;
loop:
  acc += i;
  i++;
#pragma loop_bound(0, 20)
  if (i < 20)
    goto loop;
  return acc;
}
