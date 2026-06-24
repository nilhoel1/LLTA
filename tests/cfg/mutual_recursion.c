/* Mutual recursion: ping() <-> pong() form a 2-function call-graph cycle.
 * Bounding multi-function SCCs is out of scope this phase (only direct
 * self-recursion is bounded), so even with recursion_bound pragmas the cycle is
 * UNBOUNDED and NO WCET is produced. finalize() reports the SCC ("RECURSION:
 * mutual recursion detected among {ping, pong}"). Documented expected failure. */
volatile int n;

int pong(int x);

#pragma recursion_bound(20)
int ping(int x) {
  if (x <= 0)
    return 0;
  return pong(x - 1);
}

#pragma recursion_bound(20)
int pong(int x) {
  if (x <= 0)
    return 1;
  return ping(x - 1);
}

int main(void) { return ping(n); }
