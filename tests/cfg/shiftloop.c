/* CFG shape: a multi-bit shift on a wide type. MSP430 has no barrel shifter, so
 * the backend synthesizes a shift LOOP (e.g. clrc;rrc / add rX,rX self-loops)
 * that has no source statement and no IR loop. Exercises the implicit loop-bound
 * path (RTTarget::getImplicitLoopBound) that bounds it by the operand bit width
 * -- the category that previously left compress/edn unbounded. */
volatile unsigned long data;
volatile int out;

int main(void) {
  unsigned long x = data;
  x = x << 13;
  out = (int)x;
  return out;
}
