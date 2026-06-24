/* CFG shape: a call to a noreturn function. The call block has no continuation
 * successor, so ProgramGraph wires NO return edge (HasLanding == false). exit()
 * is an external body-less callee (resolved from libc at link), so the WCET is
 * additionally flagged UNSOUND; a WCET is still produced.
 * exit() is forward-declared rather than via <stdlib.h>, which is not on the
 * msp430 cc1 include path used by the test Makefile. */
void exit(int) __attribute__((noreturn));
volatile int go;

int main(void) {
  if (go)
    exit(1);
  return 0;
}
