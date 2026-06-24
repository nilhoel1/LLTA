/* Linear (single-call-site) self-recursion bounded via #pragma recursion_bound.
 * sum(n) calls itself exactly once per frame, so total invocations = depth + 1.
 * recursion_bound(N) is the inter-procedural analog of a loop trip count: the
 * recursive call edge is the single backedge into sum's entry, capped at N. */
volatile int n;

#pragma recursion_bound(11)
int sum(int x) {
  if (x == 0)
    return 0;
  return x + sum(x - 1);
}

int main(void) { return sum(n); }
