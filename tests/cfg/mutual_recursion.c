/* Mutual recursion: ping() <-> pong() form a 2-function call-graph cycle,
 * bounded via #pragma recursion_bound. main calls ping(n), so the SCC's
 * external entry (header) is ping; a DFS over the cycle's call edges marks
 * pong->ping as the backedge into ping's entry and caps it at ping's
 * recursion_bound, with pong bounded transitively -> finite WCET. */
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
