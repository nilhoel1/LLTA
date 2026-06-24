/* CFG shape: a callee with multiple return blocks. ProgramGraph must collect
 * every isReturnBlock() and wire each back to the caller's landing block. */
volatile int in;

int classify(int x) {
  if (x < 0)
    return -1;
  if (x == 0)
    return 0;
  return 1;
}

int main(void) { return classify(in); }
