/* CFG shape: a single counted loop bounded by a pragma. The accumulator is
 * volatile so each iteration performs an un-foldable memory access -- otherwise
 * indvars rewrites the sum to a closed form and collapses the loop. Baseline
 * that the loop-bound row is emitted and the WCET is finite. */
volatile int acc;

int main(void) {
#pragma loop_bound(0, 10)
  for (int i = 0; i < 10; i++)
    acc += i;
  return acc;
}
