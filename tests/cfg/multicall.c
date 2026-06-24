/* CFG shape: several calls in one source block (one basic block before the
 * CallSplitterPass runs). Exercises CallSplitterPass splitting a single block at
 * multiple call sites and ProgramGraph wiring each call/return pair. */
volatile int sink;

int a(int x) { return x + 1; }
int b(int x) { return x + 2; }
int c(int x) { return x + 3; }

int main(void) {
  int r = a(1) + b(2) + c(3);
  sink = r;
  return r;
}
