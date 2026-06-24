/* Negative case: a 2-function cycle up<->down where the SCC's external entry
 * (header) up() has NO recursion_bound. The DFS marks down->up as the backedge
 * into up's entry, but up is unbounded, so the cycle is UNBOUNDED and NO WCET
 * is produced -- even though down() carries a bound (a non-header bound does
 * not rescue the cycle). finalize() reports "RECURSION: mutual recursion among
 * {down, up} is UNBOUNDED". Guards the safety boundary: a missing header bound
 * must never silently yield an unsound WCET. */
volatile int n;

int down(int x);

/* header (called from main), intentionally NOT annotated */
int up(int x) {
  if (x <= 0)
    return 0;
  return down(x - 1);
}

#pragma recursion_bound(20)
int down(int x) {
  if (x <= 0)
    return 1;
  return up(x - 1);
}

int main(void) { return up(n); }
