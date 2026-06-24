/* Three-function mutual-recursion cycle a -> b -> c -> a, bounded via
 * #pragma recursion_bound. main calls a(n) (external entry = header a); the DFS
 * over the SCC's call edges finds c->a as the backedge into a's entry and caps
 * it, with b and c bounded transitively -> finite WCET. Verifies back-edge
 * detection beyond a 2-node cycle. */
volatile int n;

int b(int);
int c(int);

#pragma recursion_bound(15)
int a(int i) {
  if (i <= 0)
    return 0;
  return b(i - 1);
}

#pragma recursion_bound(15)
int b(int i) {
  if (i <= 0)
    return 1;
  return c(i - 1);
}

#pragma recursion_bound(15)
int c(int i) {
  if (i <= 0)
    return 2;
  return a(i - 1);
}

int main(void) { return a(n); }
