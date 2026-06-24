/* CFG shape: the minimal start function (single block, no loops, no calls).
 * Smallest possible reachable graph; pins that the baseline path works. */
volatile int v;

int main(void) { return v + 1; }
