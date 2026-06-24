/* CFG shape: a nested loop followed by a sibling loop, all bounded by pragmas.
 * Exercises loop-bound aggregation across nesting and multiple headers. The
 * accumulator is volatile so the loops are not folded into a closed form. */
volatile int acc;

int main(void) {
#pragma loop_bound(0, 5)
  for (int i = 0; i < 5; i++) {
#pragma loop_bound(0, 4)
    for (int j = 0; j < 4; j++)
      acc += i * j;
  }
#pragma loop_bound(0, 8)
  for (int k = 0; k < 8; k++)
    acc += k;
  return acc;
}
