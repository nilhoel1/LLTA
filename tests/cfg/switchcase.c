/* CFG shape: switch dispatch (jump table or branch chain after lowering).
 * Exercises multi-successor blocks and merge points in the graph. */
volatile int sel;

int main(void) {
  int r = 0;
  switch (sel) {
  case 0:
    r = 10;
    break;
  case 1:
    r = 20;
    break;
  case 2:
    r = 30;
    break;
  case 3:
    r = 40;
    break;
  default:
    r = 99;
    break;
  }
  return r;
}
